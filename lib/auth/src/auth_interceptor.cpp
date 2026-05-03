#include "certosc/auth/auth_interceptor.h"
#include "certosc/common/logging.h"
#include <grpcpp/support/interceptor.h>
#include <grpcpp/grpcpp.h>

namespace certosc {

// ─── AuthInterceptor ───────────────────────────────────────────────────────

AuthInterceptor::AuthInterceptor(
    grpc::experimental::ServerRpcInfo* info,
    std::shared_ptr<JwtManager> jwt_manager,
    std::shared_ptr<RbacManager> rbac_manager,
    const std::unordered_set<std::string>& public_methods)
    : info_(info)
    , jwt_manager_(jwt_manager)
    , rbac_manager_(rbac_manager)
    , public_methods_(public_methods) {
}

void AuthInterceptor::Intercept(
    grpc::experimental::InterceptorBatchMethods* methods) {

    if (methods->QueryInterceptionHookPoint(
            grpc::experimental::InterceptionHookPoints::POST_RECV_INITIAL_METADATA)) {

        std::string method_name = info_->method();

        // Skip auth for public methods (Login, RegisterNode)
        if (public_methods_.count(method_name) > 0) {
            methods->Proceed();
            return;
        }

        // Extract the authorization metadata
        auto* metadata = methods->GetRecvInitialMetadata();
        auto it = metadata->find("authorization");
        if (it == metadata->end()) {
            LOG_WARN("Auth: missing authorization header for {}", method_name);
            // We'll let it proceed — the service method can check context
            methods->Proceed();
            return;
        }

        std::string auth_value(it->second.data(), it->second.size());

        // Strip "Bearer " prefix
        const std::string bearer_prefix = "Bearer ";
        if (auth_value.substr(0, bearer_prefix.size()) == bearer_prefix) {
            auth_value = auth_value.substr(bearer_prefix.size());
        }

        // Validate token
        auto claims = jwt_manager_->validate_token(auth_value);
        if (!claims) {
            LOG_WARN("Auth: invalid token for method {}", method_name);
        } else {
            LOG_TRACE("Auth: validated token for user '{}' (role={})",
                      claims->username, claims->role);
        }
    }

    methods->Proceed();
}

// ─── AuthInterceptorFactory ────────────────────────────────────────────────

AuthInterceptorFactory::AuthInterceptorFactory(
    std::shared_ptr<JwtManager> jwt_manager,
    std::shared_ptr<RbacManager> rbac_manager)
    : jwt_manager_(jwt_manager)
    , rbac_manager_(rbac_manager) {

    // Methods that don't require authentication
    public_methods_ = {
        "/certosc.MasterService/Login",
        "/certosc.MasterService/SubmitPublicRequest",
        "/certosc.MasterService/UpdateJobRequest",
        "/certosc.AgentService/RegisterNode",
        "/certosc.AgentService/SendHeartbeat",
    };
}

grpc::experimental::Interceptor*
AuthInterceptorFactory::CreateServerInterceptor(
    grpc::experimental::ServerRpcInfo* info) {
    return new AuthInterceptor(info, jwt_manager_, rbac_manager_, public_methods_);
}

} // namespace certosc
