#ifndef __RPC_SERVER_H_
#define __RPC_SERVER_H_

#include <memory>
#include <thread>
#include "rpc-protocol.h"

namespace rpc {
  class Server {
   public:
    Server();

   private:
    struct Player {
      std::string name;
      int money;
    };

    struct State {
      rpc_protocol::Response ProcessRequest(const rpc_protocol::Request& request);

      std::shared_ptr<rpc_protocol::Server> server;

      std::mutex mutex;
      std::vector<Player> players;
      bool exit{false};
    };

    static
    void acceptFunc(std::shared_ptr<State> state);

    static
    void clientFunc(
      std::shared_ptr<State> state,
      std::shared_ptr<rpc_protocol::ServerTransport> transport);

    std::shared_ptr<State> state_;
  };
}

#endif
