add_library(certosc-common STATIC
    src/config.cpp
    src/logging.cpp
    src/uuid.cpp
)

target_include_directories(certosc-common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(certosc-common PUBLIC
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    nlohmann_json::nlohmann_json
    certosc-proto
)
