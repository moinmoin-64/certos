add_executable(certosc-tests
    test_fair_share.cpp
    test_jwt_manager.cpp
)

target_include_directories(certosc-tests PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../master/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/auth/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/common/include
)

# We need to compile the specific implementation files we are testing 
# because they are built into separate executables/libraries
# Alternatively, we can link against the libraries. We have lib-auth.
target_link_libraries(certosc-tests PRIVATE
    certosc-common
    certosc-auth
    GTest::gtest_main
    GTest::gtest
)

# For testing master-specific components, we either build them into a library or add the cpp files.
# Since master is an executable, we compile fair_share.cpp directly here for testing
target_sources(certosc-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../master/src/scheduler/fair_share.cpp
)

add_test(NAME CertOSTests COMMAND certosc-tests)
