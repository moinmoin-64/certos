#pragma once

#include "certosc/agent.grpc.pb.h"
#include "certosc/common/types.h"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <optional>

namespace certosc {

class AgentGrpcClient {
public:
    explicit AgentGrpcClient(const std::string& target_address);

    std::optional<std::string> register_node(const std::string& hostname, const std::string& ip, uint16_t port, const ResourceSpec& total_res);
    // Sends a heartbeat and returns a list of job IDs the master wants cancelled
    struct HeartbeatResult {
        bool ok;
        std::vector<std::string> jobs_to_cancel;
    };
    HeartbeatResult send_heartbeat(const std::string& node_id, const ResourceSpec& avail_res, uint32_t running_jobs, double cpu_pct, double ram_pct);
    
    std::optional<JobSpec> fetch_job(const std::string& node_id, const ResourceSpec& avail_res, std::string& out_job_id);
    
    bool report_job_started(const std::string& node_id, const std::string& job_id, uint32_t pid);
    bool report_job_completed(const std::string& node_id, const std::string& job_id, int32_t exit_code, const std::string& stdout_data, const std::string& stderr_data);
    bool report_job_failed(const std::string& node_id, const std::string& job_id, const std::string& error_msg, int32_t exit_code);

private:
    std::unique_ptr<AgentService::Stub> stub_;
};

} // namespace certosc
