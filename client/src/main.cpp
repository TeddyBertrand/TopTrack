#include <raylib.h>

#include <cstdio>

#include "client/editor/tile_editor.hpp"
#include "client/game/car_controller.hpp"
#include "client/net/session.hpp"
#include "client/ui/hud.hpp"
#include "toptrack/protocol.hpp"

int main(int argc, char **argv) {
  InitWindow(1280, 720, "TopTrack");
  SetTargetFPS(60);

  toptrack::client::game::CarController car;
  toptrack::client::ui::Hud hud;
  hud.init();

  std::string serverHost = argc > 1 ? argv[1] : "127.0.0.1";
  uint16_t serverPort = argc > 2 ? static_cast<uint16_t>(std::atoi(argv[2])) : 7777;

  toptrack::client::net::NetSession netSession;
  bool netConnected = netSession.connect(serverHost, serverPort);

  toptrack::protocol::LeaderboardUpdate lastLeaderboard;
  float raceElapsedSeconds = 0.0f;

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    car.update(dt);
    hud.update(car.state(), dt);
    raceElapsedSeconds += dt;

    // T submits the elapsed run time (plus its recorded ghost) as a fake
    // finish — real finish-line detection isn't wired yet.
    if (netConnected && IsKeyPressed(KEY_T)) {
      toptrack::protocol::TimeEntry entry;
      entry.playerName = "player1";
      entry.trackId = "track-1";
      entry.timeMs = raceElapsedSeconds * 1000.0;
      entry.ghost = car.ghost();
      netSession.submitTime(entry);
      car.resetRun();
      raceElapsedSeconds = 0.0f;
    }

    if (auto update = netSession.takeLeaderboard()) {
      lastLeaderboard = std::move(*update);
    }

    BeginDrawing();
    ClearBackground(DARKGRAY);

    const auto &s = car.state();
    DrawCircle(static_cast<int>(640 + s.x), static_cast<int>(360 + s.y), 10, RED);

    DrawText(netConnected ? "server: connected (T to submit time)" : "server: not connected",
              10, 10, 18, netConnected ? GREEN : RED);

    int y = 36;
    for (const auto &standing : lastLeaderboard.standings) {
      char line[128];
      std::snprintf(line, sizeof(line), "%s  %.0fms", standing.playerName.c_str(), standing.timeMs);
      DrawText(line, 10, y, 18, RAYWHITE);
      y += 20;
    }

    hud.draw();
    EndDrawing();
  }

  netSession.disconnect();
  hud.shutdown();
  CloseWindow();
  return 0;
}
