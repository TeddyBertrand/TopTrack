#include "client/editor/tile_editor.hpp"

namespace toptrack::client::editor {

void TileEditor::update(float /*dtSeconds*/) {
  // TODO: mouse-to-grid picking, tile placement/rotation, checkpoint
  // ordering, save/load via nlohmann::json.
}

void TileEditor::draw() const {
  // TODO: draw grid + placed tiles with raylib.
}

} // namespace toptrack::client::editor
