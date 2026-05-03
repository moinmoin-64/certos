add_executable(certosc-gateway
    src/main.cpp
    src/http_server.cpp
    src/websocket_handler.cpp
    src/rest_routes.cpp
    src/grpc_bridge.cpp
)

target_include_directories(certosc-gateway PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(certosc-gateway PRIVATE
    certosc-common
    certosc-auth
    certosc-proto
    Boost::beast
    Boost::asio
    Boost::json
)
