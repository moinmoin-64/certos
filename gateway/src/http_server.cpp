#include "certosc/gateway/http_server.h"
#include "certosc/gateway/rest_routes.h"
#include "certosc/common/logging.h"

#include <boost/beast/websocket.hpp>
#include <iostream>

namespace certosc {

HttpServer::HttpServer(const GatewayConfig& config, std::shared_ptr<GrpcBridge> bridge)
    : config_(config), bridge_(bridge) 
{
    net::ip::address addr = net::ip::make_address(config_.bind_address);
    tcp::endpoint endpoint{addr, config_.http_port};
    
    acceptor_ = std::make_unique<tcp::acceptor>(ioc_, endpoint);
    LOG_INFO("Gateway HTTP server listening on {}:{}", config_.bind_address, config_.http_port);
}

void HttpServer::start() {
    do_accept();
    thread_ = std::thread([this]() {
        ioc_.run();
    });
}

void HttpServer::stop() {
    ioc_.stop();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void HttpServer::do_accept() {
    acceptor_->async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto socket_ptr = std::make_shared<tcp::socket>(std::move(socket));
                auto buffer = std::make_shared<beast::flat_buffer>();
                auto parser = std::make_shared<http::request_parser<http::string_body>>();
                parser->body_limit(100 * 1024 * 1024); // 100 MB limit
                
                http::async_read(*socket_ptr, *buffer, *parser,
                    [this, socket_ptr, parser, buffer](boost::system::error_code ec, std::size_t) {
                        if (!ec) {
                            handle_request(parser->get(), socket_ptr);
                        } else {
                            LOG_WARN("Failed to read HTTP request: {}", ec.message());
                            http::response<http::string_body> res{http::status::bad_request, 11}; // HTTP/1.1
                            res.set(http::field::content_type, "application/json");
                            res.body() = "{\"success\":false,\"error\":\"Payload too large or read error\"}";
                            res.prepare_payload();
                            auto res_ptr = std::make_shared<http::response<http::string_body>>(std::move(res));
                            http::async_write(*socket_ptr, *res_ptr, [socket_ptr, res_ptr](boost::system::error_code, std::size_t){
                                socket_ptr->shutdown(tcp::socket::shutdown_send);
                            });
                        }
                    });
            }
            do_accept();
        });
}

void HttpServer::handle_request(http::request<http::string_body> req, std::shared_ptr<tcp::socket> socket) {
    // Check if it's a websocket upgrade
    if (boost::beast::websocket::is_upgrade(req)) {
        LOG_INFO("WebSocket upgrade requested");
        // Hand off to WebsocketHandler (stubbed for now)
        return;
    }

    // Handle REST
    auto res = RestRoutes::handle(req, bridge_, config_);
    
    auto sp = std::make_shared<http::response<http::string_body>>(std::move(res));
    http::async_write(*socket, *sp,
        [socket, sp](boost::system::error_code ec, std::size_t) {
            socket->shutdown(tcp::socket::shutdown_send, ec);
        });
}

} // namespace certosc
