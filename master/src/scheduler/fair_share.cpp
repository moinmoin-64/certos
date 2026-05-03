#include "certosc/master/scheduler/fair_share.h"
#include "certosc/common/types.h"
#include <cmath>

namespace certosc {

FairShareManager::FairShareManager(double half_life_hours)
    : half_life_hours_(half_life_hours > 0 ? half_life_hours : 24.0) {
    last_decay_time_ = now_unix();
}

void FairShareManager::add_usage(const std::string& user_id, double cpu_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    user_usage_[user_id] += cpu_seconds;
}

void FairShareManager::apply_decay() {
    std::lock_guard<std::mutex> lock(mutex_);
    int64_t now = now_unix();
    double hours_elapsed = static_cast<double>(now - last_decay_time_) / 3600.0;
    
    if (hours_elapsed <= 0) return;

    // Decay formula: Usage = Usage * (0.5 ^ (elapsed / half_life))
    double decay_factor = std::pow(0.5, hours_elapsed / half_life_hours_);

    for (auto& pair : user_usage_) {
        pair.second *= decay_factor;
    }
    
    last_decay_time_ = now;
}

double FairShareManager::get_priority_factor(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = user_usage_.find(user_id);
    if (it == user_usage_.end() || it->second <= 0) {
        return 1.0; // Max priority factor for users with no usage
    }

    // A simple non-linear penalty curve
    // E.g. usage=1000 CPU hours -> factor drops
    double usage = it->second;
    double factor = 1.0 / (1.0 + std::log1p(usage / 3600.0)); // Base on hours
    return factor;
}

} // namespace certosc
