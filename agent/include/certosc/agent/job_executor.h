#pragma once

#include "certosc/common/types.h"
#include "certosc/agent.pb.h"
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <unordered_map>

namespace certosc {

struct JobResult {
    std::string job_id;
    int32_t exit_code;
    std::string error_message;
    std::string stdout_data;
    std::string stderr_data;
    bool success;
};

using JobCompletedCallback = std::function<void(const JobResult&)>;
using JobStartedCallback = std::function<void(const std::string& job_id, uint32_t pid)>;
using JobLogChunkCallback = std::function<void(const std::string& job_id, const std::string& chunk)>;

class JobExecutor {
public:
    JobExecutor(const std::string& work_dir);
    ~JobExecutor();

    void set_callbacks(JobStartedCallback on_start, JobCompletedCallback on_complete, JobLogChunkCallback on_log_chunk = nullptr);

    // Launch a job in the background
    bool execute_async(const std::string& job_id, const JobSpec& spec);
    
    // Attempt to cancel a running job
    bool cancel(const std::string& job_id);
    
    uint32_t get_running_count();

private:
    void execution_thread(std::string job_id, JobSpec spec);

    std::string work_dir_;
    JobStartedCallback on_started_;
    JobCompletedCallback on_completed_;
    JobLogChunkCallback on_log_chunk_;

    std::mutex mutex_;
    struct RunningJob {
        pid_t pid = 0;
        std::thread thread;
    };
    std::unordered_map<std::string, RunningJob> running_jobs_;
};

} // namespace certosc
