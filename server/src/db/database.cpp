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

void Database::saveTrack(const toptrack::Track &track) {
  SQLite::Database db(path_, SQLite::OPEN_READWRITE);
  SQLite::Statement stmt(db,
      "INSERT INTO tracks (id, name, author_name, tiles_json, bronze_ms, silver_ms, gold_ms) "
      "VALUES (?, ?, ?, ?, ?, ?, ?) "
      "ON CONFLICT(id) DO UPDATE SET name=excluded.name, author_name=excluded.author_name, "
      "tiles_json=excluded.tiles_json, bronze_ms=excluded.bronze_ms, "
      "silver_ms=excluded.silver_ms, gold_ms=excluded.gold_ms");
  stmt.bind(1, track.id);
  stmt.bind(2, track.name);
  stmt.bind(3, track.authorName);
  stmt.bind(4, toptrack::serializeTiles(track.tiles));
  stmt.bind(5, track.medals.bronzeMs);
  stmt.bind(6, track.medals.silverMs);
  stmt.bind(7, track.medals.goldMs);
  stmt.exec();
}

std::optional<toptrack::Track> Database::loadTrack(const std::string &trackId) {
  SQLite::Database db(path_, SQLite::OPEN_READONLY);
  SQLite::Statement stmt(db,
      "SELECT id, name, author_name, tiles_json, bronze_ms, silver_ms, gold_ms "
      "FROM tracks WHERE id = ?");
  stmt.bind(1, trackId);
  if (!stmt.executeStep()) return std::nullopt;

  toptrack::Track track;
  track.id = stmt.getColumn(0).getString();
  track.name = stmt.getColumn(1).getString();
  track.authorName = stmt.getColumn(2).getString();
  track.tiles = toptrack::deserializeTiles(stmt.getColumn(3).getString());
  track.medals.bronzeMs = stmt.getColumn(4).getDouble();
  track.medals.silverMs = stmt.getColumn(5).getDouble();
  track.medals.goldMs = stmt.getColumn(6).getDouble();
  return track;
}

void Database::recordTime(const toptrack::protocol::TimeEntry &entry) {
  SQLite::Database db(path_, SQLite::OPEN_READWRITE);
  SQLite::Statement stmt(db,
      "INSERT INTO times (track_id, player_name, time_ms, ghost_json) VALUES (?, ?, ?, ?)");
  stmt.bind(1, entry.trackId);
  stmt.bind(2, entry.playerName);
  stmt.bind(3, entry.timeMs);
  stmt.bind(4, toptrack::protocol::serializeGhost(entry.ghost));
  stmt.exec();
}

std::vector<toptrack::protocol::TimeEntry> Database::bestTimesForTrack(
    const std::string &trackId, int limit) {
  SQLite::Database db(path_, SQLite::OPEN_READONLY);
  SQLite::Statement stmt(db,
      "SELECT player_name, time_ms, ghost_json FROM times "
      "WHERE track_id = ? ORDER BY time_ms ASC LIMIT ?");
  stmt.bind(1, trackId);
  stmt.bind(2, limit);

  std::vector<toptrack::protocol::TimeEntry> results;
  while (stmt.executeStep()) {
    toptrack::protocol::TimeEntry entry;
    entry.playerName = stmt.getColumn(0).getString();
    entry.trackId = trackId;
    entry.timeMs = stmt.getColumn(1).getDouble();
    entry.ghost = toptrack::protocol::deserializeGhost(stmt.getColumn(2).getString());
    results.push_back(std::move(entry));
  }
  return results;
}

} // namespace toptrack::server::db
