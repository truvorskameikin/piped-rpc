#include <iostream>
#include "rpc-socket-transport.h"

#if defined(_WIN32)
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
namespace socketroutines {
  typedef SOCKET SocketType;
  typedef int AddressLenType;
  const SocketType kInvalidSocket = INVALID_SOCKET;
}
#endif

#if defined(__APPLE__) || defined(__ANDROID__) || defined(__gnu_linux__)
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
namespace socketroutines {
  typedef int SocketType;
  typedef socklen_t AddressLenType;
  const SocketType kInvalidSocket = -1;
}
#endif

namespace socketroutines {
  static
  bool InitSockets() {
#if defined(_WIN32)
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0)
        return true;
    return false;
#else
    signal(SIGPIPE, SIG_IGN);
#endif

    return true;
  }

  inline
  void CloseSocket(socketroutines::SocketType s)
  {
#if defined(_WIN32)
    closesocket(s);
#else
    close(s);
#endif
  }

  inline
  void MakeSocketNoblock(socketroutines::SocketType s) {
#if defined(_WIN32)
    unsigned long is_blocking = true;
    ioctlsocket(s, FIONBIO, &is_blocking);
#else
    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
  }

  inline
  bool SocketDroppedConnection() {
#if defined(_WIN32)
    int e = WSAGetLastError();
    return (e != WSAEWOULDBLOCK);
#else
    int e = errno;
    return (e != EAGAIN && e != EWOULDBLOCK);
#endif
  }

  inline
  void ShutdownSocket(socketroutines::SocketType s, bool send, bool receive) {
#if defined(_WIN32)
    if (send && receive)
      shutdown(s, SD_BOTH);
    else if (send)
      shutdown(s, SD_SEND);
    else if (receive)
      shutdown(s, SD_RECEIVE);
#else
    if (send && receive)
      shutdown(s, SHUT_RDWR);
    else if (send)
      shutdown(s, SHUT_WR);
    else if (receive)
      shutdown(s, SHUT_RD);
#endif
  }

  class SocketGuard {
   public:
    SocketGuard(SocketType socket) : socket_(socket) {
    }

    ~SocketGuard() {
      socketroutines::CloseSocket(socket_);
    }

    SocketType socket() {
      return socket_;
    }

   private:
    SocketType socket_;
  };
}

namespace rpc {
  namespace detail {
    class SocketClientTransportImpl : public rpc_protocol::ClientTransport {
     public:
      SocketClientTransportImpl(const std::string& server) : server_(server) {
      }

      rpc_protocol::Response MakeRequest(const rpc_protocol::Request& request) {
        rpc_protocol::Response result;

        socketroutines::InitSockets();

        struct addrinfo* addr_result;
        struct addrinfo addr_hints;

        memset(&addr_hints, 0, sizeof(addr_hints));
        addr_hints.ai_family = AF_INET;
        addr_hints.ai_socktype = SOCK_STREAM;
        addr_hints.ai_protocol = IPPROTO_TCP;

        if (getaddrinfo(server_.c_str(), "12345", &addr_hints, &addr_result) != 0)
          return result;

        socketroutines::SocketType sock = socketroutines::kInvalidSocket;

        std::string resolved_ip;
        for (struct addrinfo* addr = addr_result; addr != 0; addr = addr->ai_next) {
          sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
          if (sock == socketroutines::kInvalidSocket)
            continue;
          if (connect(sock, addr->ai_addr, addr->ai_addrlen) == -1) {
            socketroutines::CloseSocket(sock);
            sock = socketroutines::kInvalidSocket;
          } else {
            struct sockaddr_in* a = (struct sockaddr_in *) addr->ai_addr;
            resolved_ip = inet_ntoa(a->sin_addr);

            break;
          }
        }

        freeaddrinfo(addr_result);

        if (sock == socketroutines::kInvalidSocket)
          return result;

        std::vector<char> wire;
        request.Store(wire);

        if (!wire.empty()) {
          int32_t request_size = wire.size();
          send(sock, (const char*) &request_size, sizeof(int32_t), 0);
          send(sock, (const char*) &wire[0], wire.size(), 0);

          int32_t response_size = 0;
          recv(sock, (char *) &response_size, sizeof(int32_t), 0);

          if (response_size > 0) {
            std::vector<char> response_buffer(response_size);

            recv(sock, (char *) &response_buffer[0], response_buffer.size(), 0);
            result.Read(&response_buffer[0], response_buffer.size());
          }
        }

        socketroutines::CloseSocket(sock);

        return result;
      }

