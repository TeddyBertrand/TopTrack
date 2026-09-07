#include "client/game/ghost_player.hpp"

#include <algorithm>
#include <utility>

namespace toptrack::client::game {

void GhostPlayer::load(std::vector<toptrack::protocol::GhostFrame> frames) {
  frames_ = std::move(frames);
  std::sort(frames_.begin(), frames_.end(),
            [](const auto &a, const auto &b) { return a.t < b.t; });
}

void GhostPlayer::clear() { frames_.clear(); }

std::optional<toptrack::protocol::GhostFrame> GhostPlayer::sampleAt(float tSeconds) const {
  if (frames_.empty()) return std::nullopt;
  if (tSeconds < frames_.front().t || tSeconds > frames_.back().t) return std::nullopt;

  auto next = std::lower_bound(frames_.begin(), frames_.end(), tSeconds,
      [](const toptrack::protocol::GhostFrame &frame, float t) { return frame.t < t; });

  if (next == frames_.begin()) return *next;
  auto prev = std::prev(next);
  if (next == frames_.end()) return *prev;

  float span = next->t - prev->t;
  float alpha = span > 0.0f ? (tSeconds - prev->t) / span : 0.0f;

  toptrack::protocol::GhostFrame out;
  out.t = tSeconds;
  out.x = prev->x + (next->x - prev->x) * alpha;
  out.y = prev->y + (next->y - prev->y) * alpha;
  out.headingRad = prev->headingRad + (next->headingRad - prev->headingRad) * alpha;
  return out;
}

} // namespace toptrack::client::game
