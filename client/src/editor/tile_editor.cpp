#include "client/editor/tile_editor.hpp"

#include <raylib.h>

namespace toptrack::client::editor {

toptrack::Tile *TileEditor::tileAt(int gridX, int gridY) {
  for (auto &tile : track_.tiles) {
    if (tile.x == gridX && tile.y == gridY) return &tile;
  }
  return nullptr;
}

void TileEditor::update(float /*dtSeconds*/) {
  if (IsKeyPressed(KEY_ONE)) selectedType_ = toptrack::TileType::Straight;
  if (IsKeyPressed(KEY_TWO)) selectedType_ = toptrack::TileType::Curve;
  if (IsKeyPressed(KEY_THREE)) selectedType_ = toptrack::TileType::Chicane;
  if (IsKeyPressed(KEY_FOUR)) selectedType_ = toptrack::TileType::Start;
  if (IsKeyPressed(KEY_FIVE)) selectedType_ = toptrack::TileType::Checkpoint;
  if (IsKeyPressed(KEY_SIX)) selectedType_ = toptrack::TileType::Finish;

  Vector2 mouse = GetMousePosition();
  int gridX = static_cast<int>(mouse.x) / kCellSizePx;
  int gridY = static_cast<int>(mouse.y) / kCellSizePx;

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    auto *existing = tileAt(gridX, gridY);
    if (!existing) {
      toptrack::Tile tile;
      tile.x = gridX;
      tile.y = gridY;
      tile.type = selectedType_;
      if (selectedType_ == toptrack::TileType::Checkpoint) {
        tile.checkpointOrder = nextCheckpointOrder_++;
      }
      track_.tiles.push_back(tile);
    } else {
      existing->type = selectedType_;
    }
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    for (auto it = track_.tiles.begin(); it != track_.tiles.end(); ++it) {
      if (it->x == gridX && it->y == gridY) {
        track_.tiles.erase(it);
        break;
      }
    }
  }

  if (IsKeyPressed(KEY_R)) {
    if (auto *tile = tileAt(gridX, gridY)) {
      tile->rotationDeg = (tile->rotationDeg + 90) % 360;
    }
  }
}

namespace {
Color colorForTileType(toptrack::TileType type) {
  switch (type) {
    case toptrack::TileType::Straight: return GRAY;
    case toptrack::TileType::Curve: return SKYBLUE;
    case toptrack::TileType::Chicane: return ORANGE;
    case toptrack::TileType::Start: return GREEN;
    case toptrack::TileType::Checkpoint: return YELLOW;
    case toptrack::TileType::Finish: return RED;
    case toptrack::TileType::Empty: default: return BLANK;
  }
}
} // namespace

void TileEditor::draw() const {
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  for (int x = 0; x < screenW; x += kCellSizePx) DrawLine(x, 0, x, screenH, DARKGRAY);
  for (int y = 0; y < screenH; y += kCellSizePx) DrawLine(0, y, screenW, y, DARKGRAY);

  for (const auto &tile : track_.tiles) {
    int px = tile.x * kCellSizePx;
    int py = tile.y * kCellSizePx;
    DrawRectangle(px + 2, py + 2, kCellSizePx - 4, kCellSizePx - 4, colorForTileType(tile.type));

    if (tile.rotationDeg != 0) {
      Vector2 center{static_cast<float>(px + kCellSizePx / 2), static_cast<float>(py + kCellSizePx / 2)};
      DrawLine(static_cast<int>(center.x), static_cast<int>(center.y),
                static_cast<int>(center.x), py + 4, WHITE);
    }
  }

  Vector2 mouse = GetMousePosition();
  int gridX = static_cast<int>(mouse.x) / kCellSizePx;
  int gridY = static_cast<int>(mouse.y) / kCellSizePx;
  DrawRectangleLines(gridX * kCellSizePx, gridY * kCellSizePx, kCellSizePx, kCellSizePx, WHITE);
}

} // namespace toptrack::client::editor
