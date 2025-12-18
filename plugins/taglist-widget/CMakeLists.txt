cmake_minimum_required(VERSION 3.15)
project(taglist-widget VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Cairo
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

# Include LeviathanDM headers
include_directories(${CMAKE_SOURCE_DIR}/../../include)

# Plugin source
add_library(taglist-widget SHARED
    TagListWidget.cpp
)

# Link libraries
target_link_libraries(taglist-widget
    ${CAIRO_LIBRARIES}
    pthread
)

target_include_directories(taglist-widget PRIVATE
    ${CAIRO_INCLUDE_DIRS}
)

# Set library properties (no 'lib' prefix, ensure .so suffix)
set_target_properties(taglist-widget PROPERTIES
    PREFIX ""
    SUFFIX ".so"
)

# Install to system plugin directory
install(TARGETS taglist-widget
    LIBRARY DESTINATION lib/leviathan/plugins
)
