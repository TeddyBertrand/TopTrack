#include "server/net/server.hpp"

#include <asio.hpp>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
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

  explicit Hub(const std::string &dbPath) : database(dbPath) {}
};

// One framed message: [uint32 length][uint8 type][payload], matching
// shared/include/toptrack/protocol.hpp's MessageHeader.
class Session : public std::enable_shared_from_this<Session> {
public:
  Session(tcp::socket socket, Hub &hub)
      : socket_(std::move(socket)), hub_(hub) {}

  void start() { readHeader(); }

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
      case MessageType::SubmitTime: {
        auto entry = toptrack::protocol::deserializeTimeEntry(json);
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
};

} // namespace

Server::Server(uint16_t port) : port_(port) {}

void Server::run() {
  asio::io_context io;
  tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), port_));

  Hub hub("toptrack.db");
  // TODO: real round scheduling (admin command / cron). Bootstrapped here
  // so SubmitTime has an active round to land in until that exists.
  // TODO: source medal thresholds from the actual track once round
  // scheduling loads a real Track via hub.database.loadTrack instead of
  // hardcoding round-1/track-1 here.
  hub.roundManager.startRound("round-1", "track-1", 180.0,
                               toptrack::MedalTimes{60000, 45000, 30000});

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
