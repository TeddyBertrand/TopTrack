#pragma once

#include <algorithm>
#include <chrono>
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

  // Returns the updated leaderboard so the caller can broadcast it. A
  // submission arriving after the round's durationSeconds has elapsed is
  // dropped (closes the round as a side effect) rather than scored — the
  // returned leaderboard is just whatever standings already existed.
  toptrack::protocol::LeaderboardUpdate submitTime(
      const toptrack::protocol::TimeEntry &entry);

  // True once durationSeconds has elapsed since startRound(); also closes
  // the round (isRoundActive() becomes false) as a side effect of the
  // check, so callers don't need a separate closeRound() call.
  bool hasExpired();

  bool isRoundActive() const { return roundActive_; }
  const std::string &currentRoundId() const { return currentRoundId_; }
  const std::string &currentTrackId() const { return currentTrackId_; }
  double currentDurationSeconds() const { return durationSeconds_; }
  const toptrack::MedalTimes &currentMedals() const { return medals_; }
  size_t standingsCount() const { return standings_.size(); }

  // Wall-clock seconds left before hasExpired() flips true; clamped to 0,
  // not itself a side-effecting check like hasExpired().
  double secondsRemaining() const {
    double elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - startTime_).count();
    return std::max(0.0, durationSeconds_ - elapsed);
  }

private:
  bool roundActive_ = false;
  std::string currentRoundId_;
  std::string currentTrackId_;
  double durationSeconds_ = 0;
  std::chrono::steady_clock::time_point startTime_;
  toptrack::MedalTimes medals_;
  std::vector<toptrack::protocol::TimeEntry> standings_;
};

} // namespace toptrack::server::tournament
