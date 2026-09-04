#pragma once

#include <cstdint>
#include <string>

namespace toptrack::client::net {

// Asio TCP client mirroring server/net/server.hpp framing: connects to the
// host's server, sends SubmitTime, receives RoundStart/LeaderboardUpdate.
class Client {
public:
  bool connect(const std::string &host, uint16_t port);
  void poll(); // non-blocking pump of pending reads/writes
  void disconnect();
};

} // namespace toptrack::client::net
