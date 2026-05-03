cmake_minimum_required(VERSION 3.20..3.30)
project(CertOS
    VERSION 1.0.0
    DESCRIPTION "HPC Cloud Operating System - Distributed Cluster Management"
    LANGUAGES CXX
)

# ─── C++ Standard ───────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── Build Type ─────────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ─── Compiler Warnings ─────────────────────────────────────────────────────
add_compile_options(
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Release>:-O2>
)

# ─── Find Dependencies ─────────────────────────────────────────────────────
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
# Boost components (vcpkg specific config mode)
find_package(boost_asio CONFIG REQUIRED)
find_package(boost_beast CONFIG REQUIRED)
find_package(boost_json CONFIG REQUIRED)

# jwt-cpp is header-only, find it
find_path(JWT_CPP_INCLUDE_DIRS "jwt-cpp/jwt.h")

# ─── Protobuf/gRPC code generation ─────────────────────────────────────────
# Find protoc and grpc_cpp_plugin
find_program(_PROTOBUF_PROTOC protoc)
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

# ─── Proto library ──────────────────────────────────────────────────────────
add_subdirectory(proto)

# ─── Libraries ──────────────────────────────────────────────────────────────
add_subdirectory(lib)

# ─── Executables ────────────────────────────────────────────────────────────
add_subdirectory(master)
add_subdirectory(agent)
add_subdirectory(gateway)

# ─── Tests ──────────────────────────────────────────────────────────────────
option(CERTOSC_BUILD_TESTS "Build tests" ON)
if(CERTOSC_BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_subdirectory(tests)
endif()

# ─── Install ────────────────────────────────────────────────────────────────
install(TARGETS certosc-master certosc-agent certosc-gateway
    RUNTIME DESTINATION bin
)
install(DIRECTORY web/ DESTINATION share/certosc/web)
install(DIRECTORY config/ DESTINATION etc/certosc)

message(STATUS "")
message(STATUS "╔══════════════════════════════════════════════════╗")
message(STATUS "║          CertOS HPC Cloud OS v${PROJECT_VERSION}            ║")
message(STATUS "║  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "║  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "║  Tests: ${CERTOSC_BUILD_TESTS}")
message(STATUS "╚══════════════════════════════════════════════════╝")
message(STATUS "")
