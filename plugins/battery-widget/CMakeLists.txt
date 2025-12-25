cmake_minimum_required(VERSION 3.20)
project(BatteryWidget)

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
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Find the leviathan-ui library (includes logging support)
find_library(LEVIATHAN_UI_LIB leviathan-ui
    PATHS ${CMAKE_SOURCE_DIR}/../../build
    NO_DEFAULT_PATH
)

if(NOT LEVIATHAN_UI_LIB)
    message(FATAL_ERROR "Could not find leviathan-ui library. Build the main project first!")
endif()

message(STATUS "Found leviathan-ui: ${LEVIATHAN_UI_LIB} (includes logging support)")

# Plugin source
add_library(battery-widget SHARED
    BatteryWidget.cpp
)

# Include directories - use compositor's logger (no external dependencies)
target_include_directories(battery-widget PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../../include
    ${CMAKE_SOURCE_DIR}/../../logger
    ${CAIRO_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)

# Link libraries - libleviathan-ui provides logging through SimpleLogger
target_link_libraries(battery-widget PRIVATE
    ${LEVIATHAN_UI_LIB}
    ${CAIRO_LIBRARIES}
    ${GIO_LIBRARIES}
    ${GLIB_LIBRARIES}
)

# Compiler flags
target_compile_options(battery-widget PRIVATE
    ${CAIRO_CFLAGS_OTHER}
    ${GIO_CFLAGS_OTHER}
    ${GLIB_CFLAGS_OTHER}
    -fPIC
    -Wall
    -Wextra
)

# Install plugin to plugins directory
install(TARGETS battery-widget
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/leviathan/plugins
)
