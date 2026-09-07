#include "server/net/server.hpp"

#include <asio.hpp>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "server/db/database.hpp"
#include "server/tournament/round_manager.hpp"
#include "toptrack/protocol.hpp"

namespace toptrack::server::net {

using asio::ip::tcp;

namespace {

class Session;

// Shared state every Session dispatches into: persistence + the active
// round's standings, plus the set of currently-connected sessions so a
// LeaderboardUpdate can be broadcast to everyone racing that round.
struct Hub {
  db::Database database;
  tournament::RoundManager roundManager;
  std::set<std::shared_ptr<Session>> sessions;

  // Bootstrap round rotation: same track every time, roundId just
  // increments. Real round scheduling (different tracks per round,
  // admin-triggered) is still a TODO. Medal thresholds are no longer
  // hardcoded here, though — see startNextRound().
  std::string trackId = "track-1";
  double roundDurationSeconds = 180.0;
  toptrack::MedalTimes fallbackMedals{60000, 45000, 30000};
  int roundCounter = 1;

  explicit Hub(const std::string &dbPath) : database(dbPath) {}

  void startNextRound() {
    std::string roundId = "round-" + std::to_string(roundCounter++);
    // Prefer the real track's medal thresholds (set by whoever uploaded
    // it via the tile editor); fall back to a placeholder only if the
    // track hasn't been uploaded/saved yet.
    auto track = database.loadTrack(trackId);
    auto medals = track ? track->medals : fallbackMedals;
    roundManager.startRound(roundId, trackId, roundDurationSeconds, medals);
  }
};

// One framed message: [uint32 length][uint8 type][payload], matching
// shared/include/toptrack/protocol.hpp's MessageHeader.
class Session : public std::enable_shared_from_this<Session> {
public:
  Session(tcp::socket socket, Hub &hub)
      : socket_(std::move(socket)), hub_(hub) {}

  void start() { readHeader(); }

  const std::string &playerName() const { return playerName_; }

  void send(toptrack::protocol::MessageType type, const std::string &json) {
    std::string frame;
    uint32_t len = static_cast<uint32_t>(json.size());
    frame.resize(sizeof(len) + sizeof(uint8_t) + json.size());
    std::memcpy(frame.data(), &len, sizeof(len));
    uint8_t typeByte = static_cast<uint8_t>(type);
    std::memcpy(frame.data() + sizeof(len), &typeByte, sizeof(typeByte));
    std::memcpy(frame.data() + sizeof(len) + sizeof(typeByte), json.data(),
                json.size());

    bool writeInProgress = !outbox_.empty();
    outbox_.push_back(std::move(frame));
    if (!writeInProgress) doWrite();
  }

private:
  void doWrite() {
    auto self = shared_from_this();
    asio::async_write(socket_, asio::buffer(outbox_.front()),
                       [this, self](std::error_code ec, std::size_t) {
                         if (ec) return;
                         outbox_.pop_front();
                         if (!outbox_.empty()) doWrite();
                       });
  }

  void readHeader() {
    auto self = shared_from_this();
    asio::async_read(
        socket_, asio::buffer(&length_, sizeof(length_)),
        [this, self](std::error_code ec, std::size_t) {
          if (ec) return disconnect();
          asio::async_read(
              socket_, asio::buffer(&type_, sizeof(type_)),
              [this, self](std::error_code ec2, std::size_t) {
                if (ec2) return disconnect();
                readPayload();
              });
        });
  }

  void readPayload() {
    auto self = shared_from_this();
    payload_.resize(length_);
    asio::async_read(socket_, asio::buffer(payload_),
                      [this, self](std::error_code ec, std::size_t) {
                        if (ec) return disconnect();
                        dispatch();
                        readHeader();
                      });
  }

  // Client disconnected or a socket error occurred — drop this session
  // from the hub so it stops receiving broadcasts.
  void disconnect() { hub_.sessions.erase(shared_from_this()); }

