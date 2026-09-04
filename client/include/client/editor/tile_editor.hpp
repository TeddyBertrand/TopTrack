#pragma once

#include "toptrack/track.hpp"

namespace toptrack::client::editor {

// Grid-based tile placement (RmlUi toolbar for piece selection + raylib
// grid canvas for placement/rotation). Stub for now — see plan Phase:
// map editor.
class TileEditor {
public:
  void update(float dtSeconds);
  void draw() const;

  const toptrack::Track &track() const { return track_; }

private:
  toptrack::Track track_{};
};

} // namespace toptrack::client::editor
