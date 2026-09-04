#include "server/db/database.hpp"

#include <SQLiteCpp/SQLiteCpp.h>

namespace toptrack::server::db {

Database::Database(const std::string &path) : path_(path) { migrate(); }

void Database::migrate() {
  SQLite::Database db(path_, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
  db.exec(
      "CREATE TABLE IF NOT EXISTS tracks ("
      "  id TEXT PRIMARY KEY,"
      "  name TEXT NOT NULL,"
      "  author_name TEXT NOT NULL,"
      "  tiles_json TEXT NOT NULL,"
      "  bronze_ms REAL, silver_ms REAL, gold_ms REAL"
      ")");
  db.exec(
      "CREATE TABLE IF NOT EXISTS times ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  track_id TEXT NOT NULL,"
      "  player_name TEXT NOT NULL,"
      "  time_ms REAL NOT NULL,"
      "  ghost_json TEXT NOT NULL,"
      "  FOREIGN KEY(track_id) REFERENCES tracks(id)"
      ")");
}

void Database::saveTrack(const toptrack::Track &) {
  // TODO: serialize Track (tiles + medals) to JSON and upsert.
}

std::optional<toptrack::Track> Database::loadTrack(const std::string &) {
  // TODO: query + deserialize.
  return std::nullopt;
}

void Database::recordTime(const toptrack::protocol::TimeEntry &) {
  // TODO: insert into times, serializing ghost frames as JSON.
}

std::vector<toptrack::protocol::TimeEntry> Database::bestTimesForTrack(
    const std::string &, int) {
  // TODO: SELECT ... ORDER BY time_ms ASC LIMIT ?.
  return {};
}

} // namespace toptrack::server::db
