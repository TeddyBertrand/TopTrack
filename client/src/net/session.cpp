#include "client/net/session.hpp"

#include <utility>

namespace toptrack::client::net {

NetSession::~NetSession() { disconnect(); }

bool NetSession::connect(const std::string &host, uint16_t port) {
  if (connected_) return true;
  if (!client_.connect(host, port)) return false;

  connected_ = true;
  receiveThread_ = std::thread(&NetSession::receiveLoop, this);
  return true;
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

std::optional<toptrack::protocol::LeaderboardUpdate>
NetSession::takeLeaderboard() {
  std::lock_guard<std::mutex> lock(leaderboardMutex_);
  return std::exchange(leaderboard_, std::nullopt);
}

void NetSession::receiveLoop() {
  while (connected_) {
    auto msg = client_.receiveOne();
    if (!msg) break; // disconnected/error

    if (msg->first == toptrack::protocol::MessageType::LeaderboardUpdate) {
      auto update = toptrack::protocol::deserializeLeaderboardUpdate(msg->second);
      std::lock_guard<std::mutex> lock(leaderboardMutex_);
      leaderboard_ = std::move(update);
    }
  }
  connected_ = false;
}

} // namespace toptrack::client::net
