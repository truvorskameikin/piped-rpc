#include <iostream>
#include <thread>
#include "rpc-server.h"

int main(int argc, char* argv[]) {
  rpc::Server server;

  std::cout << "Server started" << std::endl;
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return 0;
}