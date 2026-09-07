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

enum class Medal : uint8_t {
  None = 0,
  Bronze,
  Silver,
  Gold,
};

// Slowest-qualifying medal for a finish time, given goldMs < silverMs <
// bronzeMs thresholds. A threshold of 0 (unset) never qualifies.
Medal medalForTime(double timeMs, const MedalTimes &thresholds);

struct Track {
  std::string id;
  std::string name;
  std::string authorName;
  std::vector<Tile> tiles;
  MedalTimes medals;
};

std::string serialize(const Track &track);
Track deserializeTrack(const std::string &json);

// Standalone tile-list (de)serialization for storage layers that store
// tiles separately from the rest of the track row.
std::string serializeTiles(const std::vector<Tile> &tiles);
std::vector<Tile> deserializeTiles(const std::string &json);

} // namespace toptrack
