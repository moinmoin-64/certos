#include "certosc/master/master_server.h"
#include "certosc/common/logging.h"
#include "certosc/auth/auth_interceptor.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

namespace certosc {

MasterServer::MasterServer(const MasterConfig& config) : config_(config) {
    admin_db_ = std::make_shared<StateStore>("certosc_admin.db");
    admin_db_->init();
    
    jobs_db_ = std::make_shared<StateStore>("certosc_jobs.db");
    jobs_db_->init();

    // Create a dummy admin user in admin_db_ for testing
    admin_db_->set_kv("user:admin", "admin123");
    admin_db_->set_kv("role:admin", "admin");
    admin_db_->set_kv("user:user1", "user123");
    admin_db_->set_kv("role:user1", "user");
    admin_db_->set_kv("system:user_list", "admin,user1");

    node_registry_ = std::make_shared<NodeRegistry>();
    job_mgr_ = std::make_shared<JobManager>(jobs_db_);
    resource_mgr_ = std::make_shared<ResourceManager>();
    fair_share_mgr_ = std::make_shared<FairShareManager>(config.fair_share_decay_hours);
    
    scheduler_ = std::make_shared<SchedulerEngine>(job_mgr_, node_registry_, fair_share_mgr_);
    
    jwt_mgr_ = std::make_shared<JwtManager>(config.jwt_secret, config.jwt_expiry_sec);
    rbac_mgr_ = std::make_shared<RbacManager>();

    master_service_ = std::make_unique<MasterServiceImpl>(job_mgr_, node_registry_, admin_db_);
    agent_service_ = std::make_unique<AgentServiceImpl>(job_mgr_, node_registry_);
}

MasterServer::~MasterServer() {
    stop();
}

void MasterServer::start() {
    scheduler_->start(config_.scheduler_interval_ms);

    std::string server_address = config_.bind_address + ":" + std::to_string(config_.grpc_port);
    grpc::ServerBuilder builder;

    // Optional: Add TLS credentials
    std::shared_ptr<grpc::ServerCredentials> creds;
    if (config_.enable_tls) {
        LOG_WARN("TLS enabled but not fully implemented in this phase");
        creds = grpc::InsecureServerCredentials();
    } else {
        creds = grpc::InsecureServerCredentials();
    }

    builder.AddListeningPort(server_address, creds);

    // Register services
    builder.RegisterService(master_service_.get());
    builder.RegisterService(agent_service_.get());

    // Reflection for tools like grpcurl
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    // Add Auth Interceptor
    std::vector<std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>> interceptor_creators;
    interceptor_creators.push_back(std::make_unique<AuthInterceptorFactory>(jwt_mgr_, rbac_mgr_));
    builder.experimental().SetInterceptorCreators(std::move(interceptor_creators));

    grpc_server_ = builder.BuildAndStart();
    LOG_INFO("Master gRPC server listening on {}", server_address);
}

void MasterServer::wait() {
    if (grpc_server_) {
        grpc_server_->Wait();
    }
}

void MasterServer::stop() {
    scheduler_->stop();
    if (grpc_server_) {
        grpc_server_->Shutdown();
        LOG_INFO("Master gRPC server stopped");
    }
}

} // namespace certosc