  void dispatch() {
    using toptrack::protocol::MessageType;
    std::string json(payload_.begin(), payload_.end());

    switch (static_cast<MessageType>(type_)) {
      case MessageType::Hello: {
        auto hello = toptrack::protocol::deserializeHelloRequest(json);
        playerName_ = hello.playerName;
        std::cout << "hello from player=" << hello.playerName << "\n";

        toptrack::protocol::RoundStart roundStart;
        roundStart.roundId = hub_.roundManager.currentRoundId();
        roundStart.trackId = hub_.roundManager.currentTrackId();
        roundStart.durationSeconds = hub_.roundManager.currentDurationSeconds();
        send(MessageType::RoundStart, toptrack::protocol::serialize(roundStart));

        toptrack::protocol::LeaderboardUpdate update;
        update.roundId = roundStart.roundId;
        update.standings = hub_.database.bestTimesForTrack(roundStart.trackId);
        send(MessageType::LeaderboardUpdate, toptrack::protocol::serialize(update));
        break;
      }
      case MessageType::SubmitTime: {
        auto entry = toptrack::protocol::deserializeTimeEntry(json);
        // Anti-cheat sanity check only (no input replay yet — see the
        // function's own doc comment) — flagged, not rejected, since
        // there's no reject-response message type yet.
        if (!toptrack::protocol::isTimeEntryPlausible(entry, toptrack::CarTuning{})) {
          std::cout << "warning: implausible time from player="
                     << entry.playerName << " trackId=" << entry.trackId
                     << " timeMs=" << entry.timeMs << "\n";
        }
        // Score the medal before persisting so it lands in the times
        // table, not just in the live in-memory standings.
        entry.medal = toptrack::medalForTime(entry.timeMs, hub_.roundManager.currentMedals());
        hub_.database.recordTime(entry);
        auto update = hub_.roundManager.submitTime(entry);
        auto updateJson = toptrack::protocol::serialize(update);
        for (auto &session : hub_.sessions) {
          session->send(MessageType::LeaderboardUpdate, updateJson);
        }
        break;
      }
      case MessageType::TrackUpload: {
        auto track = toptrack::deserializeTrack(json);
        hub_.database.saveTrack(track);
        break;
      }
      case MessageType::TrackDownload: {
        auto request = toptrack::protocol::deserializeTrackRequest(json);
        auto track = hub_.database.loadTrack(request.trackId);
        // Empty `id` signals not-found; reuses TrackUpload as the response
        // type since it already carries a full Track payload.
        auto responseJson = toptrack::serialize(track.value_or(toptrack::Track{}));
        send(MessageType::TrackUpload, responseJson);
        break;
      }
      default:
        std::cout << "unhandled message type=" << static_cast<int>(type_)
                   << " bytes=" << length_ << "\n";
        break;
    }
  }

