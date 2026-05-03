#include "certosc/gateway/grpc_bridge.h"
#include "certosc/common/logging.h"

namespace certosc {

GrpcBridge::GrpcBridge(const std::string& master_address) {
    auto channel = grpc::CreateChannel(master_address, grpc::InsecureChannelCredentials());
    stub_ = MasterService::NewStub(channel);
}

grpc::ClientContext* GrpcBridge::setup_context(grpc::ClientContext& context, const std::string& token) {
    if (!token.empty()) {
        context.AddMetadata("authorization", "Bearer " + token);
    }
    return &context;
}

boost::json::object GrpcBridge::login(const boost::json::object& req_body) {
    Credentials req;
    if (req_body.contains("username")) req.set_username(boost::json::value_to<std::string>(req_body.at("username")));
    if (req_body.contains("password")) req.set_password(boost::json::value_to<std::string>(req_body.at("password")));

    AuthToken res;
    grpc::ClientContext context;
    grpc::Status status = stub_->Login(&context, req, &res);

    boost::json::object json_res;
    if (status.ok()) {
        json_res["success"] = true;
        json_res["access_token"] = res.access_token();
        
        boost::json::object user;
        user["username"] = res.user().username();
        user["role"] = (res.user().role() == USER_ROLE_ADMIN) ? "admin" : "user";
        json_res["user"] = user;
    } else {
        json_res["success"] = false;
        json_res["error"] = status.error_message();
    }
    return json_res;
}

boost::json::object GrpcBridge::list_nodes(const std::string& token) {
    Empty req;
    ListNodesResponse res;
    grpc::ClientContext context;
    setup_context(context, token);
    
    grpc::Status status = stub_->ListNodes(&context, req, &res);

    boost::json::object json_res;
    if (status.ok()) {
        boost::json::array nodes;
        for (const auto& n : res.nodes()) {
            boost::json::object node;
            node["id"] = n.node_id();
            node["hostname"] = n.hostname();
            node["ip"] = n.ip_address();
            node["status"] = (n.status() == NODE_STATUS_ONLINE) ? "online" : "offline";
            
            boost::json::object res_obj;
            res_obj["cpu_cores"] = n.available_resources().cpu_cores();
            res_obj["ram_mb"] = n.available_resources().ram_mb();
            node["resources"] = res_obj;
            
            nodes.push_back(node);
        }
        json_res["success"] = true;
        json_res["nodes"] = nodes;
    } else {
        json_res["success"] = false;
        json_res["error"] = status.error_message();
    }
    return json_res;
}

boost::json::object GrpcBridge::list_jobs(const std::string& token) {
    ListJobsRequest req;
    ListJobsResponse res;
    grpc::ClientContext context;
    setup_context(context, token);
    
    grpc::Status status = stub_->ListJobs(&context, req, &res);

    boost::json::object json_res;
    if (status.ok()) {
        boost::json::array jobs;
        for (const auto& j : res.jobs()) {
            boost::json::object job;
            job["job_id"] = j.job_id();
            job["username"] = j.username();
            job["status"] = (int)j.status();
            job["submitted_at"] = j.submitted_at();
            
            boost::json::object spec;
            spec["name"] = j.spec().name();
            spec["command"] = j.spec().command();
            job["spec"] = spec;
            
            job["assigned_node_id"] = j.assigned_node_id();
            jobs.push_back(job);
        }
        json_res["success"] = true;
        json_res["jobs"] = jobs;
    } else {
        json_res["success"] = false;
        json_res["error"] = status.error_message();
    }
    return json_res;
}

boost::json::object GrpcBridge::submit_job(const std::string& token, const boost::json::object& req_body) {
    SubmitJobRequest req;
    auto spec = req.mutable_spec();
    
    if (req_body.contains("name")) spec->set_name(boost::json::value_to<std::string>(req_body.at("name")));
    if (req_body.contains("command")) spec->set_command(boost::json::value_to<std::string>(req_body.at("command")));
    
    auto resources = spec->mutable_resources();
    resources->set_cpu_cores(1);
    resources->set_ram_mb(1024);
    
    JobInfo res;
    grpc::ClientContext context;
    setup_context(context, token);
    
    grpc::Status status = stub_->SubmitJob(&context, req, &res);

    boost::json::object json_res;
    if (status.ok()) {
        json_res["success"] = true;
        json_res["job_id"] = res.job_id();
    } else {
        json_res["success"] = false;
        json_res["error"] = status.error_message();
    }
    return json_res;
}

boost::json::object GrpcBridge::get_cluster_status(const std::string& token) {
    Empty req;
    ClusterInfo res;
    grpc::ClientContext context;
    setup_context(context, token);
    
    grpc::Status status = stub_->GetClusterStatus(&context, req, &res);

    boost::json::object json_res;
    if (status.ok()) {
        json_res["success"] = true;
        json_res["online_nodes"] = res.online_nodes();
        json_res["running_jobs"] = res.running_jobs();
        json_res["queued_jobs"] = res.queued_jobs();
    } else {
        json_res["success"] = false;
        json_res["error"] = status.error_message();
    }
    return json_res;
}

boost::json::object GrpcBridge::cancel_job(const std::string& token, const std::string& job_id) {
    CancelJobRequest req;
    req.set_job_id(job_id);
    
    StatusResponse res;
    grpc::ClientContext context;
    setup_context(context, token);
    
    grpc::Status status = stub_->CancelJob(&context, req, &res);

    boost::json::object json_res;
    if (status.ok()) {
        json_res["success"] = res.success();
        json_res["message"] = res.message();
    } else {
        json_res["success"] = false;
        json_res["error"] = status.error_message();
    }
    return json_res;
}

boost::json::object GrpcBridge::submit_public_request(const JobRequest& request) {
    StatusResponse res;
    grpc::ClientContext context;
    grpc::Status status = stub_->SubmitPublicRequest(&context, request, &res);

    boost::json::object json_res;
    json_res["success"] = status.ok() && res.success();
    if (json_res["success"].as_bool()) {
        json_res["request_id"] = request.request_id();
        json_res["message"] = res.message();
    } else {
        json_res["error"] = status.ok() ? res.message() : status.error_message();
    }
    return json_res;
}

boost::json::object GrpcBridge::admin_list_requests(const std::string& token) {
    Empty req;
    ListJobRequestsResponse res;
    grpc::ClientContext context;
    setup_context(context, token);
    
    grpc::Status status = stub_->AdminListRequests(&context, req, &res);

    boost::json::object json_res;
    if (status.ok()) {
        boost::json::array requests;
        for (const auto& r : res.requests()) {
            boost::json::object req;
            req["request_id"] = r.request_id();
            req["applicant_name"] = r.applicant_name();
            req["applicant_email"] = r.applicant_email();
            req["project_goal"] = r.project_goal();
            req["status"] = (int)r.status();
            req["submitted_at"] = r.submitted_at();

            boost::json::object spec;
            boost::json::object resources;
            resources["cpu_cores"] = r.requested_spec().resources().cpu_cores();
            resources["ram_mb"] = r.requested_spec().resources().ram_mb();
            spec["resources"] = resources;
            req["requested_spec"] = spec;

            requests.push_back(req);
        }
        json_res["success"] = true;
        json_res["requests"] = requests;
    } else {
        json_res["success"] = false;
        json_res["error"] = status.error_message();
    }
    return json_res;
}

boost::json::object GrpcBridge::admin_review_request(const std::string& token, const std::string& req_id, bool approve, const std::string& comment) {
    ReviewRequest req;
    req.set_request_id(req_id);
    req.set_approve(approve);
    req.set_comment(comment);
    
    StatusResponse res;
    grpc::ClientContext context;
    setup_context(context, token);
    
    grpc::Status status = stub_->AdminReviewRequest(&context, req, &res);

    boost::json::object json_res;
    json_res["success"] = status.ok() && res.success();
    if (!json_res["success"].as_bool()) {
        json_res["error"] = status.ok() ? res.message() : status.error_message();
    } else {
        json_res["message"] = res.message();
    }
    return json_res;
}

boost::json::object GrpcBridge::update_request_archive(const std::string& req_id, const std::string& archive_path) {
    JobRequest req;
    req.set_request_id(req_id);
    req.mutable_requested_spec()->set_archive_path(archive_path);
    
    StatusResponse res;
    grpc::ClientContext context;
    grpc::Status status = stub_->UpdateJobRequest(&context, req, &res);
    
    boost::json::object json_res;
    json_res["success"] = status.ok() && res.success();
    if (!json_res["success"].as_bool()) {
        json_res["error"] = status.ok() ? res.message() : status.error_message();
    }
    return json_res;
}

boost::json::object GrpcBridge::create_user(const std::string& token, const boost::json::object& req_body) {
    CreateUserRequest req;
    req.set_username(boost::json::value_to<std::string>(req_body.at("username")));
    req.set_password(boost::json::value_to<std::string>(req_body.at("password")));
    req.set_role(req_body.at("role").as_string() == "admin" ? USER_ROLE_ADMIN : USER_ROLE_USER);

    UserInfo res;
    grpc::ClientContext context;
    setup_context(context, token);
    grpc::Status status = stub_->CreateUser(&context, req, &res);

    boost::json::object json_res;
    json_res["success"] = status.ok();
    if (status.ok()) {
        json_res["username"] = res.username();
    } else {
        json_res["error"] = status.error_message();
    }
    return json_res;
}

boost::json::object GrpcBridge::admin_list_users(const std::string& token) {
    Empty req;
    ListUsersResponse res;
    grpc::ClientContext context;
    setup_context(context, token);
    grpc::Status status = stub_->AdminListUsers(&context, req, &res);

    boost::json::object json_res;
    if (status.ok()) {
        boost::json::array users;
        for (const auto& u : res.users()) {
            boost::json::object user;
            user["username"] = u.username();
            user["role"] = (u.role() == USER_ROLE_ADMIN) ? "admin" : "user";
            users.push_back(user);
        }
        json_res["success"] = true;
        json_res["users"] = users;
    } else {
        json_res["success"] = false;
        json_res["error"] = status.error_message();
    }
    return json_res;
}

boost::json::object GrpcBridge::admin_delete_user(const std::string& token, const std::string& username) {
    DeleteUserRequest req;
    req.set_user_id(username);
    
    StatusResponse res;
    grpc::ClientContext context;
    setup_context(context, token);
    grpc::Status status = stub_->AdminDeleteUser(&context, req, &res);

    boost::json::object json_res;
    json_res["success"] = status.ok() && res.success();
    if (!json_res["success"].as_bool()) {
        json_res["error"] = status.ok() ? res.message() : status.error_message();
    }
    return json_res;
}

} // namespace certosc
