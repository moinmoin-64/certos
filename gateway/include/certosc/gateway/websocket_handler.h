#pragma once

#include "certosc/gateway/grpc_bridge.h"

#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

namespace certosc {

namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;

class WebsocketHandler {
public:
    WebsocketHandler(std::shared_ptr<GrpcBridge> bridge);
    
    // Accept an upgraded connection
    void accept(websocket::stream<tcp::socket> ws);

    // Broadcast message to all connected clients
    void broadcast(const std::string& message);

private:
    std::shared_ptr<GrpcBridge> bridge_;
    std::mutex mutex_;
    
    // Simplistic handling for Phase 1
    // We would need a thread pool or async loop for proper WS handling
};

} // namespace certosc
