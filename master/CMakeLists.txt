add_executable(certosc-master
    src/main.cpp
    src/master_server.cpp
    src/grpc_service.cpp
    src/state_store.cpp
    src/node_registry.cpp
    src/job_manager.cpp
    src/resource_manager.cpp
    src/scheduler/fair_share.cpp
    src/scheduler/scheduler_engine.cpp
)

target_include_directories(certosc-master PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(certosc-master PRIVATE
    certosc-common
    certosc-auth
    certosc-proto
    unofficial::sqlite3::sqlite3
)
