cmake_minimum_required(VERSION 3.18)
project(whaleroute VERSION 2.0.0 DESCRIPTION "C++17 request routing library")
include(GNUInstallDirs)
include(external/seal_lake)

SealLake_Download(
        GIT_REPOSITORY https://github.com/kamchatka-volcano/sfun.git
        GIT_TAG        cmake_maintenance
        DIRECTORIES
            include/sfun
        DESTINATION include/whaleroute/detail/external
        TEXT_REPLACEMENTS
            "namespace sfun" "namespace whaleroute::sfun"
)

SealLake_HeaderOnlyLibrary(
        COMPILE_FEATURES cxx_std_17
)

SealLake_OptionalBuildSteps(tests)
