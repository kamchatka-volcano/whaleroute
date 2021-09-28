cmake_minimum_required(VERSION 3.18)
project(test_whaleroute)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(CheckPIESupported)
check_pie_supported()


find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS}, ..)

set(SRC
    test_router.cpp
    test_router_without_processor.cpp
    test_router_without_response_value.cpp
    test_access.cpp
    test_router_without_processor_and_response_value.cpp
    test_router_without_processor_and_request_type_and_response_value.cpp
    test_router_without_processor_and_request_type.cpp
    test_router_without_request_type_and_response_value.cpp
    test_router_without_request_type.cpp)

add_executable(${PROJECT_NAME} ${SRC})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE OFF)
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE ../include/)
target_link_libraries(${PROJECT_NAME} PRIVATE ${GTEST_BOTH_LIBRARIES} Threads::Threads)
