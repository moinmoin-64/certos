#pragma once

#include "certosc/common/types.h"
#include "certosc/master/state_store.h"
#include "certosc/master.pb.h"
#include <unordered_map>
#include <mutex>
#include <vector>
#include <optional>
#include <memory>

namespace certosc {

class JobManager {
public:
    explicit JobManager(std::shared_ptr<StateStore> db = nullptr);

    std::string submit_job(const JobSpec& spec, const std::string& user_id, const std::string& username);
    bool update_job_status(const std::string& job_id, JobStatus status, const std::string& node_id = "");
    
    std::optional<JobInfo> get_job(const std::string& job_id);
    std::vector<JobInfo> list_jobs(JobStatus filter_status = JOB_STATUS_UNSPECIFIED);
    std::vector<JobInfo> get_pending_jobs(); // Jobs that are QUEUED
    
    void fail_jobs_on_node(const std::string& node_id);
    
    bool cancel_job(const std::string& job_id);
    std::vector<std::string> get_jobs_to_cancel_for_node(const std::string& node_id);

private:
    std::mutex mutex_;
    std::unordered_map<std::string, JobInfo> jobs_;
    std::shared_ptr<StateStore> db_;
};

} // namespace certosc
