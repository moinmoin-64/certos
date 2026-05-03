#pragma once

#include <string>
#include <spdlog/spdlog.h>

namespace certosc {

/// Initialize the logging system
/// @param component_name Name of the component (e.g., "master", "agent", "gateway")
/// @param log_dir Directory for log files
/// @param level Log level string ("trace", "debug", "info", "warn", "error", "critical")
void init_logging(const std::string& component_name,
                  const std::string& log_dir = "",
                  const std::string& level = "info");

/// Get the default logger
std::shared_ptr<spdlog::logger> get_logger();

// Convenience macros
#define LOG_TRACE(...)    SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...)    SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...)     SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)     SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...)    SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

} // namespace certosc
