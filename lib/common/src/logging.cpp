#include "certosc/common/logging.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <filesystem>
#include <vector>

namespace certosc {

static std::shared_ptr<spdlog::logger> s_logger;

void init_logging(const std::string& component_name,
                  const std::string& log_dir,
                  const std::string& level) {
    std::vector<spdlog::sink_ptr> sinks;

    // Console sink with colors
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
    sinks.push_back(console_sink);

    // File sink (rotating, 10MB max, 3 files)
    if (!log_dir.empty()) {
        try {
            std::filesystem::create_directories(log_dir);
            auto file_path = log_dir + "/" + component_name + ".log";
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                file_path, 10 * 1024 * 1024, 3
            );
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] [%t] %v");
            sinks.push_back(file_sink);
        } catch (const std::exception& e) {
            // Fall back to console-only logging
            fprintf(stderr, "[logging] Failed to create file sink: %s\n", e.what());
        }
    }

    s_logger = std::make_shared<spdlog::logger>(component_name, sinks.begin(), sinks.end());

    // Set log level
    if (level == "trace") s_logger->set_level(spdlog::level::trace);
    else if (level == "debug") s_logger->set_level(spdlog::level::debug);
    else if (level == "info") s_logger->set_level(spdlog::level::info);
    else if (level == "warn") s_logger->set_level(spdlog::level::warn);
    else if (level == "error") s_logger->set_level(spdlog::level::err);
    else if (level == "critical") s_logger->set_level(spdlog::level::critical);
    else s_logger->set_level(spdlog::level::info);

    spdlog::set_default_logger(s_logger);
    spdlog::flush_every(std::chrono::seconds(3));

    LOG_INFO("CertOS {} logging initialized (level={})", component_name, level);
}

std::shared_ptr<spdlog::logger> get_logger() {
    if (!s_logger) {
        init_logging("certosc");
    }
    return s_logger;
}

} // namespace certosc
