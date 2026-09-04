#include <cstdlib>
#include <iostream>

#include "server/net/server.hpp"

int main(int argc, char **argv) {
  uint16_t port = 7777;
  if (argc > 1) port = static_cast<uint16_t>(std::atoi(argv[1]));

  try {
    toptrack::server::net::Server server(port);
    server.run();
  } catch (const std::exception &e) {
    std::cerr << "server error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
