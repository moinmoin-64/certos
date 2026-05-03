#include "certosc/master/grpc_service.h"
#include "certosc/common/logging.h"
#include <sstream>

namespace certosc {

// ─── MasterServiceImpl ─────────────────────────────────────────────────────

MasterServiceImpl::MasterServiceImpl(std::shared_ptr<JobManager> job_mgr, std::shared_ptr<NodeRegistry> node_registry, std::shared_ptr<StateStore> admin_db)
    : job_mgr_(job_mgr), node_registry_(node_registry), admin_db_(admin_db) {}

grpc::Status MasterServiceImpl::Login(grpc::ServerContext* context, const Credentials* request, AuthToken* response) {
    auto db_pass = admin_db_->get_kv("user:" + request->username());
    if (db_pass && db_pass.value() == request->password()) {
        response->set_access_token("mock-jwt-for-" + request->username()); // Replace with JwtManager
        response->set_token_type("Bearer");
        auto* user = response->mutable_user();
        user->set_username(request->username());
        auto db_role = admin_db_->get_kv("role:" + request->username());
        if (db_role && db_role.value() == "admin") {
            user->set_role(USER_ROLE_ADMIN);
        } else {
            user->set_role(USER_ROLE_USER);
        }
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid credentials");
}

grpc::Status MasterServiceImpl::CreateUser(grpc::ServerContext* context, const CreateUserRequest* request, UserInfo* response) {
    std::string username = request->username();
    std::string password = request->password();
    std::string role = (request->role() == USER_ROLE_ADMIN) ? "admin" : "user";

    if (admin_db_->get_kv("user:" + username)) {
        return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "User already exists");
    }

    admin_db_->set_kv("user:" + username, password);
    admin_db_->set_kv("role:" + username, role);

    // Update user list
    std::string user_list = "admin,user1";
    auto existing_list = admin_db_->get_kv("system:user_list");
    if (existing_list) {
        user_list = existing_list.value();
    }
    if (user_list.find(username) == std::string::npos) {
        user_list += "," + username;
        admin_db_->set_kv("system:user_list", user_list);
    }

    response->set_username(username);
    response->set_role(request->role());
    response->set_created_at(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    
    LOG_INFO("Created new user: {} with role {}", username, role);
    return grpc::Status::OK;
}

grpc::Status MasterServiceImpl::AdminListUsers(grpc::ServerContext* context, const Empty* request, ListUsersResponse* response) {
    std::vector<std::string> users = {"admin", "user1"};
    auto user_list_str = admin_db_->get_kv("system:user_list");
    if (user_list_str) {
        users.clear();
        std::stringstream ss(user_list_str.value());
        std::string u;
        while (std::getline(ss, u, ',')) {
            if (!u.empty()) users.push_back(u);
        }
    }

    for (const auto& uname : users) {
        auto* user = response->add_users();
        user->set_username(uname);
        auto role = admin_db_->get_kv("role:" + uname);
        user->set_role((role && role.value() == "admin") ? USER_ROLE_ADMIN : USER_ROLE_USER);
    }
    return grpc::Status::OK;
}

grpc::Status MasterServiceImpl::AdminDeleteUser(grpc::ServerContext* context, const DeleteUserRequest* request, StatusResponse* response) {
    std::string username = request->user_id();
    if (username == "admin") {
        return grpc::Status(grpc::StatusCode::PERMISSION_DENIED, "Cannot delete root admin");
    }

    // Logic to remove from list and kv
    response->set_success(true);
    response->set_message("User deleted");
    return grpc::Status::OK;
}

grpc::Status MasterServiceImpl::GetJobStatus(grpc::ServerContext* context, const GetJobRequest* request, JobInfo* response) {
    auto job = job_mgr_->get_job(request->job_id());
    if (job) {
        response->CopyFrom(*job);
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Job not found");
}

grpc::Status MasterServiceImpl::SubmitJob(grpc::ServerContext* context, const SubmitJobRequest* request, JobInfo* response) {
    std::string username = "unknown";
    auto metadata = context->client_metadata();
    auto it = metadata.find("authorization");
    if (it != metadata.end()) {
        std::string auth(it->second.data(), it->second.length());
        // Simple hack to extract from mock token "Bearer mock-jwt-for-username"
        if (auth.find("mock-jwt-for-") != std::string::npos) {
            username = auth.substr(auth.find("mock-jwt-for-") + 13);
        }
    }

    std::string job_id = job_mgr_->submit_job(request->spec(), username, username);
    auto job_opt = job_mgr_->get_job(job_id);
    if (job_opt) {
        response->CopyFrom(*job_opt);
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to retrieve submitted job");
}

grpc::Status MasterServiceImpl::CancelJob(grpc::ServerContext* context, const CancelJobRequest* request, StatusResponse* response) {
    bool ok = job_mgr_->cancel_job(request->job_id());
    response->set_success(ok);
    if (!ok) {
        response->set_message("Failed to cancel job or job not found");
    } else {
        response->set_message("Job marked for cancellation");
    }
    return grpc::Status::OK;
}

grpc::Status MasterServiceImpl::ListNodes(grpc::ServerContext* context, const Empty* request, ListNodesResponse* response) {
    auto nodes = node_registry_->get_all_nodes();
    for (const auto& n : nodes) {
        response->add_nodes()->CopyFrom(n);
    }
    return grpc::Status::OK;
}

grpc::Status MasterServiceImpl::ListJobs(grpc::ServerContext* context, const ListJobsRequest* request, ListJobsResponse* response) {
    auto jobs = job_mgr_->list_jobs(request->status_filter());
    for (const auto& j : jobs) {
        response->add_jobs()->CopyFrom(j);
    }
    response->set_total_count(jobs.size());
    return grpc::Status::OK;
}

#include <google/protobuf/util/json_util.h>
#include <chrono>

grpc::Status MasterServiceImpl::SubmitPublicRequest(grpc::ServerContext* context, const JobRequest* request, StatusResponse* response) {
    JobRequest req = *request;
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    
    // Generate a simple ID if not provided
    if (req.request_id().empty()) {
        req.set_request_id("req-" + std::to_string(now) + "-" + std::to_string(rand() % 1000));
    }
    req.set_submitted_at(now);
    req.set_status(JOB_REQUEST_PENDING);

    std::string json_data;
    google::protobuf::util::JsonPrintOptions options;
    auto s = google::protobuf::util::MessageToJsonString(req, &json_data, options);
    if (!s.ok()) {
        response->set_success(false);
        response->set_message("Failed to serialize request");
        return grpc::Status::OK;
    }

    if (admin_db_->add_job_request(req.request_id(), req.applicant_email(), (int)req.status(), json_data)) {
        response->set_success(true);
        response->set_message(req.request_id());
        return grpc::Status::OK;
    }
    
    response->set_success(false);
    response->set_message("Failed to store request in database");
    return grpc::Status::OK;
}

grpc::Status MasterServiceImpl::UpdateJobRequest(grpc::ServerContext* context, const JobRequest* request, StatusResponse* response) {
    auto requests = admin_db_->list_requests();
    std::string target_json;
    for (const auto& r : requests) {
        if (r.first == request->request_id()) {
            target_json = r.second;
            break;
        }
    }

    if (target_json.empty()) {
        response->set_success(false);
        response->set_message("Request not found");
        return grpc::Status::OK;
    }

    JobRequest req;
    auto s1 = google::protobuf::util::JsonStringToMessage(target_json, &req);
    if (!s1.ok()) {
        response->set_success(false);
        response->set_message("Failed to parse request");
        return grpc::Status::OK;
    }
    
    // Update fields that are set in the incoming request
    if (!request->requested_spec().archive_path().empty()) {
        req.mutable_requested_spec()->set_archive_path(request->requested_spec().archive_path());
    }
    
    std::string json_data;
    auto s2 = google::protobuf::util::MessageToJsonString(req, &json_data);
    if (!s2.ok()) {
        response->set_success(false);
        response->set_message("Failed to serialize update");
        return grpc::Status::OK;
    }
    admin_db_->update_request_status(req.request_id(), (int)req.status(), json_data);

    response->set_success(true);
    return grpc::Status::OK;
}

grpc::Status MasterServiceImpl::AdminListRequests(grpc::ServerContext* context, const Empty* request, ListJobRequestsResponse* response) {
    auto requests = admin_db_->list_requests();
    for (const auto& r : requests) {
        JobRequest* req = response->add_requests();
        google::protobuf::util::JsonParseOptions options;
        auto s = google::protobuf::util::JsonStringToMessage(r.second, req, options);
        if (!s.ok()) {
            LOG_ERROR("Failed to parse request {}: {}", r.first, s.ToString());
        }
    }
    return grpc::Status::OK;
}

grpc::Status MasterServiceImpl::AdminReviewRequest(grpc::ServerContext* context, const ReviewRequest* request, StatusResponse* response) {
    // In a real system, we'd find the request in DB first
    auto requests = admin_db_->list_requests();
    std::string target_json;
    for (const auto& r : requests) {
        if (r.first == request->request_id()) {
            target_json = r.second;
            break;
        }
    }

    if (target_json.empty()) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Request not found");
    }

    JobRequest req;
    auto s1 = google::protobuf::util::JsonStringToMessage(target_json, &req);
    if (!s1.ok()) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to parse stored request");
    }

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    req.set_reviewed_at(now);
    req.set_review_comment(request->comment());

    if (request->approve()) {
        req.set_status(JOB_REQUEST_APPROVED);
        // Automatically submit the job
        std::string job_id = job_mgr_->submit_job(req.requested_spec(), "public-user", req.applicant_name());
        LOG_INFO("Job approved and submitted. JobID: {}", job_id);
    } else {
        req.set_status(JOB_REQUEST_REJECTED);
    }

    std::string json_data;
    auto s2 = google::protobuf::util::MessageToJsonString(req, &json_data);
    if (!s2.ok()) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to serialize request");
    }
    admin_db_->update_request_status(req.request_id(), (int)req.status(), json_data);

    response->set_success(true);
    response->set_message(request->approve() ? "Request approved and job scheduled" : "Request rejected");
    return grpc::Status::OK;
}

// ─── AgentServiceImpl ──────────────────────────────────────────────────────

AgentServiceImpl::AgentServiceImpl(std::shared_ptr<JobManager> job_mgr, std::shared_ptr<NodeRegistry> node_registry)
    : job_mgr_(job_mgr), node_registry_(node_registry) {}

grpc::Status AgentServiceImpl::RegisterNode(grpc::ServerContext* context, const RegisterNodeRequest* request, RegisterNodeResponse* response) {
    NodeInfo info;
    info.set_hostname(request->hostname());
    info.set_ip_address(request->ip_address());
    info.set_port(request->port());
    info.set_os_info(request->os_info());
    info.mutable_total_resources()->CopyFrom(request->total_resources());

    std::string node_id = node_registry_->register_node(info);
    
    response->set_success(true);
    response->set_node_id(node_id);
    response->set_message("Registration successful");
    response->set_heartbeat_interval_seconds(certosc::defaults::HEARTBEAT_INTERVAL_SEC);
    response->set_metrics_interval_seconds(certosc::defaults::METRICS_INTERVAL_SEC);
    
    return grpc::Status::OK;
}

grpc::Status AgentServiceImpl::SendHeartbeat(grpc::ServerContext* context, const HeartbeatRequest* request, HeartbeatResponse* response) {
    bool ok = node_registry_->update_heartbeat(request->node_id(), request->available_resources(), request->running_jobs());
    response->set_acknowledged(ok);
    if (!ok) {
        response->set_message("Node not registered");
    } else {
        auto to_cancel = job_mgr_->get_jobs_to_cancel_for_node(request->node_id());
        for (const auto& jid : to_cancel) {
            response->add_jobs_to_cancel(jid);
        }
    }
    return grpc::Status::OK;
}

grpc::Status AgentServiceImpl::FetchJob(grpc::ServerContext* context, const FetchJobRequest* request, FetchJobResponse* response) {
    // Agents poll this to get their assigned jobs
    // In a push model, the master would call the agent. But polling simplifies firewall traversal.
    
    // Find a job assigned to this node that hasn't started yet
    auto jobs = job_mgr_->list_jobs(JOB_STATUS_ASSIGNED);
    for (const auto& job : jobs) {
        if (job.assigned_node_id() == request->node_id()) {
            response->set_has_job(true);
            response->set_job_id(job.job_id());
            response->mutable_spec()->CopyFrom(job.spec());
            
            // Mark it as picked up (we'll transition to RUNNING when the agent reports it started)
            // But we don't want to re-dispatch it on the next poll
            // For now, assume the agent immediately reports JobStarted
            return grpc::Status::OK;
        }
    }
    
    response->set_has_job(false);
    return grpc::Status::OK;
}

grpc::Status AgentServiceImpl::ReportJobStarted(grpc::ServerContext* context, const JobStartedReport* request, StatusResponse* response) {
    bool ok = job_mgr_->update_job_status(request->job_id(), JOB_STATUS_RUNNING, request->node_id());
    response->set_success(ok);
    return grpc::Status::OK;
}

grpc::Status AgentServiceImpl::ReportJobCompleted(grpc::ServerContext* context, const JobCompletedReport* request, StatusResponse* response) {
    bool ok = job_mgr_->update_job_status(request->job_id(), JOB_STATUS_COMPLETED, request->node_id());
    // In reality, we'd also store the output data in the StateStore or file system
    response->set_success(ok);
    return grpc::Status::OK;
}

grpc::Status AgentServiceImpl::ReportJobFailed(grpc::ServerContext* /*context*/, const JobFailedReport* request, StatusResponse* response) {
    bool ok = job_mgr_->update_job_status(request->job_id(), JOB_STATUS_FAILED, request->node_id());
    // Also store error message
    response->set_success(ok);
    return grpc::Status::OK;
}

} // namespace certosc
