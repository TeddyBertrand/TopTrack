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

**Known gaps / next likely steps** (not yet started): server↔shared
protocol routing (server currently only logs messages, doesn't dispatch
to `RoundManager`/`Database`), DB persistence methods, RmlUi/raylib
render bridge, tile editor logic, client networking, ghost recording/
playback, medal computation from `MedalTimes` thresholds, client build
verification.
