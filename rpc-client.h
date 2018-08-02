#ifndef __RPC_CLIENT_H_
#define __RPC_CLIENT_H_

#include <memory>
#include <thread>
#include "rpc-protocol.h"

namespace rpc {
  class Client {
   public:
    Client(const std::string& server);

    int CreatePlayer(const std::string& name, int money);
    void AddMoneyToPlayer(int player_id, int money);
    int GetPlayerMoney(int player_id);
    int32_t SumMoneyForAllPlayersAsync(std::function<void(int32_t, int)> result_callback);

   private:
    void incId() {
      ++id_;
      if (id_ < 0)
        id_ = 1;
    }

    int32_t id_{1};
    std::shared_ptr<rpc_protocol::ClientTransport> client_;
  };
}

#endif
