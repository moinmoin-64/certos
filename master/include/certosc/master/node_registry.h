#pragma once

#include "certosc/common/types.h"
#include "certosc/master.pb.h"
#include <unordered_map>
#include <mutex>
#include <string>

namespace certosc {

class NodeRegistry {
public:
    NodeRegistry() = default;

    std::string register_node(const NodeInfo& info);
    bool unregister_node(const std::string& node_id);
    
    bool update_heartbeat(const std::string& node_id, const ResourceSpec& available_resources, uint32_t running_jobs);
    
    std::vector<NodeInfo> get_all_nodes();
    std::optional<NodeInfo> get_node(const std::string& node_id);

    // Called periodically to mark dead nodes offline
    void check_timeouts(uint32_t timeout_sec);

private:
    std::mutex mutex_;
    std::unordered_map<std::string, NodeInfo> nodes_;
};

} // namespace certosc
