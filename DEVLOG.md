# DEVLOG

Running log of what got built, why, in plain terms — read this to
understand the codebase without digging through every file first.

## 2026-09-04 — Initial scaffold

Started from empty repo (README stub only). Set up whole project skeleton
per the agreed plan (see prompt history / CLAUDE.md for stack rationale).

**CMake project**, three targets:
- `shared/` — static lib `toptrack_shared`. Deps: nlohmann/json.
- `server/` — executable `toptrack_server`. Deps: shared, Asio, SQLiteCpp,
  Threads.
- `client/` — executable `toptrack_client`, optional
  (`-DTOPTRACK_BUILD_CLIENT=OFF` to skip). Deps: shared, Asio, raylib,
  RmlUi (RmlCore/RmlDebugger).

All third-party deps pulled via CMake `FetchContent` (no system package
install needed): asio (standalone, header-only), nlohmann/json 3.11.3,
SQLiteCpp 3.3.2, raylib 5.5, RmlUi 6.1.

**`shared/`** — the part both client and server link against, so they can
never disagree with each other:
- `physics.hpp/.cpp`: `stepCar()`, a hand-written top-down arcade car
  model. Gears give a stepped top-speed curve (`CarTuning::maxSpeed[1..5]`),
  drift is a grip/slip split driven by the handbrake input rather than a
  full tire-friction sim. One function, no physics engine — used by the
  client for local simulation and (intended) by the server to
  re-simulate/validate submitted runs.
- `track.hpp`: `Tile`/`TileType`/`Track`/`MedalTimes` — the tile-grid map
  format the editor will produce and the server will store.
- `protocol.hpp/.cpp`: wire message structs (`RoundStart`, `TimeEntry`,
  `GhostFrame`, `LeaderboardUpdate`) with nlohmann JSON
  (de)serialization via `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE`. Framing
  decided but only implemented on the server read side so far:
  `[uint32 length][uint8 MessageType][json payload]`.

**`server/`**:
- `net/server.cpp`: one Asio `io_context`, accepts connections, one
  `Session` object per client reading the `[length][type][payload]` frame.
  Currently only logs received messages (type + byte count) — routing
  into `db`/`tournament` is not wired yet.
- `db/database.cpp`: SQLiteCpp wrapper. `migrate()` creates the `tracks`
  and `times` tables and actually runs. `saveTrack`/`loadTrack`/
  `recordTime`/`bestTimesForTrack` are all `// TODO` stubs — no real
  persistence yet.
- `tournament/round_manager.cpp`: `RoundManager` — the one part of the
  server with real logic. `startRound()` resets standings;
  `submitTime()` appends a time, sorts by `timeMs` ascending, returns the
  updated `LeaderboardUpdate` for the caller to broadcast. Doesn't touch
  the DB yet.
- `main.cpp`: reads port from argv (default 7777), runs the server.

Verified: server configures + builds clean via
`cmake -S . -B build -DTOPTRACK_BUILD_CLIENT=OFF && cmake --build build -j`,
binary starts and listens without crashing.

**`client/`** (not build-verified yet — heavier deps):
- `game/car_controller.cpp`: reads raylib key state (arrows + space for
  handbrake), builds a `CarInput`, calls the shared `stepCar`.
- `editor/tile_editor.cpp`, `ui/hud.cpp`, `net/client.cpp`: structural
  stubs only (empty `// TODO` bodies) — no tile placement, no real UI, no
  networking logic yet.
- `ui/hud.cpp` currently just draws placeholder text via raylib
  `DrawText`. Important open problem noted in code/CLAUDE.md: **there is
  no official RmlUi backend for raylib** — a custom
  `RenderInterface`/`SystemInterface` bridging the two has to be written
  before RmlUi can actually render anything. `client/CMakeLists.txt` sets
  `RMLUI_BACKEND=none` because of this.
- `main.cpp`: opens a raylib window, steps the car controller + HUD each
  frame, draws the car as a red circle offset from window center (10px
  radius) — purely a "does the loop run" placeholder, not real rendering.

**Docs added**: `README.md` (build/run instructions), `CLAUDE.md`
(guidance file for future Claude Code sessions), this file.

## 2026-09-04 — Build/run scripts + Makefile

Added `scripts/build.sh` (configure+build, `--server-only` flag maps to
`-DTOPTRACK_BUILD_CLIENT=OFF`), `scripts/run-server.sh` [port],
`scripts/run-client.sh`, and a `Makefile` wrapping them
(`make build`, `make server-only`, `make run-server`, `make run-client`,
`make clean`). Verified `make server-only` + running the binary works.

## 2026-09-04 — One-shot dev scripts

