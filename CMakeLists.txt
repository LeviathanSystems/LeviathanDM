cmake_minimum_required(VERSION 3.15)
project(LeviathanDM VERSION 1.0.0 LANGUAGES C CXX)

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)  # Use GNU extensions which are more lenient with C headers

# Fetch spdlog for logging
include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
)
FetchContent_MakeAvailable(spdlog)

# Fetch nlohmann/json for IPC
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -DWLR_USE_UNSTABLE -fpermissive")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Workaround for C99 'static' in array parameters in wlroots headers (not valid C++)
# This treats errors as warnings for C++ when including C headers
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-error>)

# Find required packages
find_package(PkgConfig REQUIRED)

# Option to use vendored dependencies
option(USE_VENDORED_DEPS "Use vendored wlroots and libdisplay-info" ON)

if(USE_VENDORED_DEPS)
    message(STATUS "Using vendored dependencies")
    
    # Check if dependencies are downloaded
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/subprojects/wlroots/meson.build")
        message(FATAL_ERROR "Vendored wlroots not found. Run ./setup-deps.sh first!")
    endif()
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/subprojects/libdisplay-info/meson.build")
        message(FATAL_ERROR "Vendored libdisplay-info not found. Run ./setup-deps.sh first!")
    endif()
    
    # Build libdisplay-info first (wlroots depends on it)
    # Note: libdisplay-info uses meson
    set(DISPLAY_INFO_BUILD_DIR "${CMAKE_BINARY_DIR}/libdisplay-info-build")
    set(DISPLAY_INFO_INSTALL_DIR "${CMAKE_BINARY_DIR}/libdisplay-info-install")
    
    if(NOT EXISTS "${DISPLAY_INFO_INSTALL_DIR}/lib/libdisplay-info.so")
        message(STATUS "Building libdisplay-info...")
        execute_process(
            COMMAND meson setup ${DISPLAY_INFO_BUILD_DIR} ${CMAKE_SOURCE_DIR}/subprojects/libdisplay-info 
                --prefix=${DISPLAY_INFO_INSTALL_DIR}
            RESULT_VARIABLE MESON_RESULT
        )
        if(NOT MESON_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to configure libdisplay-info. Make sure meson is installed.")
        endif()
        
        execute_process(
            COMMAND meson compile -C ${DISPLAY_INFO_BUILD_DIR}
            RESULT_VARIABLE COMPILE_RESULT
        )
        if(NOT COMPILE_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to compile libdisplay-info")
        endif()
        
        execute_process(
            COMMAND meson install -C ${DISPLAY_INFO_BUILD_DIR}
            RESULT_VARIABLE INSTALL_RESULT
        )
        if(NOT INSTALL_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to install libdisplay-info")
        endif()
    endif()
    
    # Add libdisplay-info to pkg-config path
    set(ENV{PKG_CONFIG_PATH} "${DISPLAY_INFO_INSTALL_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    
    # Build vendored wlroots (also uses meson)
    set(WLROOTS_BUILD_DIR "${CMAKE_BINARY_DIR}/wlroots-build")
    set(WLROOTS_INSTALL_DIR "${CMAKE_BINARY_DIR}/wlroots-install")
    
    if(NOT EXISTS "${WLROOTS_INSTALL_DIR}/lib/libwlroots-0.19.so")
        message(STATUS "Building wlroots...")
        
        # Set PKG_CONFIG_PATH environment for wlroots build
        set(WLROOTS_PKG_CONFIG "${DISPLAY_INFO_INSTALL_DIR}/lib/pkgconfig")
        if(DEFINED ENV{PKG_CONFIG_PATH})
            set(WLROOTS_PKG_CONFIG "${WLROOTS_PKG_CONFIG}:$ENV{PKG_CONFIG_PATH}")
        endif()
        
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env "PKG_CONFIG_PATH=${WLROOTS_PKG_CONFIG}"
                meson setup ${WLROOTS_BUILD_DIR} ${CMAKE_SOURCE_DIR}/subprojects/wlroots
                --prefix=${WLROOTS_INSTALL_DIR}
                -Dxwayland=disabled
                -Dexamples=false
                -Dbackends=drm,libinput,x11
            RESULT_VARIABLE MESON_RESULT
        )
        if(NOT MESON_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to configure wlroots")
        endif()
        
        execute_process(
            COMMAND meson compile -C ${WLROOTS_BUILD_DIR}
            RESULT_VARIABLE COMPILE_RESULT
        )
        if(NOT COMPILE_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to compile wlroots")
        endif()
        
        execute_process(
            COMMAND meson install -C ${WLROOTS_BUILD_DIR}
            RESULT_VARIABLE INSTALL_RESULT
        )
        if(NOT INSTALL_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to install wlroots")
        endif()
    endif()
    
    # Set wlroots variables
    set(WLROOTS_INCLUDE_DIRS 
        ${WLROOTS_INSTALL_DIR}/include/wlroots-0.19
        ${WLROOTS_INSTALL_DIR}/include
    )
    set(WLROOTS_LIBRARIES ${WLROOTS_INSTALL_DIR}/lib/libwlroots-0.19.so)
    
    # Set libdisplay-info variables
    set(DISPLAY_INFO_INCLUDE_DIRS ${DISPLAY_INFO_INSTALL_DIR}/include)
    set(DISPLAY_INFO_LIBRARIES ${DISPLAY_INFO_INSTALL_DIR}/lib/libdisplay-info.so)
    
    # Update PKG_CONFIG_PATH for finding vendored wayland-protocols
    set(ENV{PKG_CONFIG_PATH} "${WLROOTS_INSTALL_DIR}/share/pkgconfig:${DISPLAY_INFO_INSTALL_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    
else()
    message(STATUS "Using system dependencies")
    
    # Use system wlroots and libdisplay-info
    pkg_check_modules(WLROOTS REQUIRED wlroots)
    pkg_check_modules(DISPLAY_INFO REQUIRED libdisplay-info)
    
    set(DISPLAY_INFO_INCLUDE_DIRS ${DISPLAY_INFO_INCLUDE_DIRS})
    set(DISPLAY_INFO_LIBRARIES ${DISPLAY_INFO_LIBRARIES})
endif()

# Common dependencies (always from system)
pkg_check_modules(WAYLAND_SERVER REQUIRED wayland-server)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
pkg_check_modules(XKB REQUIRED xkbcommon)
pkg_check_modules(PIXMAN REQUIRED pixman-1)
pkg_check_modules(LIBINPUT REQUIRED libinput)
pkg_check_modules(YAMLCPP REQUIRED yaml-cpp)

# Common dependencies (always from system)
pkg_check_modules(WAYLAND_SERVER REQUIRED wayland-server)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
pkg_check_modules(XKB REQUIRED xkbcommon)
pkg_check_modules(PIXMAN REQUIRED pixman-1)
pkg_check_modules(LIBINPUT REQUIRED libinput)
pkg_check_modules(YAMLCPP REQUIRED yaml-cpp)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${WLROOTS_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/wlroots-build/protocol
    ${WAYLAND_SERVER_INCLUDE_DIRS}
    ${XKB_INCLUDE_DIRS}
    ${PIXMAN_INCLUDE_DIRS}
    ${DISPLAY_INFO_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    # Wayland layer
    src/wayland/Server.cpp
    src/wayland/Output.cpp
    src/wayland/View.cpp
    src/wayland/Input.cpp
    src/wayland/LayerManager.cpp
    # Core layer
    src/core/Seat.cpp
    src/core/Screen.cpp
    src/core/Tag.cpp
    src/core/Client.cpp
    # Utilities
    src/TilingLayout.cpp
    src/Config.cpp
    src/ConfigParser.cpp
    src/KeyBindings.cpp
    # IPC
    src/IPC.cpp
)

# Headers
set(HEADERS
    # Wayland layer
    include/wayland/Server.hpp
    include/wayland/Output.hpp
    include/wayland/View.hpp
    include/wayland/Input.hpp
    include/wayland/LayerManager.hpp
    # Core layer
    include/core/Seat.hpp
    include/core/Screen.hpp
    include/core/Tag.hpp
    include/core/Client.hpp
    # Utilities
    include/TilingLayout.hpp
    include/Config.hpp
    include/ConfigParser.hpp
    include/KeyBindings.hpp
    include/Types.hpp
    include/IPC.hpp
)

# Executable
add_executable(leviathan ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(leviathan
    ${WLROOTS_LIBRARIES}
    ${WAYLAND_SERVER_LIBRARIES}
    ${XKB_LIBRARIES}
    ${PIXMAN_LIBRARIES}
    ${DISPLAY_INFO_LIBRARIES}
    ${LIBINPUT_LIBRARIES}
    ${YAMLCPP_LIBRARIES}
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

# leviathanctl utility
add_executable(leviathanctl 
    src/leviathanctl.cpp
    src/IPC.cpp
)
target_link_libraries(leviathanctl
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

# Installation
install(TARGETS leviathan leviathanctl DESTINATION bin)

# Optional: install config file template
install(FILES config/leviathanrc DESTINATION share/leviathan)

# Print configuration summary
message(STATUS "==================================================")
message(STATUS "LeviathanDM Configuration Summary")
message(STATUS "==================================================")
message(STATUS "Vendored dependencies: ${USE_VENDORED_DEPS}")
if(USE_VENDORED_DEPS)
    message(STATUS "  wlroots: vendored (0.19.2)")
    message(STATUS "  libdisplay-info: vendored (0.2.0)")
else()
    message(STATUS "  wlroots: system")
    message(STATUS "  libdisplay-info: system")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "==================================================")


# Installation
install(TARGETS leviathan DESTINATION bin)

# Optional: install config file template
install(FILES config/leviathanrc DESTINATION share/leviathan)
