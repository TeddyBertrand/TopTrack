# TopTrack

2D top-down Trackmania-like game — drift/gear/slide handling, tile-based
map editor, time-attack medals, COTD-style tournaments for a small
self-hosted group (~10-12 players). No game engine: hand-written C++
client (raylib + RmlUi) and server (Asio + SQLite).

See `docs/plan.md`-equivalent design notes in the project plan for full
feature list, stack rationale, and architecture.

## Build

```
cmake -S . -B build
cmake --build build -j
```

Server only (skip raylib/RmlUi client deps):

```
cmake -S . -B build -DTOPTRACK_BUILD_CLIENT=OFF
cmake --build build -j
```

## Run

```
./build/server/toptrack_server 7777
./build/client/toptrack_client   # in another terminal, once implemented
```

## Layout

- `shared/` — car physics model + wire protocol, linked into both client
  and server so behavior can never drift between them.
- `client/` — raylib rendering/input, RmlUi UI (HUD/menus/editor panels),
  tile editor, Asio net client.
- `server/` — Asio TCP server, SQLite persistence, tournament round
  management, leaderboard broadcast.
