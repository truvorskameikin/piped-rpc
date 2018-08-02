#include "rpc-socket-transport.h"
#include "rpc-client.h"

namespace rpc {
  Client::Client(const std::string& server) {
    client_ = std::make_shared<rpc::SocketClientTransport>(server);
  }

  int Client::CreatePlayer(const std::string& name, int money) {
    incId();

    rpc_protocol::Request request;
    request.set_id(id_);
    request.set_method("Player.Create");
    request.add_arg(rpc_protocol::Value(name));
    request.add_arg(rpc_protocol::Value(money));

    rpc_protocol::Response response = client_->MakeRequest(request);
    if (response.id() != request.id())
      return -1;

    return response.result().int32_value();
  }

  void Client::AddMoneyToPlayer(int player_id, int money) {
    incId();

    rpc_protocol::Request request;
    request.set_id(id_);
    request.set_method("Player.AddMoney");
    request.add_arg(rpc_protocol::Value(player_id));
    request.add_arg(rpc_protocol::Value(money));

    client_->MakeRequest(request);
  }

  int Client::GetPlayerMoney(int player_id) {
    incId();

    rpc_protocol::Request request;
    request.set_id(id_);
    request.set_method("Player.GetMoney");
    request.add_arg(rpc_protocol::Value(player_id));

    rpc_protocol::Response response = client_->MakeRequest(request);
    if (response.id() != request.id())
      return -1;

    return response.result().int32_value();
  }

  int32_t Client::SumMoneyForAllPlayersAsync(std::function<void(int32_t, int)> result_callback) {
    incId();

    int32_t id = id_;

    auto work_func = [id](
      std::shared_ptr<rpc_protocol::ClientTransport> client,
      std::function<void(int32_t, int)> result_callback) {

      rpc_protocol::Request request;
      request.set_id(id);
      request.set_method("Player.SumMoney");

      int money = -1;

      rpc_protocol::Response response = client->MakeRequest(request);
      if (response.id() == id)
        money = (int) response.result().int32_value();

      result_callback(id, money);
    };

    std::thread work_thread(work_func, client_, result_callback);
    work_thread.detach();

    return id;
  }
}