Added `scripts/dev-server.sh [port]` and `scripts/dev-client.sh`, each
build+run in one call (wraps `build.sh` + `run-*.sh`). `make run-server`/
`make run-client` now call these instead of separate build/run steps.

## 2026-09-04 — Server↔shared protocol routing

`server/src/net/server.cpp` no longer just logs messages — added a `Hub`
(owns `Database` + `RoundManager` + the set of connected `Session`s) that
every `Session` dispatches into. `SubmitTime` messages now: deserialize to
`TimeEntry`, `database.recordTime()`, `roundManager.submitTime()` (sorts
standings), then broadcast the resulting `LeaderboardUpdate` to every
connected session. `Session::send()` added (frames + queues writes so
concurrent broadcasts don't interleave on one socket). Sessions are
dropped from the `Hub` on read/write error so dead connections stop
receiving broadcasts.

Temporary bootstrap: server auto-starts one round (`"round-1"` /
`"track-1"`, 180s) on startup so `SubmitTime` has somewhere to land —
real round scheduling (admin trigger / cron) doesn't exist yet, tracked as
a TODO in `server.cpp`.

Other message types (`RoundStart`, `TrackUpload`/`TrackDownload`) are
still unhandled (`default:` case logs and ignores) — `Track` doesn't have
JSON (de)serialization wired in `shared/` yet, so track upload/download
routing is next.

Verified: `make server-only` builds clean, server starts, creates/migrates
`toptrack.db` on launch.

## 2026-09-04 — Basic client net implementation + link verification

Implemented `client/net/client.hpp/.cpp` for real (was a stub): blocking
Asio TCP client (pimpl'd `asio::io_context`/`tcp::socket` to keep asio out
of the header) matching the server's `[length][type][payload]` framing —
`connect()`, `sendTimeEntry()`, `receiveOne()` (blocks for one framed
message, returns `nullopt` on disconnect), `disconnect()`. Blocking is
deliberate for now — simple to reason about and test; the real raylib game
loop will need this backgrounded or made async later, noted in the header.

Added `tools/net_test` — a small standalone executable
(`toptrack_net_test <host> <port>`) that links the real
`client/src/net/client.cpp` plus `toptrack_shared`, but **not**
raylib/RmlUi, so it builds fast and works under
`-DTOPTRACK_BUILD_CLIENT=OFF`. It connects, submits a fake `TimeEntry`,
prints whatever `LeaderboardUpdate` comes back. This is how the
server↔shared routing from the previous entry actually got verified
end-to-end rather than by inspection.

Verified: `make server-only` builds `toptrack_server` +
`toptrack_net_test`; ran the server, then `toptrack_net_test` against it —
connected, submitted a time, got back a correctly-populated
`LeaderboardUpdate` (`round-1`, one standing, matching player/time). Real
client (raylib+RmlUi) net loop integration still not done — `client/net`
now has working guts but `client/src/main.cpp` doesn't call into it yet.

**Known gaps / next likely steps** (not yet started): server↔shared
protocol routing (server currently only logs messages, doesn't dispatch
to `RoundManager`/`Database`), DB persistence methods, RmlUi/raylib
render bridge, tile editor logic, ghost recording/playback, medal
computation from `MedalTimes` thresholds, client build verification.

## Client net loop wired into main.cpp

Added `client/net/session.hpp/.cpp` (`NetSession`): wraps the blocking
`Client` in a background thread so the 60fps raylib loop never stalls on
`receiveOne()`. Background thread owns the receive loop and stashes the
latest `LeaderboardUpdate` behind a mutex (`takeLeaderboard()`); the main
thread calls `submitTime()` to write. Concurrent blocking read (bg
thread) + write (main thread) on the same Asio socket is fine as long as
writes don't overlap each other, which holds here since `submitTime()`
is only ever called from the main thread.

`client/src/main.cpp` now connects to `127.0.0.1:7777` (overridable via
argv) at startup, submits a fake finish time (elapsed run seconds) on
pressing `T` — same shape as `tools/net_test`, real finish-line detection
isn't wired yet — and draws whatever `LeaderboardUpdate` last arrived as
plain text over the placeholder car view.

Added `find_package(Threads REQUIRED)` / `Threads::Threads` to
`client/CMakeLists.txt` for `std::thread`.

Not verified against a running window: this sandbox's raylib build fails
at CMake configure (`Could NOT find X11` — GLFW dependency missing from
the environment, unrelated to this change). Verified instead by
`g++ -fsyntax-only` against the real asio/nlohmann-json/raylib headers
pulled into `build/_deps/` — `session.cpp` and `main.cpp` both compile
clean. Full client build + in-window smoke test still needed on a
machine with X11 dev libs present.
