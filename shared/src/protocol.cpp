#include "toptrack/protocol.hpp"

#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

namespace toptrack::protocol {

using json = nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HelloRequest, playerName)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GhostFrame, t, x, y, headingRad)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TimeEntry, playerName, trackId, timeMs, ghost, medal)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoundStart, roundId, trackId, durationSeconds)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LeaderboardUpdate, roundId, standings)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TrackRequest, trackId)

std::string serialize(const RoundStart &msg) { return json(msg).dump(); }
std::string serialize(const TimeEntry &msg) { return json(msg).dump(); }
std::string serialize(const LeaderboardUpdate &msg) { return json(msg).dump(); }
std::string serialize(const TrackRequest &msg) { return json(msg).dump(); }
std::string serialize(const HelloRequest &msg) { return json(msg).dump(); }

RoundStart deserializeRoundStart(const std::string &s) {
  return json::parse(s).get<RoundStart>();
}
TimeEntry deserializeTimeEntry(const std::string &s) {
  return json::parse(s).get<TimeEntry>();
}
LeaderboardUpdate deserializeLeaderboardUpdate(const std::string &s) {
  return json::parse(s).get<LeaderboardUpdate>();
}

std::string serializeGhost(const std::vector<GhostFrame> &ghost) {
  return json(ghost).dump();
}

std::vector<GhostFrame> deserializeGhost(const std::string &s) {
  return json::parse(s).get<std::vector<GhostFrame>>();
}

TrackRequest deserializeTrackRequest(const std::string &s) {
  return json::parse(s).get<TrackRequest>();
}

HelloRequest deserializeHelloRequest(const std::string &s) {
  return json::parse(s).get<HelloRequest>();
}

bool isTimeEntryPlausible(const TimeEntry &entry, const toptrack::CarTuning &tuning) {
  if (entry.ghost.empty()) return false;

  // Reported time should match the ghost's own recorded duration
  // (last frame's t, in seconds) within half a second of slack.
  double ghostDurationMs = static_cast<double>(entry.ghost.back().t) * 1000.0;
  if (std::fabs(ghostDurationMs - entry.timeMs) > 500.0) return false;

  float topSpeed = tuning.maxSpeed[5];
  for (std::size_t i = 1; i < entry.ghost.size(); ++i) {
    const auto &prev = entry.ghost[i - 1];
    const auto &curr = entry.ghost[i];
    float dt = curr.t - prev.t;
    if (dt <= 0.0f) return false; // frames must be strictly increasing in time

    float dx = curr.x - prev.x;
    float dy = curr.y - prev.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    // 1.5x safety margin over top speed to absorb physics-step rounding.
    if (distance > topSpeed * dt * 1.5f) return false;
  }
  return true;
}

} // namespace toptrack::protocol
