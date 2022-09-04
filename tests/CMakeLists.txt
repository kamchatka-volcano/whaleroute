cmake_minimum_required(VERSION 3.18)
project(test_whaleroute)

enable_testing()
include(external/googletest)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(CheckPIESupported)
check_pie_supported()

set(SRC
    test_router.cpp
        test_response_value_arguments.cpp
        test_multiple_route_specifiers.cpp
        test_router_without_response_value.cpp
        test_router_without_route_specifiers.cpp
        test_router_without_route_specifiers_and_response_value.cpp
        )


add_executable(test_whaleroute ${SRC})
target_compile_features(test_whaleroute PUBLIC cxx_std_17)
set_target_properties(test_whaleroute PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(test_whaleroute PROPERTIES POSITION_INDEPENDENT_CODE OFF)

add_test(NAME test_whaleroute COMMAND test_whaleroute)
target_link_libraries(test_whaleroute PRIVATE GTest::gtest_main GTest::gmock_main Threads::Threads whaleroute::whaleroute)

include(GoogleTest)
gtest_discover_tests(test_whaleroute)