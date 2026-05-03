#include "certosc/master/node_registry.h"
#include "certosc/common/logging.h"
#include "certosc/common/uuid.h"

namespace certosc {

std::string NodeRegistry::register_node(const NodeInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if node already exists by IP/Hostname (simplified)
    for (auto& pair : nodes_) {
        if (pair.second.hostname() == info.hostname() && pair.second.ip_address() == info.ip_address()) {
            LOG_INFO("Re-registering existing node: {}", info.hostname());
            NodeInfo& existing = pair.second;
            existing.set_status(NODE_STATUS_ONLINE);
            existing.set_last_heartbeat(now_unix());
            existing.mutable_total_resources()->CopyFrom(info.total_resources());
            existing.mutable_available_resources()->CopyFrom(info.total_resources());
            return pair.first;
        }
    }

    std::string node_id = "node-" + generate_uuid().substr(0, 8);
    NodeInfo new_node = info;
    new_node.set_node_id(node_id);
    new_node.set_status(NODE_STATUS_ONLINE);
    new_node.set_registered_at(now_unix());
    new_node.set_last_heartbeat(now_unix());
    new_node.mutable_available_resources()->CopyFrom(info.total_resources());
    
    nodes_[node_id] = new_node;
    LOG_INFO("Registered new node: {} ({})", node_id, info.hostname());
    return node_id;
}

bool NodeRegistry::unregister_node(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (nodes_.find(node_id) != nodes_.end()) {
        nodes_.erase(node_id);
        LOG_INFO("Unregistered node: {}", node_id);
        return true;
    }
    return false;
}

bool NodeRegistry::update_heartbeat(const std::string& node_id, const ResourceSpec& available_resources, uint32_t running_jobs) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(node_id);
    if (it != nodes_.end()) {
        it->second.set_last_heartbeat(now_unix());
        it->second.set_status(NODE_STATUS_ONLINE); // recover if offline
        it->second.mutable_available_resources()->CopyFrom(available_resources);
        it->second.set_running_jobs(running_jobs);
        return true;
    }
    return false;
}

std::vector<NodeInfo> NodeRegistry::get_all_nodes() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<NodeInfo> result;
    result.reserve(nodes_.size());
    for (const auto& pair : nodes_) {
        result.push_back(pair.second);
    }
    return result;
}

std::optional<NodeInfo> NodeRegistry::get_node(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(node_id);
    if (it != nodes_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void NodeRegistry::check_timeouts(uint32_t timeout_sec) {
    std::lock_guard<std::mutex> lock(mutex_);
    int64_t now = now_unix();
    for (auto& pair : nodes_) {
        if (pair.second.status() == NODE_STATUS_ONLINE) {
            if (now - pair.second.last_heartbeat() > timeout_sec) {
                LOG_WARN("Node {} timed out (no heartbeat for {}s). Marking offline.", pair.first, timeout_sec);
                pair.second.set_status(NODE_STATUS_OFFLINE);
                // The scheduler will handle re-queueing jobs on this node.
            }
        }
    }
}

} // namespace certosc
