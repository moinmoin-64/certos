#pragma once

#include "certosc/metrics.pb.h"
#include <string>

namespace certosc {

/// System resource monitor — collects CPU, RAM, and Disk metrics
class SystemMonitor {
public:
    SystemMonitor() = default;

    /// Collect the current system metrics
    /// @param node_id The ID of this node to inject into the snapshot
    MetricsSnapshot collect(const std::string& node_id);

private:
    uint64_t last_cpu_total_ = 0;
    uint64_t last_cpu_idle_ = 0;
    uint64_t last_net_rx_ = 0;
    uint64_t last_net_tx_ = 0;
    int64_t last_collection_time_ = 0;

    double get_cpu_usage();
    void get_ram_usage(uint64_t& used_mb, uint64_t& total_mb, double& percent);
    void get_disk_usage(const std::string& path, uint64_t& used_mb, uint64_t& total_mb, double& percent);
    void get_load_avg(double& l1, double& l5, double& l15);
    void get_net_io(double& rx_bytes_sec, double& tx_bytes_sec, int64_t time_delta_ms);
};

} // namespace certosc
