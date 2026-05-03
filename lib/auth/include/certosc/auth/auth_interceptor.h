#pragma once

#include "certosc/auth/jwt_manager.h"
#include "certosc/auth/rbac.h"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <unordered_set>
#include <string>

namespace certosc {

/// gRPC server interceptor that validates JWT tokens on incoming requests
class AuthInterceptor : public grpc::experimental::Interceptor {
public:
    AuthInterceptor(grpc::experimental::ServerRpcInfo* info,
                    std::shared_ptr<JwtManager> jwt_manager,
                    std::shared_ptr<RbacManager> rbac_manager,
                    const std::unordered_set<std::string>& public_methods);

    void Intercept(grpc::experimental::InterceptorBatchMethods* methods) override;

private:
    grpc::experimental::ServerRpcInfo* info_;
    std::shared_ptr<JwtManager> jwt_manager_;
    std::shared_ptr<RbacManager> rbac_manager_;
    const std::unordered_set<std::string>& public_methods_;
};

/// Factory for creating AuthInterceptor instances
class AuthInterceptorFactory : public grpc::experimental::ServerInterceptorFactoryInterface {
public:
    AuthInterceptorFactory(std::shared_ptr<JwtManager> jwt_manager,
                           std::shared_ptr<RbacManager> rbac_manager);

    grpc::experimental::Interceptor* CreateServerInterceptor(
        grpc::experimental::ServerRpcInfo* info) override;

private:
    std::shared_ptr<JwtManager> jwt_manager_;
    std::shared_ptr<RbacManager> rbac_manager_;
    std::unordered_set<std::string> public_methods_;
};

} // namespace certosc
