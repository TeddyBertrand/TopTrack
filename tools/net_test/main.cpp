// Manual check that the client<->server link actually works: connect,
// submit a fake time, print the LeaderboardUpdate the server broadcasts
// back. Usage: toptrack_net_test <host> <port>
#include <cstdlib>
#include <iostream>

#include "client/net/client.hpp"
#include "toptrack/protocol.hpp"

int main(int argc, char **argv) {
  std::string host = argc > 1 ? argv[1] : "127.0.0.1";
  uint16_t port = argc > 2 ? static_cast<uint16_t>(std::atoi(argv[2])) : 7777;

  toptrack::client::net::Client client;
  if (!client.connect(host, port)) {
    std::cerr << "failed to connect to " << host << ":" << port << "\n";
    return 1;
  }
  std::cout << "connected to " << host << ":" << port << "\n";

  toptrack::protocol::TimeEntry entry;
  entry.playerName = "net_test";
  entry.trackId = "track-1";
  entry.timeMs = 12345.0;
  client.sendTimeEntry(entry);
  std::cout << "submitted time for player=" << entry.playerName
            << " trackId=" << entry.trackId << " timeMs=" << entry.timeMs
            << "\n";

  auto msg = client.receiveOne();
  if (!msg) {
    std::cerr << "no response from server (disconnected?)\n";
    return 1;
  }

  if (msg->first == toptrack::protocol::MessageType::LeaderboardUpdate) {
    auto update = toptrack::protocol::deserializeLeaderboardUpdate(msg->second);
    std::cout << "leaderboard update for round=" << update.roundId << ":\n";
    for (const auto &standing : update.standings) {
      std::cout << "  " << standing.playerName << " " << standing.timeMs
                << "ms\n";
    }
  } else {
    std::cout << "received unexpected message type="
              << static_cast<int>(msg->first) << "\n";
  }

  return 0;
}
