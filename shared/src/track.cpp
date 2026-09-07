#include "toptrack/track.hpp"

#include <nlohmann/json.hpp>

namespace toptrack {

using json = nlohmann::json;

NLOHMANN_JSON_SERIALIZE_ENUM(Medal, {
  {Medal::None, "none"},
  {Medal::Bronze, "bronze"},
  {Medal::Silver, "silver"},
  {Medal::Gold, "gold"},
})

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

Medal medalForTime(double timeMs, const MedalTimes &thresholds) {
  if (thresholds.goldMs > 0 && timeMs <= thresholds.goldMs) return Medal::Gold;
  if (thresholds.silverMs > 0 && timeMs <= thresholds.silverMs) return Medal::Silver;
  if (thresholds.bronzeMs > 0 && timeMs <= thresholds.bronzeMs) return Medal::Bronze;
  return Medal::None;
}

} // namespace toptrack
