#pragma once

#include "toptrack/physics.hpp"

namespace toptrack::client::ui {

// Owns the RmlUi context for in-race HUD (speed/gear, checkpoint timer,
// live leaderboard panel). Real RmlUi RenderInterface/SystemInterface
// bridge to raylib lands here — stub for scaffold.
class Hud {
public:
  void init();
  void update(const toptrack::CarState &carState, float dtSeconds);
  void draw() const;
  void shutdown();
};

} // namespace toptrack::client::ui
