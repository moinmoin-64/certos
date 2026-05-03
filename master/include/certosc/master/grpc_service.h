#pragma once

#include "certosc/master.grpc.pb.h"
#include "certosc/agent.grpc.pb.h"
#include "certosc/master/job_manager.h"
#include "certosc/master/node_registry.h"
#include "certosc/master/state_store.h"
#include <memory>

namespace certosc {

class MasterServiceImpl final : public MasterService::Service {
public:
    MasterServiceImpl(std::shared_ptr<JobManager> job_mgr, std::shared_ptr<NodeRegistry> node_registry, std::shared_ptr<StateStore> admin_db);

    grpc::Status Login(grpc::ServerContext* context, const Credentials* request, AuthToken* response) override;
    grpc::Status CreateUser(grpc::ServerContext* context, const CreateUserRequest* request, UserInfo* response) override;
    grpc::Status SubmitJob(grpc::ServerContext* context, const SubmitJobRequest* request, JobInfo* response) override;
    grpc::Status GetJobStatus(grpc::ServerContext* context, const GetJobRequest* request, JobInfo* response) override;
    grpc::Status CancelJob(grpc::ServerContext* context, const CancelJobRequest* request, StatusResponse* response) override;
    grpc::Status ListJobs(grpc::ServerContext* context, const ListJobsRequest* request, ListJobsResponse* response) override;
    grpc::Status ListNodes(grpc::ServerContext* context, const Empty* request, ListNodesResponse* response) override;
    
    // Admin & Public Request methods
    grpc::Status AdminListRequests(grpc::ServerContext* context, const Empty* request, ListJobRequestsResponse* response) override;
    grpc::Status AdminReviewRequest(grpc::ServerContext* context, const ReviewRequest* request, StatusResponse* response) override;
    
    grpc::Status SubmitPublicRequest(grpc::ServerContext* context, const JobRequest* request, StatusResponse* response) override;
    grpc::Status UpdateJobRequest(grpc::ServerContext* context, const JobRequest* request, StatusResponse* response) override;

    // ─── User Management ───────────────────────────────────────────────────
    grpc::Status AdminListUsers(grpc::ServerContext* context, const Empty* request, ListUsersResponse* response) override;
    grpc::Status AdminDeleteUser(grpc::ServerContext* context, const DeleteUserRequest* request, StatusResponse* response) override;

private:
    std::shared_ptr<JobManager> job_mgr_;
    std::shared_ptr<NodeRegistry> node_registry_;
    std::shared_ptr<StateStore> admin_db_;
};

class AgentServiceImpl final : public AgentService::Service {
public:
    AgentServiceImpl(std::shared_ptr<JobManager> job_mgr, std::shared_ptr<NodeRegistry> node_registry);

    grpc::Status RegisterNode(grpc::ServerContext* context, const RegisterNodeRequest* request, RegisterNodeResponse* response) override;
    grpc::Status SendHeartbeat(grpc::ServerContext* context, const HeartbeatRequest* request, HeartbeatResponse* response) override;
    grpc::Status FetchJob(grpc::ServerContext* context, const FetchJobRequest* request, FetchJobResponse* response) override;
    grpc::Status ReportJobStarted(grpc::ServerContext* context, const JobStartedReport* request, StatusResponse* response) override;
    grpc::Status ReportJobCompleted(grpc::ServerContext* context, const JobCompletedReport* request, StatusResponse* response) override;
    grpc::Status ReportJobFailed(grpc::ServerContext* context, const JobFailedReport* request, StatusResponse* response) override;

private:
    std::shared_ptr<JobManager> job_mgr_;
    std::shared_ptr<NodeRegistry> node_registry_;
};

} // namespace certosc
