#include "server/tournament/round_manager.hpp"

#include <algorithm>

namespace toptrack::server::tournament {

void RoundManager::startRound(const std::string &roundId,
                               const std::string &trackId,
                               double /*durationSeconds*/,
                               const toptrack::MedalTimes &medals) {
  roundActive_ = true;
  currentRoundId_ = roundId;
  currentTrackId_ = trackId;
  medals_ = medals;
  standings_.clear();
}

toptrack::protocol::LeaderboardUpdate RoundManager::submitTime(
    const toptrack::protocol::TimeEntry &entry) {
  auto scored = entry;
  scored.medal = toptrack::medalForTime(scored.timeMs, medals_);
  standings_.push_back(scored);
  std::sort(standings_.begin(), standings_.end(),
            [](const auto &a, const auto &b) { return a.timeMs < b.timeMs; });

  toptrack::protocol::LeaderboardUpdate update;
  update.roundId = currentRoundId_;
  update.standings = standings_;
  return update;
}

} // namespace toptrack::server::tournament
