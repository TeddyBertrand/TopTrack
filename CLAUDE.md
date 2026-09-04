# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

TopTrack: 2D top-down Trackmania-like racing game. Drift/gear/slide car
handling, tile-based map editor, time-attack with bronze/silver/gold
medals, COTD-style tournaments (live-updating shared leaderboard, not
lockstep-synced racing) for a small self-hosted group (~10-12 players).

No game engine — hand-written C++20 client and server. Design rationale
(why C++/raylib/RmlUi/Asio/SQLite over alternatives, and why no
Postgres/Redis at this scale) lives in the project plan; don't re-litigate
those choices without cause.

## Build

```
cmake -S . -B build
cmake --build build -j
```

Server-only build (skips raylib/RmlUi FetchContent, much lighter/faster —
prefer this when not touching client code):

```
cmake -S . -B build -DTOPTRACK_BUILD_CLIENT=OFF
cmake --build build -j
```

Dependencies (asio, nlohmann/json, SQLiteCpp, and for the client build:
raylib, RmlUi) are pulled via CMake `FetchContent` — first configure clones
them, so it's slow once and cached under `build/` after.

## Run

```
./build/server/toptrack_server 7777          # port arg optional, defaults to 7777
./build/client/toptrack_client                # connects to a running server (once net wiring lands)
```

No test suite yet.

## Devlog rule

`DEVLOG.md` is a running plain-language log of what was built/changed and
why. **Whenever you add or meaningfully change something in this repo,
append an entry to `DEVLOG.md`** (don't rewrite history, just add on) so a
human can read it to understand the codebase without re-deriving it from
diffs.

## Commit rule

Work happens on feature branches, never directly on `main`. Start a new
branch per task/feature (don't keep piling unrelated work onto one
branch). Commits: one
line each, no body, no co-author trailer, formatted as Conventional
Commits — `<type>(<scope>): <subject>`. Types: `feat`, `fix`, `refacto`,
`docs`, `hotfix`. Split by feature/piece of work — never more than 5 files
in one commit; if a change touches more, split it into multiple
logically-scoped commits. Push the branch when done.

## Architecture

Three CMake targets, physics/protocol shared between the other two so
client and server can never disagree on car behavior or wire format:

- **`shared/`** (`toptrack_shared` static lib) — `include/toptrack/`:
  - `physics.hpp/.cpp`: `stepCar(state, input, tuning, dt)` — the *only*
    car physics implementation. Pure function, no engine/library
    dependency. Client uses it for local simulation; server uses the exact
    same function to re-simulate/validate submitted times and ghosts. Any
    physics tuning change here affects both automatically — never
    duplicate this logic in client or server code.
  - `track.hpp`: `Track`/`Tile`/`MedalTimes` — the tile-grid map format.
  - `protocol.hpp/.cpp`: wire message types (`RoundStart`, `TimeEntry`,
    `LeaderboardUpdate`, etc.) serialized via nlohmann/json
    (`NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE`). Wire framing is
    `[uint32 length][uint8 MessageType][json payload]` — see
    `server/src/net/server.cpp` for the read side of this framing.

- **`server/`** (`toptrack_server` executable) — one Asio `io_context`,
  one `Session` per connected client (`server/src/net/server.cpp`).
  - `db/` — SQLite via SQLiteCpp (`server/src/db/database.cpp`). Single
    embedded DB file, no Postgres/Redis — deliberate for a ~10-12 player
    self-hosted scale.
  - `tournament/round_manager.cpp` — owns the active round's standings,
    sorts on each submitted time. Server aggregates independently-run
    times into one leaderboard; it does not synchronize live racing
    between clients.
  - Realtime leaderboard push is a direct in-process broadcast over open
    Asio sessions (no external pub/sub) — round manager produces a
    `LeaderboardUpdate`, caller broadcasts it to sessions in that round.

- **`client/`** (`toptrack_client` executable, only built when
  `TOPTRACK_BUILD_CLIENT=ON`, the default):
  - `game/car_controller.cpp` — reads raylib input, calls the shared
    `stepCar`. Keep input-reading and physics-stepping separate so the
    controller stays testable without a window.
  - `editor/tile_editor.cpp` — grid-based tile placement, stub.
  - `ui/hud.cpp` — intended to bridge RmlUi to raylib. **No official RmlUi
    raylib backend exists** — a custom `RenderInterface`/`SystemInterface`
    must be written here (`client/CMakeLists.txt` sets `RMLUI_BACKEND=none`
    for this reason). Currently only a raylib `DrawText` placeholder.
  - `net/client.cpp` — Asio client, mirrors the server's message framing;
    connect/poll/disconnect are currently stubs.

Most `.cpp` files under `client/` and parts of `server/db`,
`server/net`/`tournament` wiring are intentionally-marked `// TODO` stubs
from initial scaffolding, not finished implementations — check for `TODO`
comments before assuming a piece is functional.
