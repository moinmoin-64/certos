#pragma once

#include "certosc/master.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <boost/json.hpp>
#include <string>
#include <memory>

namespace certosc {

class GrpcBridge {
public:
    explicit GrpcBridge(const std::string& master_address);

    boost::json::object list_nodes(const std::string& token);
    boost::json::object list_jobs(const std::string& token);
    boost::json::object submit_job(const std::string& token, const boost::json::object& req_body);
    boost::json::object cancel_job(const std::string& token, const std::string& job_id);
    boost::json::object get_cluster_status(const std::string& token);
    
    // Auth
    boost::json::object login(const boost::json::object& req_body);
    
    // Requests
    boost::json::object submit_public_request(const JobRequest& request);
    boost::json::object admin_list_requests(const std::string& token);
    boost::json::object admin_review_request(const std::string& token, const std::string& req_id, bool approve, const std::string& comment);
    boost::json::object update_request_archive(const std::string& req_id, const std::string& archive_path);
    
    // User Management
    boost::json::object create_user(const std::string& token, const boost::json::object& req_body);
    boost::json::object admin_list_users(const std::string& token);
    boost::json::object admin_delete_user(const std::string& token, const std::string& username);

private:
    std::unique_ptr<MasterService::Stub> stub_;
    
    grpc::ClientContext* setup_context(grpc::ClientContext& context, const std::string& token);
};

} // namespace certosc
