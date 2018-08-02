#include <assert.h>
#include <iostream>
#include <thread>
#include "rpc-protocol.h"
#include "rpc-socket-transport.h"

int main(int argc, char* argv[]) {
  std::cout << "Testing RPC: " << std::endl;

  {
    rpc_protocol::Request request;
    request.set_id(123456789);
    request.set_method("Factory.createObject");
    request.add_arg(rpc_protocol::Value(42));
    request.add_arg(rpc_protocol::Value(std::string("abc")));
    request.add_arg(rpc_protocol::Value(123));
    request.add_arg(rpc_protocol::Value(std::string("abc")));

    std::vector<char> wire;

    request.Store(wire);

    rpc_protocol::Request request_check;
    request_check.Read(&wire[0], wire.size());

    assert(request.id() == request_check.id());
    assert(request.method() == request_check.method());
    assert(request.args() == request_check.args());
  }

  {
    rpc_protocol::Response response;
    response.set_id(123456789);
    response.set_result(rpc_protocol::Value(100));

    std::vector<char> wire;

    response.Store(wire);

    rpc_protocol::Response response_check;
    response_check.Read(&wire[0], wire.size());

    assert(response.id() == response_check.id());
    assert(response.result() == response_check.result());
  }

  {
    auto server_working_func = [](std::shared_ptr<rpc_protocol::ServerTransport> transport) {
      rpc_protocol::Request request = transport->Receive();

      rpc_protocol::Response response;
      response.set_id(request.id());
      response.set_result(rpc_protocol::Value(request.method()));

      transport->Send(response);
    };

    auto server_func = [server_working_func]() {
      rpc::SocketServer server;

      int connections = 2;
      while (true) {
        std::shared_ptr<rpc_protocol::ServerTransport> client = server.Accept();

        std::thread working_thread(server_working_func, client);
        working_thread.detach();

        --connections;
        if (connections < 0)
          break;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    };

    std::thread server_thread(server_func);
    server_thread.detach();

    rpc::SocketClientTransport client_transport("localhost");

    rpc_protocol::Request request;
    request.set_id(100);
    request.set_method("ping");

    rpc_protocol::Response response = client_transport.MakeRequest(request);

    assert(response.id() == 100);
    assert(response.result().string_value() == "ping");

    request.set_id(200);
    request.set_method("pong");

    response = client_transport.MakeRequest(request);

    assert(response.id() == 200);
    assert(response.result().string_value() == "pong");
  }

  std::cout << "Ok" << std::endl;

  return 0;
}
