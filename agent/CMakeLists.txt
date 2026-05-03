add_executable(certosc-agent
    src/main.cpp
    src/agent_daemon.cpp
    src/agent_grpc_client.cpp
    src/job_executor.cpp
)

target_include_directories(certosc-agent PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(certosc-agent PRIVATE
    certosc-common
    certosc-metrics
    certosc-proto
)