  tcp::socket socket_;
  Hub &hub_;
  uint32_t length_ = 0;
  uint8_t type_ = 0;
  std::vector<char> payload_;
  std::deque<std::string> outbox_;
  std::string playerName_ = "(pending hello)";
};

} // namespace

Server::Server(uint16_t port) : port_(port) {}

void Server::run() {
  asio::io_context io;
  tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), port_));

  Hub hub("toptrack.db");
  // Rounds rotate on hub.trackId every hub.roundDurationSeconds via the
  // timer below (medal thresholds sourced from the track itself, see
  // Hub::startNextRound), or on-demand via the admin console's "rotate".
  // Real admin-triggered *track selection between rounds* is handled by
  // "track <id>" below; a full admin UI/auth story is still open.
  hub.startNextRound();

  // Starts a fresh round and broadcasts its RoundStart to everyone still
  // connected, so clients pick up the new roundId without resending
  // Hello. Shared by the expiry timer and the admin "rotate" command.
  auto rotateRound = [&]() {
    hub.startNextRound();
    toptrack::protocol::RoundStart roundStart;
    roundStart.roundId = hub.roundManager.currentRoundId();
    roundStart.trackId = hub.roundManager.currentTrackId();
    roundStart.durationSeconds = hub.roundManager.currentDurationSeconds();
    auto roundStartJson = toptrack::protocol::serialize(roundStart);
    std::cout << "round rotated: " << roundStart.roundId
               << " (track=" << roundStart.trackId << ")\n";
    for (auto &session : hub.sessions) {
      session->send(toptrack::protocol::MessageType::RoundStart, roundStartJson);
    }
  };

  // Polls for round expiry every 5s (cheap relative to a 180s+ round).
  asio::steady_timer roundTimer(io);
  std::function<void()> scheduleRoundCheck = [&]() {
    roundTimer.expires_after(std::chrono::seconds(5));
    roundTimer.async_wait([&](std::error_code ec) {
      if (ec) return;
      if (hub.roundManager.hasExpired()) rotateRound();
      scheduleRoundCheck();
    });
  };
  scheduleRoundCheck();

  // Minimal admin console on stdin: "track <id>" changes which track the
  // *next* rotation uses, "rotate" forces one immediately (e.g. to apply
  // a track change without waiting out the current round), "status" prints
  // the live round/track/players snapshot, "players" lists connected
  // player names. Runs on its
  // own thread since std::getline blocks; posts back onto io's thread so
  // Hub is never touched concurrently from two threads.
  std::thread adminThread([&io, &hub, &rotateRound]() {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (line.rfind("track ", 0) == 0) {
        std::string trackId = line.substr(6);
        asio::post(io, [&hub, trackId]() {
          hub.trackId = trackId;
          std::cout << "next round will use track=" << trackId << "\n";
        });
      } else if (line == "rotate") {
        asio::post(io, rotateRound);
      } else if (line == "status") {
        asio::post(io, [&hub]() {
          std::cout << "round=" << hub.roundManager.currentRoundId()
                     << " track=" << hub.roundManager.currentTrackId()
                     << " remaining=" << hub.roundManager.secondsRemaining() << "s"
                     << " entries=" << hub.roundManager.standingsCount()
                     << " players=" << hub.sessions.size() << "\n";
        });
      } else if (line == "players") {
        asio::post(io, [&hub]() {
          if (hub.sessions.empty()) {
            std::cout << "no players connected\n";
            return;
          }
          for (auto &session : hub.sessions) {
            std::cout << "  " << session->playerName() << "\n";
          }
        });
      } else if (line == "tracks") {
        asio::post(io, [&hub]() {
          auto ids = hub.database.listTrackIds();
          if (ids.empty()) {
            std::cout << "no tracks saved\n";
            return;
          }
          for (auto &id : ids) {
            std::cout << "  " << id << (id == hub.trackId ? " (next round)" : "")
                       << "\n";
          }
        });
      } else if (line == "help") {
        std::cout << "admin commands:\n"
                      "  track <id>  - set the track the next rotation uses\n"
                      "  rotate      - force an immediate round rotation\n"
                      "  status      - print round/track/players snapshot\n"
                      "  players     - list connected player names\n"
                      "  tracks      - list saved track ids\n"
                      "  help        - show this list\n";
      } else if (!line.empty()) {
        std::cout << "unknown admin command: " << line
                   << " (try: help)\n";
      }
    }
  });
  adminThread.detach();

  std::function<void()> doAccept = [&]() {
    acceptor.async_accept([&](std::error_code ec, tcp::socket socket) {
      if (!ec) {
        std::cout << "client connected\n";
        auto session = std::make_shared<Session>(std::move(socket), hub);
        hub.sessions.insert(session);
        session->start();
      }
      doAccept();
    });
  };
  doAccept();

  std::cout << "toptrack_server listening on port " << port_ << "\n";
  io.run();
}

} // namespace toptrack::server::net
