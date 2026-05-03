#include "certosc/agent/agent_grpc_client.h"
#include "certosc/common/logging.h"

namespace certosc {

AgentGrpcClient::AgentGrpcClient(const std::string& target_address) {
    auto channel = grpc::CreateChannel(target_address, grpc::InsecureChannelCredentials());
    stub_ = AgentService::NewStub(channel);
}

std::optional<std::string> AgentGrpcClient::register_node(const std::string& hostname, const std::string& ip, uint16_t port, const ResourceSpec& total_res) {
    RegisterNodeRequest req;
    req.set_hostname(hostname);
    req.set_ip_address(ip);
    req.set_port(port);
    req.mutable_total_resources()->CopyFrom(total_res);
    req.set_os_info("Linux (Agent)"); // Simple stub
    
    RegisterNodeResponse res;
    grpc::ClientContext context;
    // Add fake auth token if needed
    context.AddMetadata("authorization", "Bearer node-token");
    
    grpc::Status status = stub_->RegisterNode(&context, req, &res);
    if (status.ok() && res.success()) {
        return res.node_id();
    } else {
        LOG_ERROR("RegisterNode failed: {} (code: {})", status.error_message(), static_cast<int>(status.error_code()));
        return std::nullopt;
    }
}

AgentGrpcClient::HeartbeatResult AgentGrpcClient::send_heartbeat(const std::string& node_id, const ResourceSpec& avail_res, uint32_t running_jobs, double cpu_pct, double ram_pct) {
    HeartbeatRequest req;
    req.set_node_id(node_id);
    req.mutable_available_resources()->CopyFrom(avail_res);
    req.set_running_jobs(running_jobs);
    req.set_cpu_usage_percent(cpu_pct);
    req.set_ram_usage_percent(ram_pct);
    req.set_timestamp(now_unix());
    
    HeartbeatResponse res;
    grpc::ClientContext context;
    
    grpc::Status status = stub_->SendHeartbeat(&context, req, &res);
    
    HeartbeatResult result;
    result.ok = status.ok() && res.acknowledged();
    if (result.ok) {
        for (const auto& jid : res.jobs_to_cancel()) {
            result.jobs_to_cancel.push_back(jid);
        }
    }
    return result;
}

std::optional<JobSpec> AgentGrpcClient::fetch_job(const std::string& node_id, const ResourceSpec& avail_res, std::string& out_job_id) {
    FetchJobRequest req;
    req.set_node_id(node_id);
    req.mutable_available_resources()->CopyFrom(avail_res);
    
    FetchJobResponse res;
    grpc::ClientContext context;
    
    grpc::Status status = stub_->FetchJob(&context, req, &res);
    if (status.ok() && res.has_job()) {
        out_job_id = res.job_id();
        return res.spec();
    }
    return std::nullopt;
}

bool AgentGrpcClient::report_job_started(const std::string& node_id, const std::string& job_id, uint32_t pid) {
    JobStartedReport req;
    req.set_node_id(node_id);
    req.set_job_id(job_id);
    req.set_pid(pid);
    req.set_started_at(now_unix());
    
    StatusResponse res;
    grpc::ClientContext context;
    grpc::Status status = stub_->ReportJobStarted(&context, req, &res);
    return status.ok() && res.success();
}

bool AgentGrpcClient::report_job_completed(const std::string& node_id, const std::string& job_id, int32_t exit_code, const std::string& stdout_data, const std::string& stderr_data) {
    JobCompletedReport req;
    req.set_node_id(node_id);
    req.set_job_id(job_id);
    req.set_exit_code(exit_code);
    req.set_completed_at(now_unix());
    req.set_stdout_data(stdout_data);
    req.set_stderr_data(stderr_data);
    
    StatusResponse res;
    grpc::ClientContext context;
    grpc::Status status = stub_->ReportJobCompleted(&context, req, &res);
    return status.ok() && res.success();
}

bool AgentGrpcClient::report_job_failed(const std::string& node_id, const std::string& job_id, const std::string& error_msg, int32_t exit_code) {
    JobFailedReport req;
    req.set_node_id(node_id);
    req.set_job_id(job_id);
    req.set_error_message(error_msg);
    req.set_exit_code(exit_code);
    req.set_failed_at(now_unix());
    
    StatusResponse res;
    grpc::ClientContext context;
    grpc::Status status = stub_->ReportJobFailed(&context, req, &res);
    return status.ok() && res.success();
}

} // namespace certosc
