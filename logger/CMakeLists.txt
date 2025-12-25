cmake_minimum_required(VERSION 3.15)
project(leviathan-logger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header-only library - no sources to compile
add_library(leviathan-logger INTERFACE)

target_include_directories(leviathan-logger INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install headers
install(FILES 
    SimpleLogger.hpp
    LogFormat.hpp
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/leviathan/logger
)
