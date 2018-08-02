#ifndef __RPC_PIPE_TRANSPORT_
#define __RPC_PIPE_TRANSPORT_

#include <memory>
#include "rpc-protocol.h"

namespace rpc {
  class PipeClientTransport : public rpc_protocol::ClientTransport {
   public:
    PipeClientTransport(const std::string& server);

    rpc_protocol::Response MakeRequest(const rpc_protocol::Request& request) {
      return impl_->MakeRequest(request);
    }

   private:
    std::unique_ptr<rpc_protocol::ClientTransport> impl_;
  };

  class PipeServer : public rpc_protocol::Server {
   public:
    PipeServer();

    std::shared_ptr<rpc_protocol::ServerTransport> Accept() {
      return impl_->Accept();
    }

   private:
    std::unique_ptr<rpc_protocol::Server> impl_;
  };
}

#endif
