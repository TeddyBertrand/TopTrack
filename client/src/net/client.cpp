#include "client/net/client.hpp"

#include <iostream>

namespace toptrack::client::net {

bool Client::connect(const std::string &host, uint16_t port) {
  // TODO: asio::ip::tcp::socket connect, matching server framing.
  std::cout << "TODO: connect to " << host << ":" << port << "\n";
  return false;
}

void Client::poll() {
  // TODO: pump async reads, dispatch RoundStart/LeaderboardUpdate.
}

void Client::disconnect() {
  // TODO: close socket.
}

} // namespace toptrack::client::net
