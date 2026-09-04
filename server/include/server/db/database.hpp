#pragma once

#include <optional>
#include <string>
#include <vector>

#include "toptrack/protocol.hpp"
#include "toptrack/track.hpp"

namespace toptrack::server::db {

// Thin wrapper around SQLiteCpp — single-file embedded DB, sized for
// ~10-12 self-hosted players (see plan: no Postgres/Redis needed here).
class Database {
public:
  explicit Database(const std::string &path);

  void migrate(); // creates tables if they don't exist

  void saveTrack(const toptrack::Track &track);
  std::optional<toptrack::Track> loadTrack(const std::string &trackId);

  void recordTime(const toptrack::protocol::TimeEntry &entry);
  std::vector<toptrack::protocol::TimeEntry> bestTimesForTrack(
      const std::string &trackId, int limit = 100);

private:
  std::string path_;
};

} // namespace toptrack::server::db
