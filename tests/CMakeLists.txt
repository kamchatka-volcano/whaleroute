cmake_minimum_required(VERSION 3.18)
project(test_whaleroute)

SealLake_GoogleTest(
        SOURCES
        test_router.cpp
        test_alt_regex_specifying.cpp
        test_response_value_arguments.cpp
        test_multiple_route_matchers.cpp
        test_router_without_route_context.cpp
        test_router_without_response_value.cpp
        test_router_without_route_matchers.cpp
        test_router_without_route_matchers_and_response_value.cpp
        LIBRARIES
        whaleroute::whaleroute
)