#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "toptrack/protocol.hpp"

namespace toptrack::client::net {

// Blocking Asio TCP client mirroring server/net/server.hpp framing:
// [uint32 length][uint8 MessageType][json payload]. Blocking is fine for
// now (simple to reason about/test); the real game loop will need to wrap
// this in a background thread or switch to async once it's driving a
// 60fps raylib loop.
class Client {
public:
  ~Client();

  bool connect(const std::string &host, uint16_t port);
  void disconnect();

  void sendTimeEntry(const toptrack::protocol::TimeEntry &entry);

  // Blocks until one framed message arrives; nullopt on disconnect/error.
  std::optional<std::pair<toptrack::protocol::MessageType, std::string>>
  receiveOne();

private:
  struct Impl;
  Impl *impl_ = nullptr;
};

} // namespace toptrack::client::net
