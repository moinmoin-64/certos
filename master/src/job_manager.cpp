#include "certosc/master/job_manager.h"
#include "certosc/common/logging.h"
#include "certosc/common/uuid.h"

namespace certosc {

JobManager::JobManager(std::shared_ptr<StateStore> db) : db_(db) {
    // Optional: Load jobs from DB on startup
}

std::string JobManager::submit_job(const JobSpec& spec, const std::string& user_id, const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string job_id = "job-" + generate_uuid().substr(0, 8);
    
    JobInfo info;
    info.set_job_id(job_id);
    info.set_user_id(user_id);
    info.set_username(username);
    info.mutable_spec()->CopyFrom(spec);
    info.set_status(JOB_STATUS_QUEUED);
    info.set_submitted_at(now_unix());
    info.set_effective_priority(spec.priority()); // Initial
    
    jobs_[job_id] = info;
    
    if (db_) {
        // Simple JSON serialization for now (mocked in logic)
        db_->set_kv("job:" + job_id, "QUEUED");
    }

    LOG_INFO("Job {} submitted by {}", job_id, username);
    return job_id;
}

bool JobManager::update_job_status(const std::string& job_id, JobStatus status, const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) return false;
    
    JobInfo& info = it->second;
    info.set_status(status);
    
    if (status == JOB_STATUS_ASSIGNED || status == JOB_STATUS_RUNNING) {
        if (!node_id.empty()) {
            info.set_assigned_node_id(node_id);
        }
        if (status == JOB_STATUS_RUNNING && info.started_at() == 0) {
            info.set_started_at(now_unix());
        }
    } else if (status == JOB_STATUS_COMPLETED || status == JOB_STATUS_FAILED || status == JOB_STATUS_CANCELLED) {
        info.set_completed_at(now_unix());
    }
    
    LOG_DEBUG("Job {} status updated to {}", job_id, status);
    return true;
}

std::optional<JobInfo> JobManager::get_job(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = jobs_.find(job_id);
    if (it != jobs_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<JobInfo> JobManager::list_jobs(JobStatus filter_status) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<JobInfo> result;
    for (const auto& pair : jobs_) {
        if (filter_status == JOB_STATUS_UNSPECIFIED || pair.second.status() == filter_status) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<JobInfo> JobManager::get_pending_jobs() {
    return list_jobs(JOB_STATUS_QUEUED);
}

void JobManager::fail_jobs_on_node(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pair : jobs_) {
        JobInfo& info = pair.second;
        if ((info.status() == JOB_STATUS_ASSIGNED || info.status() == JOB_STATUS_RUNNING) &&
            info.assigned_node_id() == node_id) {
            
            info.set_status(JOB_STATUS_FAILED);
            info.set_error_message("Node went offline");
            info.set_completed_at(now_unix());
            LOG_WARN("Job {} failed because node {} went offline", pair.first, node_id);
        }
    }
}

bool JobManager::cancel_job(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) return false;
    
    JobStatus current = it->second.status();
    if (current == JOB_STATUS_COMPLETED || current == JOB_STATUS_FAILED || current == JOB_STATUS_CANCELLED) {
        return false; // Already done
    }
    
    if (current == JOB_STATUS_QUEUED) {
        it->second.set_status(JOB_STATUS_CANCELLED);
        it->second.set_completed_at(now_unix());
        LOG_INFO("Job {} cancelled in queue", job_id);
    } else {
        // It's assigned or running. Set to CANCELLING so the heartbeat picks it up.
        // We reuse PREEMPTED or add a pseudo status, but since protobuf has CANCELLED,
        // we'll use a custom internal state or just mark as CANCELLED and still send it.
        // If it's CANCELLED, the node heartbeat will see it and kill it.
        it->second.set_status(JOB_STATUS_CANCELLED);
        it->second.set_completed_at(now_unix());
        LOG_INFO("Job {} marked for cancellation on node {}", job_id, it->second.assigned_node_id());
    }
    return true;
}

std::vector<std::string> JobManager::get_jobs_to_cancel_for_node(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> to_cancel;
    for (const auto& pair : jobs_) {
        const JobInfo& info = pair.second;
        // If the master considers it cancelled but the node might still be running it
        // We send cancel commands for all recently cancelled jobs assigned to this node.
        // For a robust system, we would track acknowledgment. Here we just send it if cancelled.
        if (info.status() == JOB_STATUS_CANCELLED && info.assigned_node_id() == node_id) {
            // Only send if it was cancelled recently to avoid infinite sending
            if (now_unix() - info.completed_at() < 60) {
                to_cancel.push_back(info.job_id());
            }
        }
    }
    return to_cancel;
}

} // namespace certosc
