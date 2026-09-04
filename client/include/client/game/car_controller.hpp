#pragma once

#include "toptrack/physics.hpp"

namespace toptrack::client::game {

// Reads raylib input, builds a CarInput, steps the shared physics model.
// Kept separate from rendering so it stays testable without a window.
class CarController {
public:
  void update(float dtSeconds);
  const toptrack::CarState &state() const { return state_; }

private:
  toptrack::CarState state_{};
  toptrack::CarTuning tuning_{};
};

} // namespace toptrack::client::game
