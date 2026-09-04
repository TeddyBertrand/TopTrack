#include "server/net/server.hpp"

#include <asio.hpp>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

namespace toptrack::server::net {

using asio::ip::tcp;

namespace {

// Reads and discards one framed message: [uint32 length][uint8 type][payload].
// Placeholder session loop — routing into db/tournament lands in the next pass.
class Session : public std::enable_shared_from_this<Session> {
public:
  explicit Session(tcp::socket socket) : socket_(std::move(socket)) {}

  void start() { readHeader(); }

private:
  void readHeader() {
    auto self = shared_from_this();
    asio::async_read(
        socket_, asio::buffer(&length_, sizeof(length_)),
        [this, self](std::error_code ec, std::size_t) {
          if (ec) return;
          asio::async_read(
              socket_, asio::buffer(&type_, sizeof(type_)),
              [this, self](std::error_code ec2, std::size_t) {
                if (ec2) return;
                readPayload();
              });
        });
  }

  void readPayload() {
    auto self = shared_from_this();
    payload_.resize(length_);
    asio::async_read(socket_, asio::buffer(payload_),
                      [this, self](std::error_code ec, std::size_t) {
                        if (ec) return;
                        std::cout << "received message type="
                                  << static_cast<int>(type_)
                                  << " bytes=" << length_ << "\n";
                        readHeader();
                      });
  }

  tcp::socket socket_;
  uint32_t length_ = 0;
  uint8_t type_ = 0;
  std::vector<char> payload_;
};

} // namespace

Server::Server(uint16_t port) : port_(port) {}

void Server::run() {
  asio::io_context io;
  tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), port_));

  std::function<void()> doAccept = [&]() {
    acceptor.async_accept([&](std::error_code ec, tcp::socket socket) {
      if (!ec) {
        std::cout << "client connected\n";
        std::make_shared<Session>(std::move(socket))->start();
      }
      doAccept();
    });
  };
  doAccept();

  std::cout << "toptrack_server listening on port " << port_ << "\n";
  io.run();
}

} // namespace toptrack::server::net
