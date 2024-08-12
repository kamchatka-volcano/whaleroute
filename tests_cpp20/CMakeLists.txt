project(test_whaleroute_cpp20)

SealLake_GoogleTest(
        SOURCES
        ../tests/test_router.cpp
        ../tests/test_response_value_arguments.cpp
        ../tests/test_multiple_route_matchers.cpp
        ../tests/test_router_without_route_context.cpp
        ../tests/test_router_without_response_value.cpp
        ../tests/test_router_without_route_matchers.cpp
        ../tests/test_router_without_route_matchers_and_response_value.cpp
        ../tests/test_router_alt_request_processor.cpp
        COMPILE_FEATURES cxx_std_20
        LIBRARIES
            whaleroute::whaleroute
)