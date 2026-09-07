// Manual check that the client<->server link actually works: connect,
// submit a fake time, print the LeaderboardUpdate the server broadcasts
// back. Usage: toptrack_net_test <host> <port>
#include <cstdlib>
#include <iostream>

#include "client/net/client.hpp"
#include "toptrack/protocol.hpp"
#include "toptrack/track.hpp"

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
                << "ms medal=" << static_cast<int>(standing.medal) << "\n";
    }
  } else {
    std::cout << "received unexpected message type="
              << static_cast<int>(msg->first) << "\n";
  }

  toptrack::Track track;
  track.id = "track-1";
  track.name = "net_test track";
  track.authorName = "net_test";
  track.tiles.push_back({0, 0, toptrack::TileType::Start, 0, -1});
  track.tiles.push_back({1, 0, toptrack::TileType::Finish, 0, -1});
  track.medals = {60000, 45000, 30000};
  client.uploadTrack(track);
  std::cout << "uploaded track id=" << track.id << " tiles=" << track.tiles.size() << "\n";

  client.requestTrack(track.id);
  auto trackMsg = client.receiveOne();
  if (!trackMsg) {
    std::cerr << "no response to track download (disconnected?)\n";
    return 1;
  }
  if (trackMsg->first == toptrack::protocol::MessageType::TrackUpload) {
    auto downloaded = toptrack::deserializeTrack(trackMsg->second);
    if (downloaded.id.empty()) {
      std::cerr << "track not found on server\n";
      return 1;
    }
    std::cout << "downloaded track id=" << downloaded.id
              << " name=" << downloaded.name
              << " tiles=" << downloaded.tiles.size() << "\n";
  } else {
    std::cout << "received unexpected message type="
              << static_cast<int>(trackMsg->first) << "\n";
  }

  return 0;
}
