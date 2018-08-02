#if defined(_WIN32)

#include <windows.h>
#include "rpc-pipe-transport.h"

namespace rpc {
  namespace detail {
    class PipeClientTransportImpl : public rpc_protocol::ClientTransport {
     public:
      PipeClientTransportImpl(const std::string& server) : server_(server) {
      }

      rpc_protocol::Response MakeRequest(const rpc_protocol::Request& request) {
        rpc_protocol::Response result;

        std::vector<char> wire;
        request.Store(wire);

        if (!wire.empty()) {
          HANDLE pipe = CreateFile(
            "\\\\.\\pipe\\piped-rpc",
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

          DWORD dwMode = PIPE_READMODE_MESSAGE;
          SetNamedPipeHandleState(pipe, &dwMode, NULL, NULL);

          int32_t request_size = (int32_t) wire.size();

          DWORD written = 0;
          WriteFile(
            pipe,
            (const char*) &request_size, sizeof(int32_t),
            &written,
            NULL);

          WriteFile(
            pipe,
            (const char*) &wire[0], wire.size(),
            &written,
            NULL);

          int32_t response_size = 0;

          DWORD bytes_read = 0;
          ReadFile(
            pipe,
            &response_size, sizeof(int32_t),
            &bytes_read,
            NULL);

          if (response_size > 0) {
            std::vector<char> response_buffer(response_size);

            ReadFile(
              pipe,
              &response_buffer[0], response_buffer.size(),
              &bytes_read,
              NULL);

            result.Read(&response_buffer[0], response_buffer.size());
          }
        }

        return result;
      }

     private:
      std::string server_;
    };

    class PipeServerTransportImpl : public rpc_protocol::ServerTransport {
     public:
      PipeServerTransportImpl(HANDLE pipe): pipe_(pipe) {
      }

      ~PipeServerTransportImpl() {
        if (pipe_ != INVALID_HANDLE_VALUE) {
          FlushFileBuffers(pipe_);
          DisconnectNamedPipe(pipe_);
          CloseHandle(pipe_);
        }
      }

      rpc_protocol::Request Receive() {
        rpc_protocol::Request result;

        int32_t request_size = 0;

        DWORD bytes_read = 0;
        ReadFile(
          pipe_,
          (char *) &request_size, sizeof(int32_t),
          &bytes_read,
          NULL);

        if (request_size > 0) {
          std::vector<char> request_buffer(request_size);

          ReadFile(
            pipe_,
            (char *) &request_buffer[0], request_buffer.size(),
            &bytes_read,
            NULL);

          result.Read(&request_buffer[0], request_buffer.size());
        }

        return result;
      }

      void Send(const rpc_protocol::Response& response) {
        std::vector<char> wire;
        response.Store(wire);

        int32_t response_size = wire.size();

        if (response_size > 0) {
          DWORD written = 0;
          WriteFile(
            pipe_,
            (const char*) &response_size, sizeof(int32_t),
            &written,
            NULL);

          WriteFile(
            pipe_,
            (const char*) &wire[0], wire.size(),
            &written,
            NULL);
        }
      }

     private:
      HANDLE pipe_{INVALID_HANDLE_VALUE};
    };

    class PipeServerImpl : public rpc_protocol::Server {
     public:
      PipeServerImpl() {
      }

      ~PipeServerImpl() {
      }

      std::shared_ptr<rpc_protocol::ServerTransport> Accept() {
        HANDLE pipe = INVALID_HANDLE_VALUE;
        while (true) {
          pipe = CreateNamedPipe(
            "\\\\.\\pipe\\piped-rpc",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            1024,
            1024,
            0,
            NULL);

          if (pipe == INVALID_HANDLE_VALUE)
            continue;

          BOOL connected = ConnectNamedPipe(pipe, NULL) ?  TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
          if (connected)
            break;
        }

        return std::make_shared<PipeServerTransportImpl>(pipe);
      }
    };
  }

  PipeClientTransport::PipeClientTransport(const std::string& server)
      : impl_(std::make_unique<detail::PipeClientTransportImpl>(server)) {
  }

  PipeServer::PipeServer()
      : impl_(std::make_unique<detail::PipeServerImpl>()) {
  }
}

#endif
