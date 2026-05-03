#include "certosc/metrics/system_monitor.h"
#include "certosc/common/logging.h"
#include "certosc/common/types.h"

#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <thread>
#include <vector>

namespace certosc {

MetricsSnapshot SystemMonitor::collect(const std::string& node_id) {
    MetricsSnapshot snap;
    snap.set_node_id(node_id);
    snap.set_timestamp(now_unix());

    int64_t now_ms = now_unix_ms();
    int64_t time_delta = now_ms - last_collection_time_;

    snap.set_cpu_usage_percent(get_cpu_usage());

    uint64_t ram_used, ram_total;
    double ram_pct;
    get_ram_usage(ram_used, ram_total, ram_pct);
    snap.set_ram_used_mb(ram_used);
    snap.set_ram_total_mb(ram_total);
    snap.set_ram_usage_percent(ram_pct);

    uint64_t disk_used, disk_total;
    double disk_pct;
    get_disk_usage("/", disk_used, disk_total, disk_pct);
    snap.set_disk_used_mb(disk_used);
    snap.set_disk_total_mb(disk_total);
    snap.set_disk_usage_percent(disk_pct);

    double l1, l5, l15;
    get_load_avg(l1, l5, l15);
    snap.set_load_avg_1m(l1);
    snap.set_load_avg_5m(l5);
    snap.set_load_avg_15m(l15);

    double rx_sec, tx_sec;
    get_net_io(rx_sec, tx_sec, time_delta);
    snap.set_network_rx_bytes_sec(rx_sec);
    snap.set_network_tx_bytes_sec(tx_sec);

    // TODO: Implement actual GPU monitoring (e.g., via NVML)
    snap.set_gpu_usage_percent(0.0);

    // Count processes roughly via /proc
    uint32_t pcount = 0;
    try {
        for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
            if (entry.is_directory()) {
                std::string name = entry.path().filename().string();
                if (std::all_of(name.begin(), name.end(), ::isdigit)) {
                    pcount++;
                }
            }
        }
    } catch (...) {}
    snap.set_process_count(pcount);

    last_collection_time_ = now_ms;
    return snap;
}

double SystemMonitor::get_cpu_usage() {
    std::ifstream file("/proc/stat");
    if (!file.is_open()) return 0.0;

    std::string line;
    std::getline(file, line);
    std::istringstream iss(line);
    std::string cpu_label;
    iss >> cpu_label;

    if (cpu_label != "cpu") return 0.0;

    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
    iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    uint64_t total_idle = idle + iowait;
    uint64_t total_non_idle = user + nice + system + irq + softirq + steal;
    uint64_t total = total_idle + total_non_idle;

    double diff_total = static_cast<double>(total - last_cpu_total_);
    double diff_idle = static_cast<double>(total_idle - last_cpu_idle_);

    last_cpu_total_ = total;
    last_cpu_idle_ = total_idle;

    if (diff_total == 0) return 0.0;
    return (1.0 - (diff_idle / diff_total)) * 100.0;
}

void SystemMonitor::get_ram_usage(uint64_t& used_mb, uint64_t& total_mb, double& percent) {
    struct sysinfo mem_info;
    if (sysinfo(&mem_info) == 0) {
        uint64_t total = mem_info.totalram * mem_info.mem_unit;
        uint64_t free = mem_info.freeram * mem_info.mem_unit;
        // Approximation: a real implementation would parse /proc/meminfo to subtract buffers/cache
        uint64_t used = total - free;

        total_mb = total / (1024 * 1024);
        used_mb = used / (1024 * 1024);
        percent = (total > 0) ? (static_cast<double>(used) / total) * 100.0 : 0.0;
    } else {
        used_mb = 0; total_mb = 0; percent = 0.0;
    }
}

void SystemMonitor::get_disk_usage(const std::string& path, uint64_t& used_mb, uint64_t& total_mb, double& percent) {
    struct statvfs stat;
    if (statvfs(path.c_str(), &stat) == 0) {
        uint64_t total = stat.f_blocks * stat.f_frsize;
        uint64_t free = stat.f_bfree * stat.f_frsize;
        uint64_t used = total - free;

        total_mb = total / (1024 * 1024);
        used_mb = used / (1024 * 1024);
        percent = (total > 0) ? (static_cast<double>(used) / total) * 100.0 : 0.0;
    } else {
        used_mb = 0; total_mb = 0; percent = 0.0;
    }
}

void SystemMonitor::get_load_avg(double& l1, double& l5, double& l15) {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        // Loads are given as 1/65536 fractions
        l1 = info.loads[0] / 65536.0;
        l5 = info.loads[1] / 65536.0;
        l15 = info.loads[2] / 65536.0;
    } else {
        l1 = l5 = l15 = 0.0;
    }
}

void SystemMonitor::get_net_io(double& rx_bytes_sec, double& tx_bytes_sec, int64_t time_delta_ms) {
    std::ifstream file("/proc/net/dev");
    if (!file.is_open()) {
        rx_bytes_sec = tx_bytes_sec = 0.0;
        return;
    }

    std::string line;
    uint64_t total_rx = 0;
    uint64_t total_tx = 0;

    // Skip first two header lines
    std::getline(file, line);
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string iface;
        iss >> iface;

        if (iface.find("lo:") != std::string::npos) continue; // Skip loopback

        uint64_t rx_bytes, tx_bytes;
        // Structure: IFACE: rx_bytes rx_packets ... tx_bytes tx_packets ...
        // We only care about rx_bytes (1st num) and tx_bytes (9th num)
        uint64_t dummy;
        iss >> rx_bytes >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy >> tx_bytes;

        total_rx += rx_bytes;
        total_tx += tx_bytes;
    }

    if (time_delta_ms > 0 && last_net_rx_ > 0) {
        rx_bytes_sec = static_cast<double>(total_rx - last_net_rx_) / (time_delta_ms / 1000.0);
        tx_bytes_sec = static_cast<double>(total_tx - last_net_tx_) / (time_delta_ms / 1000.0);
    } else {
        rx_bytes_sec = tx_bytes_sec = 0.0;
    }

    last_net_rx_ = total_rx;
    last_net_tx_ = total_tx;
}

} // namespace certosc
