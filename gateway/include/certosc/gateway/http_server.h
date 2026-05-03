#pragma once

#include "certosc/common/config.h"
#include "certosc/gateway/grpc_bridge.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <string>
#include <thread>

namespace certosc {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class HttpServer {
public:
    HttpServer(const GatewayConfig& config, std::shared_ptr<GrpcBridge> bridge);
    void start();
    void stop();

private:
    void do_accept();
    void handle_request(http::request<http::string_body> req, std::shared_ptr<tcp::socket> socket);

    GatewayConfig config_;
    std::shared_ptr<GrpcBridge> bridge_;
    net::io_context ioc_;
    std::unique_ptr<tcp::acceptor> acceptor_;
    std::thread thread_;
};

} // namespace certosc
