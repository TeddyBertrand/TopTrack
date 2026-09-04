#include "server/tournament/round_manager.hpp"

#include <algorithm>

namespace toptrack::server::tournament {

void RoundManager::startRound(const std::string &roundId,
                               const std::string &trackId,
                               double /*durationSeconds*/) {
  roundActive_ = true;
  currentRoundId_ = roundId;
  currentTrackId_ = trackId;
  standings_.clear();
}

toptrack::protocol::LeaderboardUpdate RoundManager::submitTime(
    const toptrack::protocol::TimeEntry &entry) {
  standings_.push_back(entry);
  std::sort(standings_.begin(), standings_.end(),
            [](const auto &a, const auto &b) { return a.timeMs < b.timeMs; });

  toptrack::protocol::LeaderboardUpdate update;
  update.roundId = currentRoundId_;
  update.standings = standings_;
  return update;
}

} // namespace toptrack::server::tournament
