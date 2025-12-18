cmake_minimum_required(VERSION 3.15)
project(ClockWidget)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Cairo
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

# Include LeviathanDM headers
include_directories(
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
