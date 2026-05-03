add_library(certosc-metrics STATIC
    src/system_monitor.cpp
)

target_include_directories(certosc-metrics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(certosc-metrics PUBLIC
    certosc-common
    certosc-proto
)
