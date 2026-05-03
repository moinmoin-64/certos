#include "certosc/gateway/http_server.h"
#include "certosc/common/logging.h"
#include "certosc/common/config.h"
#include "certosc/common/types.h"
#include <thread>

#include <iostream>
#include <filesystem>
#include <signal.h>

std::unique_ptr<certosc::HttpServer> g_server;

void signal_handler(int signum) {
    LOG_INFO("Received signal {}, shutting down...", signum);
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char** argv) {
    std::string config_path = "config/gateway.yaml";
    if (argc > 1) {
        config_path = argv[1];
    }

    certosc::init_logging("gateway", "logs", "debug");
    LOG_INFO("Starting {} API Gateway...", certosc::PRODUCT_NAME);

    std::filesystem::create_directories("config");
    std::filesystem::create_directories("logs");

    certosc::GatewayConfig cfg;
    if (std::filesystem::exists(config_path)) {
        cfg = certosc::load_gateway_config(config_path);
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        std::string master_addr = cfg.master_address + ":" + std::to_string(cfg.master_port);
        auto bridge = std::make_shared<certosc::GrpcBridge>(master_addr);
        
        g_server = std::make_unique<certosc::HttpServer>(cfg, bridge);
        g_server->start();
        
        // Block until stopped
        while (true) {
            std::this_thread::sleep_for(std::chrono::hours(1));
        }
    } catch (const std::exception& e) {
        LOG_CRITICAL("Gateway crashed: {}", e.what());
        return 1;
    }

    return 0;
}
