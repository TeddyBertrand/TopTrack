#include "client/ui/hud.hpp"

#include <raylib.h>

namespace toptrack::client::ui {

void Hud::init() {
  // TODO: Rml::SetSystemInterface/RenderInterface (raylib-backed), load
  // context + RML/RCSS documents for HUD/menus/leaderboard.
}

void Hud::update(const toptrack::CarState &, float) {
  // TODO: push speed/gear/timer values into RmlUi data model bindings.
}

void Hud::draw() const {
  // Placeholder text HUD until the RmlUi/raylib bridge exists.
  DrawText("TopTrack - HUD placeholder", 10, 10, 20, RAYWHITE);
}

void Hud::shutdown() {
  // TODO: Rml::Shutdown().
}

} // namespace toptrack::client::ui
