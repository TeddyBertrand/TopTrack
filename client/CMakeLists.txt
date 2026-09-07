include(FetchContent)
find_package(Threads REQUIRED)

# --- raylib ---
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

# --- RmlUi ---
# No official raylib backend exists; we write our own RenderInterface/
# SystemInterface against raylib in client/ui (TODO), so disable RmlUi's
# built-in backends/samples here.
set(RMLUI_BACKEND "none" CACHE STRING "" FORCE)
set(RMLUI_SAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  rmlui
  GIT_REPOSITORY https://github.com/mikke89/RmlUi.git
  GIT_TAG 6.1
)
FetchContent_MakeAvailable(rmlui)

add_executable(toptrack_client
  src/main.cpp
  src/game/car_controller.cpp
  src/editor/tile_editor.cpp
  src/ui/hud.cpp
  src/net/client.cpp
  src/net/session.cpp
)

target_include_directories(toptrack_client PRIVATE include)
target_link_libraries(toptrack_client PRIVATE
  toptrack_shared
  asio
  raylib
  RmlCore
  RmlDebugger
  Threads::Threads
)
