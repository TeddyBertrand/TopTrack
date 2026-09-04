#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace toptrack {

enum class TileType : uint8_t {
  Empty = 0,
  Straight,
  Curve,
  Chicane,
  Start,
  Checkpoint,
  Finish,
};

struct Tile {
  int x = 0;
  int y = 0;
  TileType type = TileType::Empty;
  int rotationDeg = 0;   // 0/90/180/270
  int checkpointOrder = -1; // only meaningful for Checkpoint tiles
};

struct MedalTimes {
  double bronzeMs = 0;
  double silverMs = 0;
  double goldMs = 0;
};

struct Track {
  std::string id;
  std::string name;
  std::string authorName;
  std::vector<Tile> tiles;
  MedalTimes medals;
};

} // namespace toptrack
