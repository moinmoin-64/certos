#pragma once

#include "certosc/common/config.h"
#include "certosc/master/state_store.h"
#include "certosc/master/node_registry.h"
#include "certosc/master/job_manager.h"
#include "certosc/master/resource_manager.h"
#include "certosc/master/scheduler/scheduler_engine.h"
#include "certosc/master/grpc_service.h"
#include "certosc/auth/jwt_manager.h"
#include "certosc/auth/rbac.h"

#include <memory>
#include <grpcpp/grpcpp.h>

namespace certosc {

class MasterServer {
public:
    explicit MasterServer(const MasterConfig& config);
    ~MasterServer();

    void start();
    void wait();
    void stop();

private:
    MasterConfig config_;
    
    std::shared_ptr<StateStore> admin_db_;
    std::shared_ptr<StateStore> jobs_db_;
    std::shared_ptr<NodeRegistry> node_registry_;
    std::shared_ptr<JobManager> job_mgr_;
    std::shared_ptr<ResourceManager> resource_mgr_;
    std::shared_ptr<FairShareManager> fair_share_mgr_;
    std::shared_ptr<SchedulerEngine> scheduler_;
    std::shared_ptr<JwtManager> jwt_mgr_;
    std::shared_ptr<RbacManager> rbac_mgr_;

    std::unique_ptr<MasterServiceImpl> master_service_;
    std::unique_ptr<AgentServiceImpl> agent_service_;
    std::unique_ptr<grpc::Server> grpc_server_;
};

} // namespace certosc
