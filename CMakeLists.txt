cmake_minimum_required(VERSION 3.18)
project(whaleroute VERSION 3.1.0 DESCRIPTION "C++17 request routing library")
include(external/seal_lake)

SealLake_Bundle(
        NAME whaleroute_sfun
        GIT_REPOSITORY https://github.com/kamchatka-volcano/sfun.git
        GIT_TAG v5.1.0
        DIRECTORIES
        include/sfun
        DESTINATION include/whaleroute/external
        TEXT_REPLACEMENTS
        "namespace sfun" "namespace whaleroute::sfun"
        "SFUN_" "WHALEROUTE_SFUN_"
)

SealLake_HeaderOnlyLibrary(
        COMPILE_FEATURES cxx_std_17
)

SealLake_OptionalSubProjects(tests)
