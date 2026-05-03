#include "certosc/agent/agent_daemon.h"
#include "certosc/common/logging.h"
#include <chrono>

namespace certosc {

AgentDaemon::AgentDaemon(const AgentConfig& config) : config_(config) {
    std::string master_target = config_.master_address + ":" + std::to_string(config_.master_port);
    grpc_client_ = std::make_shared<AgentGrpcClient>(master_target);
    executor_ = std::make_shared<JobExecutor>(config_.work_dir);
    monitor_ = std::make_shared<SystemMonitor>();
    
    executor_->set_callbacks(
        [this](const std::string& jid, uint32_t pid) {
            LOG_INFO("Job {} started locally with PID {}", jid, pid);
            grpc_client_->report_job_started(node_id_, jid, pid);
        },
        [this](const JobResult& res) {
            LOG_INFO("Job {} completed with exit code {}", res.job_id, res.exit_code);
            if (res.success || res.exit_code != -1) {
                grpc_client_->report_job_completed(node_id_, res.job_id, res.exit_code, res.stdout_data, res.stderr_data);
            } else {
                grpc_client_->report_job_failed(node_id_, res.job_id, res.error_message, res.exit_code);
            }
        }
    );
}

AgentDaemon::~AgentDaemon() {
    stop();
}

void AgentDaemon::start() {
    if (running_) return;

    // Initial Registration
    ResourceSpec total;
    total.set_cpu_cores(config_.cpu_cores > 0 ? config_.cpu_cores : 4);
    total.set_ram_mb(config_.ram_mb > 0 ? config_.ram_mb : 8192);
    total.set_gpu_count(config_.gpu_count);
    
    auto id_opt = grpc_client_->register_node("agent-host", "127.0.0.1", 0, total);
    if (!id_opt) {
        throw std::runtime_error("Failed to register with master node");
    }
    
    node_id_ = *id_opt;
    LOG_INFO("Registered with master. Node ID: {}", node_id_);

    running_ = true;
    heartbeat_thread_ = std::thread(&AgentDaemon::heartbeat_loop, this);
    polling_thread_ = std::thread(&AgentDaemon::job_polling_loop, this);
}

void AgentDaemon::stop() {
    if (!running_) return;
    running_ = false;
    
    if (heartbeat_thread_.joinable()) heartbeat_thread_.join();
    if (polling_thread_.joinable()) polling_thread_.join();
    
    LOG_INFO("Agent daemon stopped");
}

void AgentDaemon::heartbeat_loop() {
    while (running_) {
        // Collect real metrics
        auto snap = monitor_->collect(node_id_);
        
        ResourceSpec avail;
        avail.set_cpu_cores(4); // Fake available
        avail.set_ram_mb(8192);
        
        auto result = grpc_client_->send_heartbeat(
            node_id_, 
            avail, 
            executor_->get_running_count(),
            snap.cpu_usage_percent(),
            snap.ram_usage_percent()
        );
        
        if (!result.ok) {
            LOG_WARN("Heartbeat failed, master might be down");
            // Could trigger re-registration logic here
        } else {
            for (const auto& jid : result.jobs_to_cancel) {
                LOG_INFO("Master requested cancellation of job {}", jid);
                executor_->cancel(jid);
            }
        }
        
        for(uint32_t i=0; i<config_.heartbeat_interval_sec * 10 && running_; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void AgentDaemon::job_polling_loop() {
    while (running_) {
        ResourceSpec avail;
        avail.set_cpu_cores(4);
        avail.set_ram_mb(8192);
        
        std::string job_id;
        auto spec_opt = grpc_client_->fetch_job(node_id_, avail, job_id);
        
        if (spec_opt) {
            LOG_INFO("Fetched new job {} from master", job_id);
            executor_->execute_async(job_id, *spec_opt);
        } else {
            // Sleep briefly if no jobs
            for(int i=0; i<10 && running_; i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
}

} // namespace certosc
