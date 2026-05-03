#include "certosc/master/resource_manager.h"
#include "certosc/common/logging.h"

namespace certosc {

void ResourceManager::set_node_capacity(const std::string& node_id, const ResourceSpec& total) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& cap = capacities_[node_id];
    cap.total = total;
    // We assume allocated resources persist unless deallocated explicitly
}

bool ResourceManager::allocate(const std::string& node_id, const ResourceSpec& req) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = capacities_.find(node_id);
    if (it == capacities_.end()) return false;

    auto& cap = it->second;
    
    // Check if available
    uint32_t avail_cpu = cap.total.cpu_cores() - cap.allocated.cpu_cores();
    uint64_t avail_ram = cap.total.ram_mb() - cap.allocated.ram_mb();
    uint32_t avail_gpu = cap.total.gpu_count() - cap.allocated.gpu_count();

    if (req.cpu_cores() <= avail_cpu && req.ram_mb() <= avail_ram && req.gpu_count() <= avail_gpu) {
        cap.allocated.set_cpu_cores(cap.allocated.cpu_cores() + req.cpu_cores());
        cap.allocated.set_ram_mb(cap.allocated.ram_mb() + req.ram_mb());
        cap.allocated.set_gpu_count(cap.allocated.gpu_count() + req.gpu_count());
        return true;
    }
    return false;
}

void ResourceManager::deallocate(const std::string& node_id, const ResourceSpec& req) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = capacities_.find(node_id);
    if (it == capacities_.end()) return;

    auto& cap = it->second;
    
    // Prevent underflow
    uint32_t new_cpu = (cap.allocated.cpu_cores() > req.cpu_cores()) ? cap.allocated.cpu_cores() - req.cpu_cores() : 0;
    uint64_t new_ram = (cap.allocated.ram_mb() > req.ram_mb()) ? cap.allocated.ram_mb() - req.ram_mb() : 0;
    uint32_t new_gpu = (cap.allocated.gpu_count() > req.gpu_count()) ? cap.allocated.gpu_count() - req.gpu_count() : 0;

    cap.allocated.set_cpu_cores(new_cpu);
    cap.allocated.set_ram_mb(new_ram);
    cap.allocated.set_gpu_count(new_gpu);
}

ResourceSpec ResourceManager::get_available(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    ResourceSpec avail;
    auto it = capacities_.find(node_id);
    if (it != capacities_.end()) {
        const auto& cap = it->second;
        avail.set_cpu_cores(cap.total.cpu_cores() - cap.allocated.cpu_cores());
        avail.set_ram_mb(cap.total.ram_mb() - cap.allocated.ram_mb());
        avail.set_gpu_count(cap.total.gpu_count() - cap.allocated.gpu_count());
    }
    return avail;
}

bool ResourceManager::can_accommodate(const std::string& node_id, const ResourceSpec& req) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = capacities_.find(node_id);
    if (it == capacities_.end()) return false;

    const auto& cap = it->second;
    uint32_t avail_cpu = cap.total.cpu_cores() - cap.allocated.cpu_cores();
    uint64_t avail_ram = cap.total.ram_mb() - cap.allocated.ram_mb();
    uint32_t avail_gpu = cap.total.gpu_count() - cap.allocated.gpu_count();

    return (req.cpu_cores() <= avail_cpu && req.ram_mb() <= avail_ram && req.gpu_count() <= avail_gpu);
}

} // namespace certosc
