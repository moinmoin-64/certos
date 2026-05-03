#include "certosc/agent/agent_daemon.h"
#include "certosc/common/logging.h"
#include "certosc/common/config.h"
#include "certosc/common/types.h"
#include <thread>
#include <iostream>
#include <filesystem>
#include <signal.h>

std::unique_ptr<certosc::AgentDaemon> g_agent;

void signal_handler(int signum) {
    LOG_INFO("Received signal {}, shutting down...", signum);
    if (g_agent) {
        g_agent->stop();
    }
}

int main(int argc, char** argv) {
    std::string config_path = "config/agent.yaml";
    if (argc > 1) {
        config_path = argv[1];
    }

    certosc::init_logging("agent", "logs", "debug");
    LOG_INFO("Starting {} Agent Daemon...", certosc::PRODUCT_NAME);

    std::filesystem::create_directories("config");
    std::filesystem::create_directories("logs");

    certosc::AgentConfig cfg;
    if (std::filesystem::exists(config_path)) {
        cfg = certosc::load_agent_config(config_path);
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        g_agent = std::make_unique<certosc::AgentDaemon>(cfg);
        g_agent->start();
        
        // Wait until stopped by signal
        while (true) {
            std::this_thread::sleep_for(std::chrono::hours(1));
        }
    } catch (const std::exception& e) {
        LOG_CRITICAL("Agent daemon crashed: {}", e.what());
        return 1;
    }

    return 0;
}
