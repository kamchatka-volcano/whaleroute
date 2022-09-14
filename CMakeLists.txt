cmake_minimum_required(VERSION 3.18)
project(whaleroute VERSION 1.1.0 DESCRIPTION "C++17 request routing library")

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(WHALEROUTE_SUBPROJECT OFF)
else()
    set(WHALEROUTE_SUBPROJECT ON)
endif()

add_library(whaleroute INTERFACE)
add_library(whaleroute::whaleroute ALIAS whaleroute)
target_compile_features(whaleroute INTERFACE cxx_std_17)

include(external/sfun)

target_include_directories(
        whaleroute
        INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

option(ENABLE_TESTS "Enable tests" OFF)
if (ENABLE_TESTS AND NOT WHALEROUTE_SUBPROJECT)
    add_subdirectory(tests)
endif()

option(INSTALL_WHALEROUTE "Install whaleroute library unconditionally" OFF)
if (NOT WHALEROUTE_SUBPROJECT OR INSTALL_WHALEROUTE)
    include(GNUInstallDirs)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/whaleroute DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    set(WHALEROUTE_PACK_PATH ${CMAKE_INSTALL_LIBDIR}/cmake/whaleroute)

    install(TARGETS whaleroute
            EXPORT whaleroute-targets
    )
    install(EXPORT whaleroute-targets
            FILE whalerouteTargets.cmake
            NAMESPACE whaleroute::
            DESTINATION ${WHALEROUTE_PACK_PATH}
    )

    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
            ${CMAKE_CURRENT_BINARY_DIR}/whalerouteConfigVersion.cmake
            COMPATIBILITY SameMajorVersion
            ARCH_INDEPENDENT
    )
    configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/whalerouteConfig.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/whalerouteConfig.cmake
            INSTALL_DESTINATION ${WHALEROUTE_PACK_PATH}
    )
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/whalerouteConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/whalerouteConfigVersion.cmake
            DESTINATION ${WHALEROUTE_PACK_PATH}
    )
endif()
