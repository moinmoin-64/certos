#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <functional>
#include <memory>
#include <vector>
#include <map>
#include <optional>

namespace certosc {

// ─── Version ───────────────────────────────────────────────────────────────
constexpr const char* VERSION = "1.0.0";
constexpr const char* PRODUCT_NAME = "CertOS HPC Cloud OS";

// ─── Time Utilities ────────────────────────────────────────────────────────
using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;

inline int64_t now_unix() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        Clock::now().time_since_epoch()
    ).count();
}

inline int64_t now_unix_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now().time_since_epoch()
    ).count();
}

// ─── Default Configuration Values ──────────────────────────────────────────
namespace defaults {
    constexpr uint16_t MASTER_GRPC_PORT = 50051;
    constexpr uint16_t GATEWAY_HTTP_PORT = 8080;
    constexpr uint16_t GATEWAY_WS_PORT = 8081;
    constexpr uint32_t HEARTBEAT_INTERVAL_SEC = 5;
    constexpr uint32_t HEARTBEAT_TIMEOUT_SEC = 15;
    constexpr uint32_t METRICS_INTERVAL_SEC = 5;
    constexpr uint32_t SCHEDULER_INTERVAL_MS = 1000;
    constexpr uint32_t MAX_JOBS_PER_USER = 100;
    constexpr uint32_t DEFAULT_TIME_LIMIT_SEC = 3600;
    constexpr uint32_t JWT_EXPIRY_SEC = 3600;
    constexpr double FAIR_SHARE_DECAY_HALFLIFE_HOURS = 24.0;
    constexpr uint32_t BACKFILL_WINDOW_SEC = 3600;
    constexpr uint32_t MAX_BACKFILL_JOBS = 50;
    constexpr const char* DEFAULT_DB_PATH = "certosc.db";
    constexpr const char* DEFAULT_LOG_DIR = "/var/log/certosc";
    constexpr const char* JWT_SECRET = "certosc-dev-secret-change-in-production";
} // namespace defaults

} // namespace certosc
