cmake_minimum_required(VERSION 3.15)
project(SystemMonitorWidget)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

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

# Find Cairo and LeviathanDM
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

# Try to find LeviathanDM via pkg-config
pkg_check_modules(LEVIATHAN leviathan)

# Include directories
if(LEVIATHAN_FOUND)
    # Use installed headers
    include_directories(
        ${CMAKE_BINARY_DIR}
        ${LEVIATHAN_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
    )
else()
    # Fall back to source tree headers for development
    message(STATUS "LeviathanDM not installed, using source tree headers")
    include_directories(
        ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/../../include
        ${CAIRO_INCLUDE_DIRS}
    )
endif()

# Build the plugin as a shared library
add_library(system-monitor-widget SHARED
    SystemMonitorWidget.cpp
)

# Link against Cairo and LeviathanDM
if(LEVIATHAN_FOUND)
    target_link_libraries(system-monitor-widget
        ${LEVIATHAN_LIBRARIES}
        ${CAIRO_LIBRARIES}
    )
else()
    target_link_libraries(system-monitor-widget
        ${CAIRO_LIBRARIES}
        pthread
    )
endif()

# Set output name and properties
set_target_properties(system-monitor-widget PROPERTIES
    PREFIX ""  # Don't add 'lib' prefix
    SUFFIX ".so"
    OUTPUT_NAME "system-monitor-widget"
)

# Installation
install(TARGETS system-monitor-widget
    LIBRARY DESTINATION ~/.config/leviathan/plugins
)
