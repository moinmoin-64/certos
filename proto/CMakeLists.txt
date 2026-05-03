# Proto CMake - generates C++ code from .proto files

set(PROTO_FILES
    certosc/common.proto
    certosc/master.proto
    certosc/agent.proto
    certosc/metrics.proto
)

set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

set(GENERATED_SRCS "")
set(GENERATED_HDRS "")

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    get_filename_component(PROTO_DIR ${PROTO_FILE} DIRECTORY)

    set(PB_SRC "${PROTO_GEN_DIR}/${PROTO_DIR}/${PROTO_NAME}.pb.cc")
    set(PB_HDR "${PROTO_GEN_DIR}/${PROTO_DIR}/${PROTO_NAME}.pb.h")
    set(GRPC_SRC "${PROTO_GEN_DIR}/${PROTO_DIR}/${PROTO_NAME}.grpc.pb.cc")
    set(GRPC_HDR "${PROTO_GEN_DIR}/${PROTO_DIR}/${PROTO_NAME}.grpc.pb.h")

    add_custom_command(
        OUTPUT ${PB_SRC} ${PB_HDR} ${GRPC_SRC} ${GRPC_HDR}
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS
            --proto_path=${PROTO_SRC_DIR}
            --cpp_out=${PROTO_GEN_DIR}
            --grpc_out=${PROTO_GEN_DIR}
            --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}
            ${PROTO_FILE}
        DEPENDS ${PROTO_SRC_DIR}/${PROTO_FILE}
        WORKING_DIRECTORY ${PROTO_SRC_DIR}
        COMMENT "Generating protobuf/gRPC code for ${PROTO_FILE}"
    )

    list(APPEND GENERATED_SRCS ${PB_SRC} ${GRPC_SRC})
    list(APPEND GENERATED_HDRS ${PB_HDR} ${GRPC_HDR})
endforeach()

add_library(certosc-proto STATIC ${GENERATED_SRCS} ${GENERATED_HDRS})

target_include_directories(certosc-proto PUBLIC ${PROTO_GEN_DIR})

target_link_libraries(certosc-proto PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
    gRPC::grpc++_reflection
)

# Suppress warnings in generated code
target_compile_options(certosc-proto PRIVATE -w)
