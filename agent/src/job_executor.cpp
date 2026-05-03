#include "certosc/agent/job_executor.h"
#include "certosc/common/logging.h"

#include <cstdlib>
#include <cstdio>
#include <array>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>

namespace certosc {

JobExecutor::JobExecutor(const std::string& work_dir) : work_dir_(work_dir) {
    std::filesystem::create_directories(work_dir_);
}

JobExecutor::~JobExecutor() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pair : running_jobs_) {
        if (pair.second.pid > 0) {
            // Best effort kill on shutdown
            kill(pair.second.pid, SIGKILL);
        }
        if (pair.second.thread.joinable()) {
            pair.second.thread.detach(); // Not ideal, but we're destructing
        }
    }
}

void JobExecutor::set_callbacks(JobStartedCallback on_start, JobCompletedCallback on_complete, JobLogChunkCallback on_log_chunk) {
    on_started_ = std::move(on_start);
    on_completed_ = std::move(on_complete);
    on_log_chunk_ = std::move(on_log_chunk);
}

uint32_t JobExecutor::get_running_count() {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_jobs_.size();
}

bool JobExecutor::execute_async(const std::string& job_id, const JobSpec& spec) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_jobs_.find(job_id) != running_jobs_.end()) {
        return false; // Already running
    }

    RunningJob rjob;
    // We start the thread. The thread will actually fork/exec so it can waitpid without blocking the daemon.
    rjob.thread = std::thread(&JobExecutor::execution_thread, this, job_id, spec);
    running_jobs_[job_id] = std::move(rjob);
    return true;
}

bool JobExecutor::cancel(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = running_jobs_.find(job_id);
    if (it != running_jobs_.end() && it->second.pid > 0) {
        LOG_INFO("Killing job process {}", it->second.pid);
        kill(it->second.pid, SIGTERM);
        return true;
    }
    return false;
}

void JobExecutor::execution_thread(std::string job_id, JobSpec spec) {
    LOG_INFO("Starting execution thread for job {}", job_id);
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        if (on_completed_) on_completed_(JobResult{job_id, -1, "Failed to create pipe", "", "", false});
        return;
    }

    // Prepare workspace BEFORE fork if archive is provided
    std::string sandbox_dir;
    if (!spec.archive_path().empty()) {
        sandbox_dir = work_dir_ + "/" + job_id;
        std::filesystem::create_directories(sandbox_dir);
        
        LOG_INFO("Extracting archive {} to {}", spec.archive_path(), sandbox_dir);
        std::string untar_cmd = "tar -xzf " + spec.archive_path() + " -C " + sandbox_dir;
        int ret = std::system(untar_cmd.c_str());
        if (ret != 0) {
            LOG_ERROR("Failed to extract archive for job {}", job_id);
        }
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        if (on_completed_) on_completed_(JobResult{job_id, -1, "Failed to fork", "", "", false});
        return;
    }

    if (pid == 0) {
        // Child process
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        if (!sandbox_dir.empty()) {
            if (chdir(sandbox_dir.c_str()) != 0) {
                perror("chdir failed");
                _exit(1);
            }
        }

        std::vector<char*> c_args;
        std::string singularity_cmd = "singularity";
        std::string singularity_sub = "exec";
        
        if (!spec.container_image().empty()) {
            c_args.push_back(const_cast<char*>(singularity_cmd.c_str()));
            c_args.push_back(const_cast<char*>(singularity_sub.c_str()));
            c_args.push_back(const_cast<char*>("--bind"));
            c_args.push_back(const_cast<char*>(sandbox_dir.c_str()));
            c_args.push_back(const_cast<char*>(spec.container_image().c_str()));
        }

        c_args.push_back(const_cast<char*>(spec.command().c_str()));
        for (const auto& arg : spec.args()) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        execvp(c_args[0], c_args.data());
        perror("execvp failed");
        _exit(127);
    }

    // Parent process
    close(pipefd[1]);
    
    // Register PID
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = running_jobs_.find(job_id);
        if (it != running_jobs_.end()) {
            it->second.pid = pid;
        }
    }

    if (on_started_) on_started_(job_id, pid);

    std::string stdout_data;
    char buffer[1024];
    ssize_t count;
    while ((count = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[count] = '\0';
        std::string chunk(buffer);
        stdout_data += chunk;
        if (on_log_chunk_) {
            on_log_chunk_(job_id, chunk);
        }
    }
    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);

    int exit_code = -1;
    bool success = false;
    std::string error_msg;

    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
        success = (exit_code == 0);
    } else if (WIFSIGNALED(status)) {
        exit_code = 128 + WTERMSIG(status);
        error_msg = "Process killed by signal " + std::to_string(WTERMSIG(status));
    }

    JobResult result;
    result.job_id = job_id;
    result.exit_code = exit_code;
    result.success = success;
    result.error_message = error_msg;
    result.stdout_data = stdout_data;
    
    // Remove from running map
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = running_jobs_.find(job_id);
        if (it != running_jobs_.end()) {
            it->second.thread.detach();
            running_jobs_.erase(it);
        }
    }
    
    if (on_completed_) {
        on_completed_(result);
    }
}

} // namespace certosc
