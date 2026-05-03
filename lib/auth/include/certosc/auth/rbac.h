#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace certosc {

/// Permission types for RBAC
enum class Permission {
    // Job operations
    JOB_SUBMIT,
    JOB_VIEW_OWN,
    JOB_VIEW_ALL,
    JOB_CANCEL_OWN,
    JOB_CANCEL_ALL,
    JOB_SET_PRIORITY,

    // Node operations
    NODE_VIEW,
    NODE_DRAIN,
    NODE_MANAGE,

    // User operations
    USER_VIEW,
    USER_CREATE,
    USER_DELETE,

    // Cluster operations
    CLUSTER_VIEW,
    CLUSTER_MANAGE,

    // Internal
    NODE_REGISTER,
    NODE_HEARTBEAT,
    NODE_REPORT,
};

/// Role-Based Access Control manager
class RbacManager {
public:
    RbacManager();

    /// Check if a role has a specific permission
    bool has_permission(const std::string& role, Permission perm) const;

    /// Get all permissions for a role
    std::vector<Permission> get_permissions(const std::string& role) const;

    /// Get a human-readable name for a permission
    static std::string permission_name(Permission perm);

private:
    void init_default_roles();
    std::unordered_map<std::string, std::unordered_set<int>> role_permissions_;
};

} // namespace certosc
