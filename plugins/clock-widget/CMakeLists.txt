cmake_minimum_required(VERSION 3.15)
project(ClockWidget)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
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

# Find Cairo
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

# Include LeviathanDM headers
include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/../../include
    ${CAIRO_INCLUDE_DIRS}
)

# Build the plugin as a shared library
add_library(clock-widget SHARED
    ClockWidget.cpp
)

# Link against Cairo
target_link_libraries(clock-widget
    ${CAIRO_LIBRARIES}
    pthread
)

# Set output name and properties
set_target_properties(clock-widget PROPERTIES
    PREFIX ""  # Don't add 'lib' prefix
    SUFFIX ".so"
    OUTPUT_NAME "clock-widget"
)

# Install to plugin directory
install(TARGETS clock-widget
    LIBRARY DESTINATION lib/leviathan/plugins
)
