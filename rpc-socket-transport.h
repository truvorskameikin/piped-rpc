#ifndef __RPC_SOCKET_TRANSPORT_H_
#define __RPC_SOCKET_TRANSPORT_H_

#include <memory>
#include "rpc-protocol.h"

namespace rpc {
  class SocketClientTransport : public rpc_protocol::ClientTransport {
   public:
    SocketClientTransport(const std::string& server);

    rpc_protocol::Response MakeRequest(const rpc_protocol::Request& request) {
      return impl_->MakeRequest(request);
    }

   private:
    std::unique_ptr<rpc_protocol::ClientTransport> impl_;
  };

  class SocketServer : public rpc_protocol::Server {
   public:
    SocketServer();

    std::shared_ptr<rpc_protocol::ServerTransport> Accept() {
      return impl_->Accept();
    }

   private:
    std::unique_ptr<rpc_protocol::Server> impl_;
  };
}

#endif
