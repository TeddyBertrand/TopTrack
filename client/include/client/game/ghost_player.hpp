#pragma once

#include <optional>
#include <vector>

#include "toptrack/protocol.hpp"

namespace toptrack::client::game {

// Replays a recorded run's GhostFrame trail against wall-clock race time,
// so a past best can be drawn racing alongside the live car. Pure
// interpolation over already-recorded frames — no physics involved.
class GhostPlayer {
public:
  void load(std::vector<toptrack::protocol::GhostFrame> frames);
  void clear();
  bool hasGhost() const { return !frames_.empty(); }

  // Linearly interpolated position/heading at tSeconds since the ghost's
  // own run start. nullopt if there's no ghost loaded or tSeconds is past
  // the ghost's last recorded frame.
  std::optional<toptrack::protocol::GhostFrame> sampleAt(float tSeconds) const;

private:
  std::vector<toptrack::protocol::GhostFrame> frames_;
};

} // namespace toptrack::client::game
