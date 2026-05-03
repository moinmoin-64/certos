#include "certosc/master/master_server.h"
#include "certosc/common/logging.h"
#include "certosc/common/config.h"
#include <iostream>
#include <filesystem>
#include <signal.h>

std::unique_ptr<certosc::MasterServer> g_server;

void signal_handler(int signum) {
    LOG_INFO("Received signal {}, shutting down...", signum);
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char** argv) {
    std::string config_path = "config/master.yaml";
    if (argc > 1) {
        config_path = argv[1];
    }

    certosc::init_logging("master", "logs", "debug");
    LOG_INFO("Starting {} Master Node...", certosc::PRODUCT_NAME);

    // Ensure config directory exists for local testing
    std::filesystem::create_directories("config");
    std::filesystem::create_directories("logs");

    certosc::MasterConfig cfg;
    if (std::filesystem::exists(config_path)) {
        cfg = certosc::load_master_config(config_path);
    } else {
        LOG_WARN("Config file {} not found, using defaults", config_path);
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        g_server = std::make_unique<certosc::MasterServer>(cfg);
        g_server->start();
        g_server->wait();
    } catch (const std::exception& e) {
        LOG_CRITICAL("Master server crashed: {}", e.what());
        return 1;
    }

    return 0;
}
