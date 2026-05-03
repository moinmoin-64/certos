#pragma once

#include "certosc/gateway/grpc_bridge.h"
#include "certosc/common/config.h"
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <string>

namespace certosc {

namespace http = boost::beast::http;
namespace json = boost::json;

class RestRoutes {
public:
    static http::response<http::string_body> handle(
        const http::request<http::string_body>& req, 
        std::shared_ptr<GrpcBridge> bridge,
        const GatewayConfig& config);

private:
    static void add_cors_headers(http::response<http::string_body>& res, const std::string& origin);
};

} // namespace certosc
