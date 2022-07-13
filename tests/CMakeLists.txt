cmake_minimum_required(VERSION 3.18)
project(test_whaleroute)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(CheckPIESupported)
check_pie_supported()

set(SRC
    test_router.cpp
    test_router_without_processor.cpp
    test_router_without_response_value.cpp
    test_access.cpp
    test_router_without_processor_and_response_value.cpp
    test_router_without_processor_and_request_type_and_response_value.cpp
    test_router_without_processor_and_request_type.cpp
    test_router_without_request_type_and_response_value.cpp
    test_router_without_request_type.cpp
    test_multirouter.cpp)

add_executable(test_whaleroute ${SRC})
target_compile_features(test_whaleroute PUBLIC cxx_std_17)
set_target_properties(test_whaleroute PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(test_whaleroute PROPERTIES POSITION_INDEPENDENT_CODE OFF)

add_test(NAME test_whaleroute COMMAND test_whaleroute)
target_link_libraries(test_whaleroute PRIVATE ${GTEST_BOTH_LIBRARIES} Threads::Threads whaleroute::whaleroute)
