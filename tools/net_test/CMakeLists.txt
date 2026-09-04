# Lightweight client<->server link check: links the real client net code
# but skips raylib/RmlUi entirely, so it builds fast and works even with
# -DTOPTRACK_BUILD_CLIENT=OFF.
add_executable(toptrack_net_test
  main.cpp
  ${CMAKE_SOURCE_DIR}/client/src/net/client.cpp
)

target_include_directories(toptrack_net_test PRIVATE
  ${CMAKE_SOURCE_DIR}/client/include
)

target_link_libraries(toptrack_net_test PRIVATE toptrack_shared asio)
