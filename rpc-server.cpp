#include <thread>
#include "rpc-socket-transport.h"
#include "rpc-server.h"

namespace rpc {
  rpc_protocol::Response Server::State::ProcessRequest(const rpc_protocol::Request& request) {
    rpc_protocol::Response result;

    if (request.method() == "Player.Create" ) {
      std::lock_guard<std::mutex> lock(mutex);

      int32_t player_id = players.size();

      players.push_back(Player());
      if (request.args().size() > 0)
        players.back().name = request.args()[0].string_value();
      if (request.args().size() > 1)
        players.back().money = request.args()[1].int32_value();

      result.set_result(rpc_protocol::Value(player_id));

    } else if (request.method() == "Player.AddMoney") {
      std::lock_guard<std::mutex> lock(mutex);

      int32_t money = 0;
      if (request.args().size() > 0) {
        int32_t player_id = request.args()[0].int32_value();
        if (player_id >= 0 && player_id < (int) players.size()) {
          money = players[player_id].money;

          if (request.args().size() > 1) {
            players[player_id].money += request.args()[1].int32_value();
            money = players[player_id].money;
          }
        }
      }

      result.set_result(rpc_protocol::Value(money));

    } else if (request.method() == "Player.GetMoney") {
      std::lock_guard<std::mutex> lock(mutex);

      int32_t money = -1;
      if (request.args().size() > 0) {
        int32_t player_id = request.args()[0].int32_value();
        if (player_id >= 0 && player_id < (int) players.size())
          money = players[player_id].money;
      }

      result.set_result(rpc_protocol::Value(money));

    } else if (request.method() == "Player.SumMoney") {
      std::vector<Player> players_copy;

      {
        std::lock_guard<std::mutex> lock(mutex);
        players_copy = players;
      }

      int32_t money = 0;
      for (size_t i = 0; i < players_copy.size(); ++i) {
        money += players_copy[i].money;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }

      result.set_result(rpc_protocol::Value(money));
    }

    return result;
  }

  Server::Server() {
    state_ = std::make_shared<State>();
    state_->server = std::make_shared<rpc::SocketServer>();

    std::thread server_thread(acceptFunc, state_);
    server_thread.detach();
  }

  void Server::acceptFunc(std::shared_ptr<State> state) {
    while (true) {
      std::shared_ptr<rpc_protocol::ServerTransport> transport = state->server->Accept();

      std::thread client_thread(clientFunc, state, transport);
      client_thread.detach();
    }
  }

  void Server::clientFunc(
    std::shared_ptr<State> state,
    std::shared_ptr<rpc_protocol::ServerTransport> transport) {

    rpc_protocol::Request request = transport->Receive();

    rpc_protocol::Response response = state->ProcessRequest(request);
    response.set_id(request.id());

    transport->Send(response);
  }
}
