#include <raylib.h>

#include <cstdio>

#include "client/editor/tile_editor.hpp"
#include "client/game/car_controller.hpp"
#include "client/game/ghost_player.hpp"
#include "client/net/session.hpp"
#include "client/ui/hud.hpp"
#include "toptrack/protocol.hpp"

int main(int argc, char **argv) {
  InitWindow(1280, 720, "TopTrack");
  SetTargetFPS(60);

  toptrack::client::game::CarController car;
  toptrack::client::game::GhostPlayer ghostPlayer;
  toptrack::client::editor::TileEditor editor;
  toptrack::client::ui::Hud hud;
  hud.init();

  bool editorMode = false;

  std::string serverHost = argc > 1 ? argv[1] : "127.0.0.1";
  uint16_t serverPort = argc > 2 ? static_cast<uint16_t>(std::atoi(argv[2])) : 7777;

  toptrack::client::net::NetSession netSession;
  bool netConnected = netSession.connect(serverHost, serverPort, "player1");

  toptrack::protocol::LeaderboardUpdate lastLeaderboard;
  toptrack::protocol::RoundStart currentRound;
  float raceElapsedSeconds = 0.0f;

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    if (IsKeyPressed(KEY_E)) editorMode = !editorMode;

    if (editorMode) {
      editor.update(dt);
      // U uploads the in-progress track so the server can persist it.
      if (netConnected && IsKeyPressed(KEY_U)) {
        auto track = editor.track();
        track.id = "track-1";
        track.name = "editor track";
        track.authorName = "player1";
        netSession.uploadTrack(track);
      }
    } else {
      car.update(dt);
      hud.update(car.state(), dt);
      raceElapsedSeconds += dt;

      // T submits the elapsed run time (plus its recorded ghost) as a
      // fake finish — real finish-line detection isn't wired yet.
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
    }

    if (auto update = netSession.takeLeaderboard()) {
      lastLeaderboard = std::move(*update);
      // Race against the current best (standings are sorted best-first).
      if (!lastLeaderboard.standings.empty() && !lastLeaderboard.standings.front().ghost.empty()) {
        ghostPlayer.load(lastLeaderboard.standings.front().ghost);
      }
    }
    if (auto round = netSession.takeRoundStart()) {
      currentRound = std::move(*round);
    }

    BeginDrawing();
    ClearBackground(DARKGRAY);

    if (editorMode) {
      editor.draw();
      DrawText("EDITOR (E to exit, 1-6 tile type, click place/remove, R rotate, U upload)",
                10, 10, 16, RAYWHITE);
    } else {
      const auto &s = car.state();
      DrawCircle(static_cast<int>(640 + s.x), static_cast<int>(360 + s.y), 10, RED);

      if (auto ghostFrame = ghostPlayer.sampleAt(raceElapsedSeconds)) {
        DrawCircleLines(static_cast<int>(640 + ghostFrame->x), static_cast<int>(360 + ghostFrame->y),
                         10, SKYBLUE);
      }

      DrawText(netConnected ? "server: connected (T to submit time, E for editor)" : "server: not connected",
                10, 10, 18, netConnected ? GREEN : RED);

      if (!currentRound.roundId.empty()) {
        char roundLine[128];
        std::snprintf(roundLine, sizeof(roundLine), "round=%s track=%s",
                      currentRound.roundId.c_str(), currentRound.trackId.c_str());
        DrawText(roundLine, 10, 30, 16, LIGHTGRAY);
      }

      int y = 50;
      for (const auto &standing : lastLeaderboard.standings) {
        char line[128];
        std::snprintf(line, sizeof(line), "%s  %.0fms", standing.playerName.c_str(), standing.timeMs);
        DrawText(line, 10, y, 18, RAYWHITE);
        y += 20;
      }

      hud.draw();
    }
    EndDrawing();
  }

  netSession.disconnect();
  hud.shutdown();
  CloseWindow();
  return 0;
}
