#include "toptrack/track.hpp"

#include <nlohmann/json.hpp>

namespace toptrack {

using json = nlohmann::json;

NLOHMANN_JSON_SERIALIZE_ENUM(TileType, {
  {TileType::Empty, "empty"},
  {TileType::Straight, "straight"},
  {TileType::Curve, "curve"},
  {TileType::Chicane, "chicane"},
  {TileType::Start, "start"},
  {TileType::Checkpoint, "checkpoint"},
  {TileType::Finish, "finish"},
})

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Tile, x, y, type, rotationDeg, checkpointOrder)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MedalTimes, bronzeMs, silverMs, goldMs)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Track, id, name, authorName, tiles, medals)

std::string serialize(const Track &track) { return json(track).dump(); }

Track deserializeTrack(const std::string &s) {
  return json::parse(s).get<Track>();
}

std::string serializeTiles(const std::vector<Tile> &tiles) {
  return json(tiles).dump();
}

std::vector<Tile> deserializeTiles(const std::string &s) {
  return json::parse(s).get<std::vector<Tile>>();
}

} // namespace toptrack
