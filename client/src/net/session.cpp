#include "client/net/session.hpp"

#include <utility>

namespace toptrack::client::net {

NetSession::~NetSession() { disconnect(); }

bool NetSession::connect(const std::string &host, uint16_t port,
                          const std::string &playerName) {
  if (connected_) return true;
  host_ = host;
  port_ = port;
  playerName_ = playerName;

  // A previous connection's receive thread may have exited on its own
  // (server dropped us) without disconnect() having joined it yet — a
  // std::thread object must be empty before being reassigned below.
  if (receiveThread_.joinable()) receiveThread_.join();

  if (!client_.connect(host, port)) return false;

  connected_ = true;
  receiveThread_ = std::thread(&NetSession::receiveLoop, this);
  client_.sendHello(playerName);
  return true;
}

bool NetSession::reconnect() {
  if (connected_) return true;
  return connect(host_, port_, playerName_);
}

void NetSession::disconnect() {
  if (!connected_) return;
  connected_ = false;
  client_.disconnect();
  if (receiveThread_.joinable()) receiveThread_.join();
}

void NetSession::submitTime(const toptrack::protocol::TimeEntry &entry) {
  if (!connected_) return;
  client_.sendTimeEntry(entry);
}

void NetSession::uploadTrack(const toptrack::Track &track) {
  if (!connected_) return;
  client_.uploadTrack(track);
}

std::optional<toptrack::protocol::LeaderboardUpdate>
NetSession::takeLeaderboard() {
  std::lock_guard<std::mutex> lock(leaderboardMutex_);
  return std::exchange(leaderboard_, std::nullopt);
}

std::optional<toptrack::protocol::RoundStart> NetSession::takeRoundStart() {
  std::lock_guard<std::mutex> lock(roundStartMutex_);
  return std::exchange(roundStart_, std::nullopt);
}

void NetSession::receiveLoop() {
  while (connected_) {
    auto msg = client_.receiveOne();
    if (!msg) break; // disconnected/error

    if (msg->first == toptrack::protocol::MessageType::LeaderboardUpdate) {
      auto update = toptrack::protocol::deserializeLeaderboardUpdate(msg->second);
      std::lock_guard<std::mutex> lock(leaderboardMutex_);
      leaderboard_ = std::move(update);
    } else if (msg->first == toptrack::protocol::MessageType::RoundStart) {
      auto roundStart = toptrack::protocol::deserializeRoundStart(msg->second);
      std::lock_guard<std::mutex> lock(roundStartMutex_);
      roundStart_ = std::move(roundStart);
    }
  }
  connected_ = false;
}

} // namespace toptrack::client::net
