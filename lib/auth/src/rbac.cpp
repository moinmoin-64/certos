#include "certosc/auth/rbac.h"

namespace certosc {

RbacManager::RbacManager() {
    init_default_roles();
}

void RbacManager::init_default_roles() {
    // USER role — can manage own jobs, view nodes/cluster
    role_permissions_["user"] = {
        static_cast<int>(Permission::JOB_SUBMIT),
        static_cast<int>(Permission::JOB_VIEW_OWN),
        static_cast<int>(Permission::JOB_CANCEL_OWN),
        static_cast<int>(Permission::NODE_VIEW),
        static_cast<int>(Permission::CLUSTER_VIEW),
    };

    // ADMIN role — full access
    role_permissions_["admin"] = {
        static_cast<int>(Permission::JOB_SUBMIT),
        static_cast<int>(Permission::JOB_VIEW_OWN),
        static_cast<int>(Permission::JOB_VIEW_ALL),
        static_cast<int>(Permission::JOB_CANCEL_OWN),
        static_cast<int>(Permission::JOB_CANCEL_ALL),
        static_cast<int>(Permission::JOB_SET_PRIORITY),
        static_cast<int>(Permission::NODE_VIEW),
        static_cast<int>(Permission::NODE_DRAIN),
        static_cast<int>(Permission::NODE_MANAGE),
        static_cast<int>(Permission::USER_VIEW),
        static_cast<int>(Permission::USER_CREATE),
        static_cast<int>(Permission::USER_DELETE),
        static_cast<int>(Permission::CLUSTER_VIEW),
        static_cast<int>(Permission::CLUSTER_MANAGE),
    };

    // NODE role — internal agent role
    role_permissions_["node"] = {
        static_cast<int>(Permission::NODE_REGISTER),
        static_cast<int>(Permission::NODE_HEARTBEAT),
        static_cast<int>(Permission::NODE_REPORT),
    };
}

bool RbacManager::has_permission(const std::string& role, Permission perm) const {
    auto it = role_permissions_.find(role);
    if (it == role_permissions_.end()) {
        return false;
    }
    return it->second.count(static_cast<int>(perm)) > 0;
}

std::vector<Permission> RbacManager::get_permissions(const std::string& role) const {
    std::vector<Permission> result;
    auto it = role_permissions_.find(role);
    if (it != role_permissions_.end()) {
        for (int p : it->second) {
            result.push_back(static_cast<Permission>(p));
        }
    }
    return result;
}

std::string RbacManager::permission_name(Permission perm) {
    switch (perm) {
        case Permission::JOB_SUBMIT: return "job:submit";
        case Permission::JOB_VIEW_OWN: return "job:view_own";
        case Permission::JOB_VIEW_ALL: return "job:view_all";
        case Permission::JOB_CANCEL_OWN: return "job:cancel_own";
        case Permission::JOB_CANCEL_ALL: return "job:cancel_all";
        case Permission::JOB_SET_PRIORITY: return "job:set_priority";
        case Permission::NODE_VIEW: return "node:view";
        case Permission::NODE_DRAIN: return "node:drain";
        case Permission::NODE_MANAGE: return "node:manage";
        case Permission::USER_VIEW: return "user:view";
        case Permission::USER_CREATE: return "user:create";
        case Permission::USER_DELETE: return "user:delete";
        case Permission::CLUSTER_VIEW: return "cluster:view";
        case Permission::CLUSTER_MANAGE: return "cluster:manage";
        case Permission::NODE_REGISTER: return "node:register";
        case Permission::NODE_HEARTBEAT: return "node:heartbeat";
        case Permission::NODE_REPORT: return "node:report";
        default: return "unknown";
    }
}

} // namespace certosc
