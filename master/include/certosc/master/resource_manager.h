#pragma once

#include "certosc/common/types.h"
#include "certosc/master.pb.h"
#include <mutex>
#include <unordered_map>

namespace certosc {

class ResourceManager {
public:
    ResourceManager() = default;

    bool allocate(const std::string& node_id, const ResourceSpec& req);
    void deallocate(const std::string& node_id, const ResourceSpec& req);

    void set_node_capacity(const std::string& node_id, const ResourceSpec& total);
    ResourceSpec get_available(const std::string& node_id);
    
    // Check if a node has enough resources for a job
    bool can_accommodate(const std::string& node_id, const ResourceSpec& req);

private:
    std::mutex mutex_;
    struct NodeCapacity {
        ResourceSpec total;
        ResourceSpec allocated;
    };
    std::unordered_map<std::string, NodeCapacity> capacities_;
};

} // namespace certosc
