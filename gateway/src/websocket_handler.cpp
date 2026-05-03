#include "certosc/gateway/websocket_handler.h"
#include "certosc/common/logging.h"

namespace certosc {

WebsocketHandler::WebsocketHandler(std::shared_ptr<GrpcBridge> bridge)
    : bridge_(bridge) {}

void WebsocketHandler::accept(websocket::stream<tcp::socket> ws) {
    LOG_INFO("New WebSocket connection accepted (stub)");
    // In Phase 2/3: Start async read loop, register connection, forward gRPC events
}

void WebsocketHandler::broadcast(const std::string& message) {
    // In Phase 2/3: Iterate over all connected clients and async_write
}

} // namespace certosc
