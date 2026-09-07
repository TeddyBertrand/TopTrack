#pragma once

#include <vector>

#include "toptrack/physics.hpp"
#include "toptrack/protocol.hpp"

namespace toptrack::client::game {

// Reads raylib input, builds a CarInput, steps the shared physics model.
// Kept separate from rendering so it stays testable without a window.
// Also records a GhostFrame per update since the run started, so a
// finished run's trajectory can be submitted alongside its time.
class CarController {
public:
  void update(float dtSeconds);
  const toptrack::CarState &state() const { return state_; }
  const std::vector<toptrack::protocol::GhostFrame> &ghost() const { return ghost_; }

  // Clears recorded ghost frames and elapsed time for a fresh run.
  void resetRun();

private:
  toptrack::CarState state_{};
  toptrack::CarTuning tuning_{};
  std::vector<toptrack::protocol::GhostFrame> ghost_;
  float elapsedSeconds_ = 0.0f;
};

} // namespace toptrack::client::game
