cmake_minimum_required(VERSION 3.15)
project(TagsWidget)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use version 1.0.0 for now
set(PLUGIN_VERSION "1.0.0")
message(STATUS "Building ${PROJECT_NAME} version ${PLUGIN_VERSION}")

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
add_library(tags-widget SHARED
    TagsWidget.cpp
)

# Link against Cairo
target_link_libraries(tags-widget
    ${CAIRO_LIBRARIES}
    pthread
)

# Set output name and properties
set_target_properties(tags-widget PROPERTIES
    PREFIX ""  # Don't add 'lib' prefix
    SUFFIX ".so"
    OUTPUT_NAME "tags-widget"
)
