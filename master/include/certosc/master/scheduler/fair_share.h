#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace certosc {

class FairShareManager {
public:
    explicit FairShareManager(double half_life_hours);

    // Record usage (e.g., when a job completes or periodically for running jobs)
    void add_usage(const std::string& user_id, double cpu_seconds);

    // Apply time-decay to all usages based on elapsed time
    void apply_decay();

    // Calculate a dynamic priority multiplier for a user.
    // Base priority is multiplied by this factor (<= 1.0). Heavy users get lower factors.
    double get_priority_factor(const std::string& user_id);

private:
    std::mutex mutex_;
    double half_life_hours_;
    int64_t last_decay_time_;
    std::unordered_map<std::string, double> user_usage_;
};

} // namespace certosc
