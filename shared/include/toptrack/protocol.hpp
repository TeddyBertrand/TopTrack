#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "toptrack/physics.hpp"
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

// Hello payload sent right after connecting; server replies with the
// active round's RoundStart followed by a LeaderboardUpdate seeded from
// that track's persisted best times.
struct HelloRequest {
  std::string playerName;
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
  toptrack::Medal medal = toptrack::Medal::None; // set server-side on submit
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

// TrackDownload request payload. The response reuses MessageType::TrackUpload
// with a toptrack::serialize(Track) payload — an empty `id` means not found.
struct TrackRequest {
  std::string trackId;
};

// Header prefixing every message on the wire: [uint32 length][uint8 type][json payload]
struct MessageHeader {
  uint32_t length = 0;
  MessageType type{};
};

std::string serialize(const RoundStart &msg);
std::string serialize(const TimeEntry &msg);
std::string serialize(const LeaderboardUpdate &msg);
std::string serialize(const TrackRequest &msg);
std::string serialize(const HelloRequest &msg);

RoundStart deserializeRoundStart(const std::string &json);
TimeEntry deserializeTimeEntry(const std::string &json);
LeaderboardUpdate deserializeLeaderboardUpdate(const std::string &json);
TrackRequest deserializeTrackRequest(const std::string &json);
HelloRequest deserializeHelloRequest(const std::string &json);

// Standalone ghost (de)serialization for storage layers (e.g. DB ghost_json
// column) that need just the frame list, not a full TimeEntry.
std::string serializeGhost(const std::vector<GhostFrame> &ghost);
std::vector<GhostFrame> deserializeGhost(const std::string &json);

// Lightweight anti-cheat sanity check the server runs on a submitted
// TimeEntry: does NOT re-simulate inputs through stepCar (GhostFrame
// carries only position/heading, not the CarInput that produced it) — it
// just checks the reported timeMs matches the ghost's own duration, and
// that no consecutive pair of frames implies a speed the given tuning
// could never reach. Real input replay would need the client to also
// record/send CarInput per frame, which it doesn't yet.
bool isTimeEntryPlausible(const TimeEntry &entry, const toptrack::CarTuning &tuning);

} // namespace toptrack::protocol
