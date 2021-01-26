#include <iostream>

#include "logger.h"
#include "server.h"
#include "utility.h"

int main() {
  std::unique_ptr<Server> server = std::make_unique<Server>();
  server->Start();

  while (true) {
    std::string command;
    std::cin >> command;
    if (command == "exit" or command == "q") {
      server->Stop();
      server.reset();
      Sleep(1000);
      break;
    }
  }

  Log::Print(Log::Level::INFO, "Server Shutdown...");
  return 0;
}