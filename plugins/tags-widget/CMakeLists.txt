cmake_minimum_required(VERSION 3.15)
project(TagsWidget)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

# Use version 1.0.0 for now
set(PLUGIN_VERSION "1.0.0")
message(STATUS "Building ${PROJECT_NAME} version ${PLUGIN_VERSION}")

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
add_library(tags-widget SHARED
    TagsWidget.cpp
)

# Link against Cairo and LeviathanDM
if(LEVIATHAN_FOUND)
    target_link_libraries(tags-widget
        ${LEVIATHAN_LIBRARIES}
        ${CAIRO_LIBRARIES}
    )
else()
    target_link_libraries(tags-widget
        ${CAIRO_LIBRARIES}
        pthread
    )
endif()

# Set output name and properties
set_target_properties(tags-widget PROPERTIES
    PREFIX ""  # Don't add 'lib' prefix
    SUFFIX ".so"
    OUTPUT_NAME "tags-widget"
)
