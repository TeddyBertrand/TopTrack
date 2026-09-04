add_executable(toptrack_server
  src/main.cpp
  src/net/server.cpp
  src/db/database.cpp
  src/tournament/round_manager.cpp
)

target_include_directories(toptrack_server PRIVATE include)
target_link_libraries(toptrack_server PRIVATE
  toptrack_shared
  asio
  SQLiteCpp
)

find_package(Threads REQUIRED)
target_link_libraries(toptrack_server PRIVATE Threads::Threads)
