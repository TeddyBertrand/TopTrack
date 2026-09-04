#pragma once

#include <cstdint>

namespace toptrack::server::net {

// Custom TCP protocol server: one Asio io_context, one session per
// connected client, broadcasts LeaderboardUpdate messages to all sessions
// in the same round. See docs/protocol framing in shared/protocol.hpp.
class Server {
public:
  explicit Server(uint16_t port);
  void run();

private:
  uint16_t port_;
};

} // namespace toptrack::server::net
