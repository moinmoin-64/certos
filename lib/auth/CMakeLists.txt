add_library(certosc-auth STATIC
    src/jwt_manager.cpp
    src/rbac.cpp
    src/auth_interceptor.cpp
)

target_include_directories(certosc-auth PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${JWT_CPP_INCLUDE_DIRS}
)

target_link_libraries(certosc-auth PUBLIC
    certosc-common
    certosc-proto
    OpenSSL::SSL
    OpenSSL::Crypto
)
