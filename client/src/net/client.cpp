#include "client/net/client.hpp"

#include <asio.hpp>
#include <cstring>
#include <iostream>

namespace toptrack::client::net {

using asio::ip::tcp;

struct Client::Impl {
  asio::io_context io;
  tcp::socket socket{io};
};

Client::~Client() {
  disconnect();
  delete impl_;
}

bool Client::connect(const std::string &host, uint16_t port) {
  if (!impl_) impl_ = new Impl();

  try {
    tcp::resolver resolver(impl_->io);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    asio::connect(impl_->socket, endpoints);
    return true;
  } catch (const std::exception &e) {
    std::cerr << "client connect failed: " << e.what() << "\n";
    return false;
  }
}

void Client::disconnect() {
  if (impl_ && impl_->socket.is_open()) {
    std::error_code ec;
    impl_->socket.close(ec);
  }
}

namespace {
void sendFramed(tcp::socket &socket, toptrack::protocol::MessageType type,
                 const std::string &json) {
  std::string frame;
  uint32_t len = static_cast<uint32_t>(json.size());
  frame.resize(sizeof(len) + sizeof(uint8_t) + json.size());
  std::memcpy(frame.data(), &len, sizeof(len));
  uint8_t typeByte = static_cast<uint8_t>(type);
  std::memcpy(frame.data() + sizeof(len), &typeByte, sizeof(typeByte));
  std::memcpy(frame.data() + sizeof(len) + sizeof(typeByte), json.data(), json.size());
  asio::write(socket, asio::buffer(frame));
}
} // namespace

void Client::sendHello(const std::string &playerName) {
  if (!impl_) return;
  toptrack::protocol::HelloRequest hello;
  hello.playerName = playerName;
  sendFramed(impl_->socket, toptrack::protocol::MessageType::Hello,
             toptrack::protocol::serialize(hello));
}

void Client::sendTimeEntry(const toptrack::protocol::TimeEntry &entry) {
  if (!impl_) return;
  sendFramed(impl_->socket, toptrack::protocol::MessageType::SubmitTime,
             toptrack::protocol::serialize(entry));
}

void Client::uploadTrack(const toptrack::Track &track) {
  if (!impl_) return;
  sendFramed(impl_->socket, toptrack::protocol::MessageType::TrackUpload,
             toptrack::serialize(track));
}

void Client::requestTrack(const std::string &trackId) {
  if (!impl_) return;
  toptrack::protocol::TrackRequest request;
  request.trackId = trackId;
  sendFramed(impl_->socket, toptrack::protocol::MessageType::TrackDownload,
             toptrack::protocol::serialize(request));
}

std::optional<std::pair<toptrack::protocol::MessageType, std::string>>
Client::receiveOne() {
  if (!impl_) return std::nullopt;

  uint32_t length = 0;
  uint8_t type = 0;
  std::error_code ec;

  asio::read(impl_->socket, asio::buffer(&length, sizeof(length)), ec);
  if (ec) return std::nullopt;
  asio::read(impl_->socket, asio::buffer(&type, sizeof(type)), ec);
  if (ec) return std::nullopt;

  std::string payload(length, '\0');
  asio::read(impl_->socket, asio::buffer(payload.data(), length), ec);
  if (ec) return std::nullopt;

  return std::make_pair(static_cast<toptrack::protocol::MessageType>(type),
                         std::move(payload));
}

} // namespace toptrack::client::net
