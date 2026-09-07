#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "client/net/client.hpp"
#include "toptrack/protocol.hpp"

namespace toptrack::client::net {

// Drives a Client from the raylib game loop: owns a background thread that
// blocks in receiveOne() so 60fps rendering never stalls on the network,
// while submitTime() writes from the main thread. Client's underlying
// socket is a synchronous Asio socket, and concurrent read (background
// thread) + write (main thread) on the same socket from different threads
// is safe as long as writes themselves don't overlap, which callers here
// don't need to worry about since submitTime() is only ever called from
// the main thread.
class NetSession {
public:
  ~NetSession();

  // Connects and immediately sends a Hello for playerName, so the server's
  // RoundStart + seeded LeaderboardUpdate reply arrive via the background
  // receive loop shortly after.
  bool connect(const std::string &host, uint16_t port, const std::string &playerName);
  void disconnect();

  void submitTime(const toptrack::protocol::TimeEntry &entry);
  void uploadTrack(const toptrack::Track &track);

  // Latest LeaderboardUpdate received from the server, if any arrived
  // since the last call.
  std::optional<toptrack::protocol::LeaderboardUpdate> takeLeaderboard();

  // Latest RoundStart received from the server (sent in reply to Hello),
  // if any arrived since the last call.
  std::optional<toptrack::protocol::RoundStart> takeRoundStart();

private:
  void receiveLoop();

  Client client_;
  std::thread receiveThread_;
  std::atomic<bool> connected_{false};

  std::mutex leaderboardMutex_;
  std::optional<toptrack::protocol::LeaderboardUpdate> leaderboard_;

  std::mutex roundStartMutex_;
  std::optional<toptrack::protocol::RoundStart> roundStart_;
};

} // namespace toptrack::client::net
