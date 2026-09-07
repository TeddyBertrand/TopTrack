#pragma once

#include "toptrack/track.hpp"

namespace toptrack::client::editor {

// Grid-based tile placement: mouse-to-grid picking, left-click places the
// selected tile type, right-click removes, R rotates the tile under the
// cursor, number keys 1-6 change the selected type. RmlUi toolbar for
// piece selection is still TODO — for now the raylib canvas + keyboard is
// the whole UI.
class TileEditor {
public:
  void update(float dtSeconds);
  void draw() const;

  const toptrack::Track &track() const { return track_; }

  static constexpr int kCellSizePx = 32;

private:
  toptrack::Tile *tileAt(int gridX, int gridY);

  toptrack::Track track_{};
  toptrack::TileType selectedType_ = toptrack::TileType::Straight;
  int nextCheckpointOrder_ = 0;
};

} // namespace toptrack::client::editor