     private:
      std::string server_;
    };

    class SocketServerTransportImpl : public rpc_protocol::ServerTransport {
     public:
      SocketServerTransportImpl(socketroutines::SocketType sock) : sock_guard_(sock) {
      }

      rpc_protocol::Request Receive() {
        rpc_protocol::Request result;

        int32_t request_size = 0;
        recv(sock_guard_.socket(), (char *) &request_size, sizeof(int32_t), 0);

        if (request_size > 0) {
          std::vector<char> request_buffer(request_size);

          recv(sock_guard_.socket(), (char *) &request_buffer[0], request_buffer.size(), 0);
          result.Read(&request_buffer[0], request_buffer.size());
        }

        return result;
      }

      void Send(const rpc_protocol::Response& response) {
        std::vector<char> wire;
        response.Store(wire);

        int32_t response_size = wire.size();
        send(sock_guard_.socket(), (const char*) &response_size, sizeof(int32_t), 0);

        if (!wire.empty())
          send(sock_guard_.socket(), (const char*) &wire[0], wire.size(), 0);
      }

     private:
      socketroutines::SocketGuard sock_guard_;
    };

    class SocketServerImpl : public rpc_protocol::Server {
     public:
      SocketServerImpl() {
        socketroutines::InitSockets();

        sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        uint32_t addr = htonl(INADDR_ANY);

        struct hostent* he = gethostbyname("localhost");
        if (he) {
          struct in_addr** addr_list = (struct in_addr **) he->h_addr_list;
          for (int i = 0; ; ++i) {
            if (addr_list[i]) {
              addr = addr_list[i]->s_addr;
              break;
            }
          }
        }

        int reuseaddr = 1;
        setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuseaddr, sizeof(int));

  #if defined(__APPLE__)
        {
          int nosigpipe = 1;
          setsockopt(sock_, SOL_SOCKET, SO_NOSIGPIPE, (const char *) &nosigpipe, sizeof(int));
        }
  #endif

        sockaddr_in sock_addr;
        memset(&sock_addr, 0, sizeof(sockaddr_in));
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = htons(12345);
        sock_addr.sin_addr.s_addr = addr;

        if (bind(sock_, (sockaddr*) &sock_addr, sizeof(sockaddr_in)) != 0)
          return;

        listen(sock_, 128);
      }

      ~SocketServerImpl() {
        if (sock_ != socketroutines::kInvalidSocket)
          socketroutines::CloseSocket(sock_);
      }

      std::shared_ptr<rpc_protocol::ServerTransport> Accept() {
        socketroutines::AddressLenType size = sizeof(sockaddr_in);
        sockaddr_in sock_addr;

        socketroutines::SocketType client_sock = socketroutines::kInvalidSocket;
        while (true) {
          client_sock = accept(sock_, (struct sockaddr*) &sock_addr, &size);
          if (client_sock != socketroutines::kInvalidSocket)
            break;
        }

        return std::make_shared<SocketServerTransportImpl>(client_sock);
      }

     private:
      socketroutines::SocketType sock_;
    };
  }

  SocketClientTransport::SocketClientTransport(const std::string& server)
      : impl_(std::make_unique<detail::SocketClientTransportImpl>(server)) {
  }

  SocketServer::SocketServer()
      : impl_(std::make_unique<detail::SocketServerImpl>()) {
  }
}
