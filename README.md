# piped-rpc

RPC client and server with named pipe transport (socket transport is also supported)

## Design

The task was to build RPC client and server that can interact with each other with Windows named pipes, client can make sync and async calls to server, client can create/modify/retrive objects and properties from server, server can receive sync and async calls from client. The number of supported types can be limited to support only strings and integers. The requirment to build the only transport with named pipes was a little awkward for me, since the last (and the only) time I used Windows named pipes was more than 10 years ago, I have not used Windows machine for almost 4 years. So I decided to build socket transport at first, debug RPC stuff and then build pipe transport. But first things first. There are a lot of RPC implementations and a lot of RPC protocols, gRPC, Json-RPC to name a few. For the real world task one should consider looking at existing applications/protocols and then make decision on building an RPC system. For this task I decided to build everything from scratch: protocol, transport, client and server. No dependencies, no frameworks. Then I decided to do async calls only on client and for every request create a new connection. This means that there is no need to implement multiplexing in protocol. I decided to build binary protocol without endianness support - it is easy to implement and the endianness support can be added lately. I decided to build two transport implementations with sockets and windows named pipes, the socket implementation can be used on Windows/macOS/Linux and the pipe implementation can be used on Windows. Async calls on client should use std::thread. Server should use one thread for accepting connections and one thread for every connection accepted.

## Implementation

There is a common library that is used by client, server and the testing application. This common library implements protocol, client and server. Client and server applications just use client and server class from common library.

### Protocol

Protocol is implemented in files rpc-protocol.h and rpc-protocol.cpp. Also these files declare interfaces for transport, client and server. Every entity on wire is preceded by size field indicating the size of the entity, size is int32_t. Protocol defines Request and Response classes that can be saved to wire (a vector of bytes) and restored from wire.

### Transport

Transport with sockets is implemented with files rpc-socket-transport.h and rpc-socket-transport.cpp. Transport with pipes is implemented with files rpc-pipe-transport.h and rpc-pipe-transport.cpp. To send request or response transport implementations first send size of request or response and then send payload. The same is on the other end, first read size then read payload. Very straightforward!

### Server

Server is implemented with files rpc-server.h and rpc-server.cpp. Server creates one thread to accept connections and one thread per accepted connection. Threads are created in detached mode. After sending response the connection thread exits.

### Client

Client is implemented with files rpc-client.h and rpc-client.cpp. Client class implements functions to call on server:

* `int CreatePlayer(const std::string& name, int money)` - creates player on server with given name and with initial amount of money, returns player id
* `void AddMoneyToPlayer(int player_id, int money)` - adds money to player by id
* `int GetPlayerMoney(int player_id)` - returns player money by id
* `int32_t SumMoneyForAllPlayersAsync(std::function<void(int32_t, int)> result_callback)` - calculates sum of money of all players, this function runs in seperate thread, returns the request id

### Client, server and testing applications

These applications are implemented with files rpc-client-main.cpp, rpc-server-main.cpp and rpc-test.cpp. Testing is done with `assert` function, protocol and socket transport are tested. Client uses interactive mode to fire tasks to server, the result of async task is simply written to console from separate thread without any synchronization.

## Supported platforms

The main development platform was macOS and this RPC implementation is tested to work on it. It is tested on Windows with Visual Studio 2017 compiler.

## Building

All executable are built with CMake. Just `cmake .. && make` on macOS, `cmake ..` and open in Visual Studio to build.

## Known problems

This task was finished in 7 hours and has problems. First you will notice that there is no error handling in processing user input in client interactive mode. Then you can't choose the name of pipe, binding address, server addrees, port number when starting these applications. I thought that I can use `const std::string& server` variable to do this, but have not implemented it. The protocol can support translating from wire endianness to machines endianness. The envelop class/struct can be implemented to avoid sending payload size in transport implementation.

## Builds for Windows with socket transport (Visual Studio 2017):

[Client](https://www.dropbox.com/s/faikemrwjoxtwza/rpc-client.exe?dl=1)

[Server](https://www.dropbox.com/s/b47r2dwzy244s8b/rpc-server.exe?dl=1)

## Builds for Windows with pipe transport (Visual Studio 2017):

[Client](https://www.dropbox.com/s/kiyxxghd40dagz9/rpc-client.exe?dl=1)

[Server](https://www.dropbox.com/s/m0zrvop4bdpj3vb/rpc-server.exe?dl=1)
