#include <raylib.h>

#include "client/editor/tile_editor.hpp"
#include "client/game/car_controller.hpp"
#include "client/ui/hud.hpp"

int main() {
  InitWindow(1280, 720, "TopTrack");
  SetTargetFPS(60);

  toptrack::client::game::CarController car;
  toptrack::client::ui::Hud hud;
  hud.init();

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    car.update(dt);
    hud.update(car.state(), dt);

    BeginDrawing();
    ClearBackground(DARKGRAY);

    const auto &s = car.state();
    DrawCircle(static_cast<int>(640 + s.x), static_cast<int>(360 + s.y), 10, RED);

    hud.draw();
    EndDrawing();
  }

  hud.shutdown();
  CloseWindow();
  return 0;
}
