add_library(toptrack_shared STATIC
  src/physics.cpp
  src/protocol.cpp
)

target_include_directories(toptrack_shared PUBLIC include)
target_link_libraries(toptrack_shared PUBLIC nlohmann_json::nlohmann_json)
