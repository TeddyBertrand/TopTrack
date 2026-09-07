#pragma once

#include <string>
#include <vector>

#include "toptrack/protocol.hpp"
#include "toptrack/track.hpp"

namespace toptrack::server::tournament {

// Owns the current COTD-style round: opens it, accepts submitted times,
// computes the live-sorted leaderboard, and assigns medals from the
// track's medal thresholds as each time comes in. No lockstep sync — each
// client races its own instance; this only aggregates results.
class RoundManager {
public:
  void startRound(const std::string &roundId, const std::string &trackId,
                   double durationSeconds,
                   const toptrack::MedalTimes &medals = {});

  // Returns the updated leaderboard so the caller can broadcast it.
  toptrack::protocol::LeaderboardUpdate submitTime(
      const toptrack::protocol::TimeEntry &entry);

  bool isRoundActive() const { return roundActive_; }

private:
  bool roundActive_ = false;
  std::string currentRoundId_;
  std::string currentTrackId_;
  toptrack::MedalTimes medals_;
  std::vector<toptrack::protocol::TimeEntry> standings_;
};

} // namespace toptrack::server::tournament
