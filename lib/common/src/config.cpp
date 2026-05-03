#include "certosc/common/config.h"
#include <iostream>
#include <fstream>

namespace certosc {

namespace {

template<typename T>
T yaml_get(const YAML::Node& node, const std::string& key, const T& default_val) {
    if (node[key]) {
        return node[key].as<T>();
    }
    return default_val;
}

} // anonymous namespace

MasterConfig load_master_config(const std::string& path) {
    MasterConfig cfg;
    try {
        YAML::Node root = YAML::LoadFile(path);
        auto node = root["master"];
        if (!node) {
            std::cerr << "[config] No 'master' section found, using defaults\n";
            return cfg;
        }

        cfg.bind_address = yaml_get<std::string>(node, "bind_address", cfg.bind_address);
        cfg.grpc_port = yaml_get<int>(node, "grpc_port", cfg.grpc_port);
        cfg.db_path = yaml_get<std::string>(node, "db_path", cfg.db_path);
        cfg.log_dir = yaml_get<std::string>(node, "log_dir", cfg.log_dir);
        cfg.log_level = yaml_get<std::string>(node, "log_level", cfg.log_level);
        cfg.jwt_secret = yaml_get<std::string>(node, "jwt_secret", cfg.jwt_secret);
        cfg.jwt_expiry_sec = yaml_get<int>(node, "jwt_expiry_sec", cfg.jwt_expiry_sec);
        cfg.heartbeat_timeout_sec = yaml_get<int>(node, "heartbeat_timeout_sec", cfg.heartbeat_timeout_sec);
        cfg.scheduler_interval_ms = yaml_get<int>(node, "scheduler_interval_ms", cfg.scheduler_interval_ms);
        cfg.backfill_window_sec = yaml_get<int>(node, "backfill_window_sec", cfg.backfill_window_sec);
        cfg.max_backfill_jobs = yaml_get<int>(node, "max_backfill_jobs", cfg.max_backfill_jobs);
        cfg.fair_share_decay_hours = yaml_get<double>(node, "fair_share_decay_hours", cfg.fair_share_decay_hours);
        cfg.enable_tls = yaml_get<bool>(node, "enable_tls", cfg.enable_tls);
        cfg.tls_cert_path = yaml_get<std::string>(node, "tls_cert_path", cfg.tls_cert_path);
        cfg.tls_key_path = yaml_get<std::string>(node, "tls_key_path", cfg.tls_key_path);
        cfg.node_auth_key = yaml_get<std::string>(node, "node_auth_key", cfg.node_auth_key);
    } catch (const YAML::Exception& e) {
        std::cerr << "[config] Failed to load master config from " << path
                  << ": " << e.what() << "\n";
    }
    return cfg;
}

AgentConfig load_agent_config(const std::string& path) {
    AgentConfig cfg;
    try {
        YAML::Node root = YAML::LoadFile(path);
        auto node = root["agent"];
        if (!node) {
            std::cerr << "[config] No 'agent' section found, using defaults\n";
            return cfg;
        }

        cfg.master_address = yaml_get<std::string>(node, "master_address", cfg.master_address);
        cfg.master_port = yaml_get<int>(node, "master_port", cfg.master_port);
        cfg.heartbeat_interval_sec = yaml_get<int>(node, "heartbeat_interval_sec", cfg.heartbeat_interval_sec);
        cfg.metrics_interval_sec = yaml_get<int>(node, "metrics_interval_sec", cfg.metrics_interval_sec);
        cfg.log_dir = yaml_get<std::string>(node, "log_dir", cfg.log_dir);
        cfg.log_level = yaml_get<std::string>(node, "log_level", cfg.log_level);
        cfg.work_dir = yaml_get<std::string>(node, "work_dir", cfg.work_dir);
        cfg.auth_key = yaml_get<std::string>(node, "auth_key", cfg.auth_key);
        cfg.max_concurrent_jobs = yaml_get<int>(node, "max_concurrent_jobs", cfg.max_concurrent_jobs);
        cfg.enable_tls = yaml_get<bool>(node, "enable_tls", cfg.enable_tls);
        cfg.tls_ca_path = yaml_get<std::string>(node, "tls_ca_path", cfg.tls_ca_path);
        cfg.cpu_cores = yaml_get<int>(node, "cpu_cores", cfg.cpu_cores);
        cfg.ram_mb = yaml_get<int>(node, "ram_mb", cfg.ram_mb);
        cfg.gpu_count = yaml_get<int>(node, "gpu_count", cfg.gpu_count);
    } catch (const YAML::Exception& e) {
        std::cerr << "[config] Failed to load agent config from " << path
                  << ": " << e.what() << "\n";
    }
    return cfg;
}

GatewayConfig load_gateway_config(const std::string& path) {
    GatewayConfig cfg;
    try {
        YAML::Node root = YAML::LoadFile(path);
        auto node = root["gateway"];
        if (!node) {
            std::cerr << "[config] No 'gateway' section found, using defaults\n";
            return cfg;
        }

        cfg.bind_address = yaml_get<std::string>(node, "bind_address", cfg.bind_address);
        cfg.http_port = yaml_get<int>(node, "http_port", cfg.http_port);
        cfg.master_address = yaml_get<std::string>(node, "master_address", cfg.master_address);
        cfg.master_port = yaml_get<int>(node, "master_port", cfg.master_port);
        cfg.log_level = yaml_get<std::string>(node, "log_level", cfg.log_level);
        cfg.web_root = yaml_get<std::string>(node, "web_root", cfg.web_root);
        cfg.jwt_secret = yaml_get<std::string>(node, "jwt_secret", cfg.jwt_secret);
        cfg.enable_cors = yaml_get<bool>(node, "enable_cors", cfg.enable_cors);
        cfg.cors_origin = yaml_get<std::string>(node, "cors_origin", cfg.cors_origin);
    } catch (const YAML::Exception& e) {
        std::cerr << "[config] Failed to load gateway config from " << path
                  << ": " << e.what() << "\n";
    }
    return cfg;
}

} // namespace certosc
