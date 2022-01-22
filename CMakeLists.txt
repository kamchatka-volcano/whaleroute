cmake_minimum_required(VERSION 3.18)

if(NOT DEFINED WHALEROUTE_SUBPROJECT)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(WHALEROUTE_SUBPROJECT OFF)
    else()
        set(WHALEROUTE_SUBPROJECT ON)
    endif()
endif()

project(whaleroute VERSION 1.0.0 DESCRIPTION "C++17 request routing library")

add_library(whaleroute INTERFACE)
target_compile_features(whaleroute INTERFACE cxx_std_17)

target_include_directories(
        whaleroute
        INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

option(ENABLE_TESTS "Enable tests" OFF)
if (ENABLE_TESTS AND NOT WHALEROUTE_SUBPROJECT)
    enable_testing()
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
if(NOT WHALEROUTE_SUBPROJECT)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/whaleroute DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
