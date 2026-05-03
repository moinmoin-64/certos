#pragma once

#include <string>
#include <cstdint>
#include <yaml-cpp/yaml.h>

namespace certosc {

/// Master node configuration
struct MasterConfig {
    std::string bind_address = "0.0.0.0";
    uint16_t grpc_port = 50051;
    std::string db_path = "certosc.db";
    std::string log_dir = "/var/log/certosc";
    std::string log_level = "info";
    std::string jwt_secret = "certosc-dev-secret-change-in-production";
    uint32_t jwt_expiry_sec = 3600;
    uint32_t heartbeat_timeout_sec = 15;
    uint32_t scheduler_interval_ms = 1000;
    uint32_t backfill_window_sec = 3600;
    uint32_t max_backfill_jobs = 50;
    double fair_share_decay_hours = 24.0;
    bool enable_tls = false;
    std::string tls_cert_path;
    std::string tls_key_path;
    std::string node_auth_key = "certosc-node-key";
};

/// Agent node configuration
struct AgentConfig {
    std::string master_address = "localhost";
    uint16_t master_port = 50051;
    uint32_t heartbeat_interval_sec = 5;
    uint32_t metrics_interval_sec = 5;
    std::string log_dir = "/var/log/certosc";
    std::string log_level = "info";
    std::string work_dir = "/tmp/certosc-jobs";
    std::string auth_key = "certosc-node-key";
    uint32_t max_concurrent_jobs = 0;  // 0 = auto (number of CPU cores)
    bool enable_tls = false;
    std::string tls_ca_path;
    // Resource overrides (0 = auto-detect)
    uint32_t cpu_cores = 0;
    uint64_t ram_mb = 0;
    uint32_t gpu_count = 0;
};

/// Gateway configuration
struct GatewayConfig {
    std::string bind_address = "0.0.0.0";
    uint16_t http_port = 8080;
    std::string master_address = "localhost";
    uint16_t master_port = 50051;
    std::string log_level = "info";
    std::string web_root = "../web";
    std::string jwt_secret = "certosc-dev-secret-change-in-production";
    bool enable_cors = true;
    std::string cors_origin = "*";
};

/// Load master configuration from YAML file
MasterConfig load_master_config(const std::string& path);

/// Load agent configuration from YAML file
AgentConfig load_agent_config(const std::string& path);

/// Load gateway configuration from YAML file
GatewayConfig load_gateway_config(const std::string& path);

} // namespace certosc
