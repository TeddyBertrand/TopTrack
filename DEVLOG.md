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

## DB persistence implemented (was TODO stub)

`server/db/database.cpp`'s four methods were no-ops — `recordTime` was
already being called from the `SubmitTime` path but silently dropped
every write. Implemented for real against SQLiteCpp:
`saveTrack`/`loadTrack` (upsert/select on `tracks`), `recordTime`/
`bestTimesForTrack` (insert/select-ordered on `times`).

Needed JSON (de)serialization for `Track`/`Tile` (didn't exist yet) and
for a bare `vector<GhostFrame>` (existed only bundled inside `TimeEntry`).
Added `toptrack::serialize(Track)`/`deserializeTrack` +
`serializeTiles`/`deserializeTiles` (new `shared/src/track.cpp`), and
`toptrack::protocol::serializeGhost`/`deserializeGhost`, mirroring the
existing `protocol.cpp` pattern (`NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE` in
the `.cpp`, plain function declarations in the header) rather than
leaking nlohmann macros into headers. `TileType` uses
`NLOHMANN_JSON_SERIALIZE_ENUM` for stable string values.

Verified end-to-end: `toptrack_net_test` against a running server, then
read `toptrack.db` directly (`sqlite3`/`python3 sqlite3`) — the submitted
time landed in the `times` table with correct `track_id`/`player_name`/
`time_ms`.

## TrackUpload/TrackDownload dispatch wired (was undispatched enum values)

`server/net/server.cpp`'s `dispatch()` only handled `SubmitTime` —
`TrackUpload`/`TrackDownload` existed in the `MessageType` enum but fell
through to the "unhandled message type" log line, so `saveTrack`/
`loadTrack` (previous entry) had no caller. Wired both:
- `TrackUpload`: deserialize `Track`, `hub_.database.saveTrack(track)`.
- `TrackDownload`: deserialize a new `protocol::TrackRequest{trackId}`,
  `loadTrack`, and reply with `MessageType::TrackUpload` carrying the
  serialized `Track` — reusing that type rather than adding a fourth
  message type, since the payload shape is identical either direction.
  An empty `Track.id` in the reply means not-found (default-constructed
  `Track{}` on `loadTrack`'s `nullopt`).

Added `client::net::Client::uploadTrack()`/`requestTrack()` (mirrors
`sendTimeEntry()`; factored the three into a shared `sendFramed()` helper
in `client.cpp` instead of duplicating the framing code a third time).
Extended `tools/net_test` to upload a small 2-tile track then download it
back and print what came back — this is what verified the round-trip.

Verified end-to-end: `toptrack_net_test` now does time-submit +
leaderboard-read (existing) followed by track-upload +
track-download-readback in one run against a live server — all four
steps succeeded, downloaded track matched what was uploaded
(id/name/tile count).

**Known gaps / next likely steps**: RmlUi/raylib render bridge, tile
editor logic (now has somewhere real to persist to), ghost playback
(recording is done, see below), real client build verification (blocked
in this sandbox by missing X11 dev libs), `TrackDownload` request
currently has no timeout/retry — a not-found reply is indistinguishable
from a slow server until the client checks `Track.id`, and the server
still doesn't re-simulate submitted ghosts through `stepCar` to validate
them (it trusts whatever `timeMs`/ghost the client sends).

## Medal computation wired (was noted in RoundManager's own docstring as unimplemented)

Added `toptrack::Medal` enum (`None`/`Bronze`/`Silver`/`Gold`) and
`medalForTime(timeMs, MedalTimes)` to `shared/track.{hpp,cpp}` — slowest
qualifying medal, gold's threshold checked first since gold is the
fastest/tightest cutoff. `TimeEntry` gained a `medal` field (defaults
`None`, wire-serialized) so a submitted time's medal travels with it in
`LeaderboardUpdate`.

`RoundManager::startRound` now takes a `MedalTimes` (defaulted to
all-zero/no medals for callers that don't care yet); `submitTime` scores
each entry against it before sorting into standings. `server.cpp`'s
round-1/track-1 bootstrap now passes real-ish placeholder thresholds
(60000/45000/30000 ms) — genuine gap: this should come from
`hub.database.loadTrack("track-1")` once real round scheduling exists,
not a hardcoded literal; left a `TODO` at the call site.

Verified: `toptrack_net_test`'s 12345ms fake time now reports
`medal=3` (Gold) in the leaderboard printout, matching the bootstrap
thresholds.

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

## Ghost recording wired into CarController

`CarController` now records a `toptrack::protocol::GhostFrame` (t/x/y/
headingRad) every `update()` call into a `ghost_` vector, exposed via
`ghost()`; `resetRun()` clears it (and elapsed time) to start a fresh
run. `client/src/main.cpp`'s `T`-to-submit path now fills
`TimeEntry::ghost` from `car.ghost()` instead of leaving it empty, and
calls `resetRun()` after submitting so the next run starts clean.

Deliberately reuses `toptrack::protocol::GhostFrame` directly rather than
a separate game-layer type — client/server already share this struct
over the wire and `CarController` sat downstream of `toptrack::` either
way (it already depended on `physics.hpp`).

Verified with `g++ -fsyntax-only` against the real raylib/asio/json
headers in `build/_deps/` (same constraint as the net loop change — no
X11 in this sandbox), plus a full `cmake --build` of the
`-DTOPTRACK_BUILD_CLIENT=OFF` targets to confirm nothing else broke.
Ghost *playback* (rendering another run's recorded path) is still open.

## Hello handshake wired (was an unhandled enum value)

`MessageType::Hello` existed but nothing sent or dispatched it. Added
`protocol::HelloRequest{playerName}`; server's `dispatch()` now handles
`Hello` by replying with the active round's `RoundStart` (roundId/
trackId/durationSeconds, sourced from new `RoundManager` getters —
`currentRoundId()`/`currentTrackId()`/`currentDurationSeconds()`) then a
`LeaderboardUpdate` seeded from `hub_.database.bestTimesForTrack(trackId)`
— this is `bestTimesForTrack`'s first real caller (previous entry's DB
work had no caller for it yet).

`client::net::Client::sendHello()` added; `NetSession::connect()` now
takes a `playerName` and sends Hello right after connecting, and the
background receive loop stashes the `RoundStart` reply the same way it
already stashed `LeaderboardUpdate` (`takeRoundStart()`, mirroring
`takeLeaderboard()`). `main.cpp` displays the current round/track under
the connection status line. `tools/net_test` sends Hello up front too and
prints both replies before doing its existing time-submit +
track-upload/download checks.

Verified end-to-end: fresh `toptrack.db`, `toptrack_net_test` against a
live server — got back `round=round-1 track=track-1
durationSeconds=180` then `initial leaderboard ... standings=0` (empty,
correct for a freshly-migrated DB), then the rest of the existing
net_test flow ran unchanged.

## Tile editor implemented (was a pure TODO stub)

`client/editor/tile_editor.{hpp,cpp}` now does real grid placement:
mouse position → grid cell (`kCellSizePx = 32`), left-click places the
selected `TileType` (or overwrites the type of whatever's already
there), right-click removes, `R` rotates the tile under the cursor by
90°, number keys 1-6 change the selected type (Checkpoint tiles get an
auto-incrementing `checkpointOrder`). `draw()` renders a grid, each
tile as a colored rect keyed by type, a rotation tick mark, and a cursor
highlight. RmlUi toolbar for type selection is still TODO — keyboard
only for now.

Wired into `client/src/main.cpp`: `E` toggles editor mode (pauses the
car/race HUD, shows the editor canvas instead); `U` in editor mode calls
the new `NetSession::uploadTrack()` (mirrors `submitTime()`) to persist
the in-progress track as `track-1` via the `TrackUpload` path from the
earlier entry.

Verified with `g++ -fsyntax-only` against the real raylib headers (same
X11-sandbox constraint as other client changes) and a full `cmake
--build` of the server-only targets to confirm the shared/net changes
(`Track` reused as the editor's model, `NetSession::uploadTrack`) didn't
break anything. No in-window smoke test — still blocked on X11 dev libs
in this sandbox.

## Ghost playback (was the other open half of the ghost-recording gap)

Added `client::game::GhostPlayer` (`ghost_player.{hpp,cpp}`): loads a
`vector<GhostFrame>` (sorted by `t`), `sampleAt(tSeconds)` linearly
interpolates x/y/headingRad between the two frames straddling
`tSeconds`, returning `nullopt` before the first frame or after the
last. Pure interpolation, no physics — the ghost is just replaying
already-recorded positions.

Wired into `main.cpp`: whenever a `LeaderboardUpdate` arrives, the
best (`standings.front()`, since standings are sorted best-first) time's
ghost is loaded — this is the same `ghost` field `bestTimesForTrack`
already populated (see the Hello-handshake entry), just consumed on the
client now. During racing, `ghostPlayer.sampleAt(raceElapsedSeconds)`
draws the best run's position as a hollow sky-blue circle alongside the
live car — `raceElapsedSeconds` lines up with the ghost's own `t` since
both are seconds-since-run-start with the same reset-on-submit
convention (`CarController::resetRun()`).

Verified interpolation logic with a standalone throwaway harness
(`GhostPlayer` against a synthetic 6-frame, 10-units/sec ghost): midpoint
sample interpolated correctly (t=2.5 → x=25), the last frame sampled
exactly (t=5 → x=50), and out-of-range samples on both ends correctly
returned `nullopt`. Full `cmake --build` of server-only targets stayed
green; in-window rendering still unverified (X11-sandbox constraint,
as elsewhere in this log).

## Round expiry (durationSeconds was stored but never checked)

`RoundManager` stored `durationSeconds` since the round-scoring entry but
nothing ever compared it against elapsed time — a round never actually
ended. Added `startTime_` (`steady_clock::time_point`, set in
`startRound`) and `hasExpired()`: true once `durationSeconds` has
elapsed since start, and closes the round (`roundActive_ = false`) as a
side effect so callers don't need a separate close call.
`submitTime()` now calls `hasExpired()` first — a submission arriving
after expiry is dropped (not scored, not added to standings) and just
returns the leaderboard as it already stood.

`server.cpp`'s `SubmitTime` dispatch is unchanged — `recordTime()` still
persists every submission to the DB regardless of round state (that's
storage, not live standings), only `roundManager.submitTime()`'s
in-memory scoring respects expiry.

Verified with a standalone throwaway harness (`RoundManager` given a
0.2s round): immediately after `startRound`, `hasExpired()` is false and
a submission scores normally (1 standing); after sleeping 300ms,
`hasExpired()` flips true and `isRoundActive()` false; a second
submission after that point is correctly dropped (standings count stays
at 1, not 2). Full `cmake --build` of server-only targets stayed green.

## Round rotation (closes the gap the previous entry flagged)

`server/net/server.cpp`'s `Hub` gained `trackId`/`roundDurationSeconds`/
`medals`/`roundCounter` fields and `startNextRound()` (starts
`"round-" + counter++` on the same bootstrap track/duration/medals —
still not per-track medal thresholds or admin-triggered track selection,
just automatic rotation so a round doesn't die forever once it expires).
`Server::run()` now runs an `asio::steady_timer` polling every 5s
(cheap next to a 180s+ round): if `roundManager.hasExpired()`, it calls
`startNextRound()` and broadcasts the new `RoundStart` to every connected
session, so clients pick up the new roundId without resending Hello.

Verified live end-to-end rather than just unit-style: temporarily
patched `roundDurationSeconds` to 3.0 (uncommitted, reverted
immediately after), rebuilt, ran the real server binary for ~11s under
`stdbuf -oL` (needed — SIGTERM otherwise drops buffered stdout before
flush) — log showed `round rotated: round-2` then `round rotated:
round-3`, confirming the timer, `hasExpired()`, and the broadcast path
all work together. Reverted the patch, confirmed via `git diff` it's
back to 180.0, and did a final clean rebuild.

**Known gaps / next likely steps**: still no admin control or per-round
track selection — every round rotates onto the same hardcoded `track-1`
with the same hardcoded medal thresholds; real round scheduling (varying
tracks, medals sourced from `hub.database.loadTrack`) is still open.

## Submitted-time plausibility check (partial answer to the stepCar-validation gap)

Added `toptrack::protocol::isTimeEntryPlausible(entry, tuning)` to
`shared/protocol.{hpp,cpp}`. This is **not** the full `stepCar`
re-simulation the architecture doc describes — that needs the original
per-frame `CarInput`, which `GhostFrame` doesn't carry (only position/
heading), and the client doesn't record inputs anywhere yet. Instead it's
a cheaper sanity check: reported `timeMs` must match the ghost's own
last-frame `t` within 500ms, frame timestamps must be strictly
increasing, and no consecutive frame pair may imply a speed exceeding
`tuning.maxSpeed[5]` (top gear) by more than a 1.5x rounding-slack
margin — catches empty ghosts and position teleports, not subtler
input-level cheating.

Wired into `server/net/server.cpp`'s `SubmitTime` dispatch: on failure it
logs a warning (`player=... trackId=... timeMs=...`) but still records
the time — there's no reject/error response `MessageType` yet, so
rejecting outright would silently strand the client waiting on a
response that never comes.

Verified with a standalone throwaway harness (linked directly against
`shared/src/{protocol,track,physics}.cpp`, not committed) exercising
three cases: empty ghost → false, a smooth straight-line 2s run →
true, the same run with one frame teleported → false. Also rebuilt +
reran `toptrack_net_test`'s server-only path to confirm nothing
regressed (net_test's fake time has an empty ghost, so it's now expected
to log the warning — didn't verify the log line landed before the
process was killed for cleanup, but the unit-style check above covers
the actual logic).
