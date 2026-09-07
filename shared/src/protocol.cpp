#include "toptrack/protocol.hpp"

#include <nlohmann/json.hpp>

namespace toptrack::protocol {

using json = nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GhostFrame, t, x, y, headingRad)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TimeEntry, playerName, trackId, timeMs, ghost)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoundStart, roundId, trackId, durationSeconds)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LeaderboardUpdate, roundId, standings)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TrackRequest, trackId)

std::string serialize(const RoundStart &msg) { return json(msg).dump(); }
std::string serialize(const TimeEntry &msg) { return json(msg).dump(); }
std::string serialize(const LeaderboardUpdate &msg) { return json(msg).dump(); }
std::string serialize(const TrackRequest &msg) { return json(msg).dump(); }

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

} // namespace toptrack::protocol
