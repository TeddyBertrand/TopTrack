#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "toptrack/track.hpp"

namespace toptrack::protocol {

enum class MessageType : uint8_t {
  Hello = 1,
  RoundStart = 2,
  SubmitTime = 3,
  LeaderboardUpdate = 4,
  TrackUpload = 5,
  TrackDownload = 6,
};

struct GhostFrame {
  float t = 0;   // seconds since run start
  float x = 0;
  float y = 0;
  float headingRad = 0;
};

struct TimeEntry {
  std::string playerName;
  std::string trackId;
  double timeMs = 0;
  std::vector<GhostFrame> ghost;
};

struct RoundStart {
  std::string roundId;
  std::string trackId;
  double durationSeconds = 0;
};

struct LeaderboardUpdate {
  std::string roundId;
  std::vector<TimeEntry> standings; // sorted best-first
};

// Header prefixing every message on the wire: [uint32 length][uint8 type][json payload]
struct MessageHeader {
  uint32_t length = 0;
  MessageType type{};
};

std::string serialize(const RoundStart &msg);
std::string serialize(const TimeEntry &msg);
std::string serialize(const LeaderboardUpdate &msg);

RoundStart deserializeRoundStart(const std::string &json);
TimeEntry deserializeTimeEntry(const std::string &json);
LeaderboardUpdate deserializeLeaderboardUpdate(const std::string &json);

// Standalone ghost (de)serialization for storage layers (e.g. DB ghost_json
// column) that need just the frame list, not a full TimeEntry.
std::string serializeGhost(const std::vector<GhostFrame> &ghost);
std::vector<GhostFrame> deserializeGhost(const std::string &json);

} // namespace toptrack::protocol
