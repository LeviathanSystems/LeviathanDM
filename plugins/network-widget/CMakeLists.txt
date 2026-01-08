cmake_minimum_required(VERSION 3.20)
project(NetworkWidget)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Auto-increment version on each build
execute_process(
    COMMAND ${CMAKE_SOURCE_DIR}/../increment_version.sh ${CMAKE_SOURCE_DIR}/version.txt
    OUTPUT_VARIABLE PLUGIN_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Building ${PROJECT_NAME} version ${PLUGIN_VERSION}")

# Generate version header
configure_file(
    ${CMAKE_SOURCE_DIR}/../version.h.in
    ${CMAKE_BINARY_DIR}/version.h
    @ONLY
)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(LIBNL3 REQUIRED libnl-3.0 libnl-route-3.0)

# Try to find LeviathanDM via pkg-config
pkg_check_modules(LEVIATHAN leviathan)

if(NOT LEVIATHAN_FOUND)
    # Fall back to finding the library in the build directory
    find_library(LEVIATHAN_UI_LIB leviathan-ui
        PATHS ${CMAKE_SOURCE_DIR}/../../build
        NO_DEFAULT_PATH
    )
    
    if(NOT LEVIATHAN_UI_LIB)
        message(FATAL_ERROR "Could not find leviathan-ui library. Build the main project first!")
    endif()
    
    message(STATUS "Found leviathan-ui: ${LEVIATHAN_UI_LIB} (includes logging support)")
endif()

# Plugin source
add_library(network-widget SHARED
    NetworkWidget.cpp
)

# Include directories
target_include_directories(network-widget PRIVATE
    ${CMAKE_SOURCE_DIR}/../../include
    ${CMAKE_BINARY_DIR}
    ${CAIRO_INCLUDE_DIRS}
    ${LIBNL3_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(network-widget PRIVATE
    ${LEVIATHAN_UI_LIB}
    ${CAIRO_LIBRARIES}
    ${LIBNL3_LIBRARIES}
)

# Compiler flags
target_compile_options(network-widget PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    ${CAIRO_CFLAGS_OTHER}
    ${LIBNL3_CFLAGS_OTHER}
)

# Installation
install(TARGETS network-widget
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/leviathan/plugins
)
