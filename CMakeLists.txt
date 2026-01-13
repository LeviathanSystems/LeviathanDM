cmake_minimum_required(VERSION 3.15)

# Read version from VERSION file
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" LEVIATHAN_VERSION)
string(STRIP "${LEVIATHAN_VERSION}" LEVIATHAN_VERSION)
string(REPLACE "." ";" VERSION_LIST ${LEVIATHAN_VERSION})
list(GET VERSION_LIST 0 LEVIATHAN_VERSION_MAJOR)
list(GET VERSION_LIST 1 LEVIATHAN_VERSION_MINOR)
list(GET VERSION_LIST 2 LEVIATHAN_VERSION_PATCH)

project(LeviathanDM VERSION ${LEVIATHAN_VERSION} LANGUAGES C CXX)

message(STATUS "")
message(STATUS "==================================================")
message(STATUS "Building LeviathanDM v${LEVIATHAN_VERSION}")
message(STATUS "==================================================")
message(STATUS "")

# Generate version header
configure_file(
    ${CMAKE_SOURCE_DIR}/version.h.in
    ${CMAKE_BINARY_DIR}/version.h
    @ONLY
)

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)  # Use GNU extensions which are more lenient with C headers

# Enable position independent code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add our simple logger library
add_subdirectory(logger)

# Fetch nlohmann/json for IPC
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -DWLR_USE_UNSTABLE -fpermissive")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wpedantic")
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
    set(ENV{PKG_CONFIG_PATH} "${DISPLAY_INFO_INSTALL_DIR}/lib/x86_64-linux-gnu/pkgconfig:${DISPLAY_INFO_INSTALL_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    
    # Build vendored wlroots (also uses meson)
    set(WLROOTS_BUILD_DIR "${CMAKE_BINARY_DIR}/wlroots-build")
    set(WLROOTS_INSTALL_DIR "${CMAKE_BINARY_DIR}/wlroots-install")
    
    # Check both possible library install locations
    if(NOT EXISTS "${WLROOTS_INSTALL_DIR}/lib/libwlroots-0.19.so" AND NOT EXISTS "${WLROOTS_INSTALL_DIR}/lib/x86_64-linux-gnu/libwlroots-0.19.so")
        message(STATUS "Building wlroots...")
        
        # Set PKG_CONFIG_PATH environment for wlroots build
        set(WLROOTS_PKG_CONFIG "${DISPLAY_INFO_INSTALL_DIR}/lib/x86_64-linux-gnu/pkgconfig:${DISPLAY_INFO_INSTALL_DIR}/lib/pkgconfig")
        if(DEFINED ENV{PKG_CONFIG_PATH})
            set(WLROOTS_PKG_CONFIG "${WLROOTS_PKG_CONFIG}:$ENV{PKG_CONFIG_PATH}")
        endif()
        
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env "PKG_CONFIG_PATH=${WLROOTS_PKG_CONFIG}"
                meson setup ${WLROOTS_BUILD_DIR} ${CMAKE_SOURCE_DIR}/subprojects/wlroots
                --prefix=${WLROOTS_INSTALL_DIR}
                -Dxwayland=enabled
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
    
    # Check both possible library install locations (lib/ for local builds, lib/x86_64-linux-gnu/ for CI/Ubuntu)
    if(EXISTS "${WLROOTS_INSTALL_DIR}/lib/x86_64-linux-gnu/libwlroots-0.19.so")
        set(WLROOTS_LIBRARIES ${WLROOTS_INSTALL_DIR}/lib/x86_64-linux-gnu/libwlroots-0.19.so)
    else()
        set(WLROOTS_LIBRARIES ${WLROOTS_INSTALL_DIR}/lib/libwlroots-0.19.so)
    endif()
    
    # Set libdisplay-info variables
    set(DISPLAY_INFO_INCLUDE_DIRS ${DISPLAY_INFO_INSTALL_DIR}/include)
    if(EXISTS "${DISPLAY_INFO_INSTALL_DIR}/lib/x86_64-linux-gnu/libdisplay-info.so")
        set(DISPLAY_INFO_LIBRARIES ${DISPLAY_INFO_INSTALL_DIR}/lib/x86_64-linux-gnu/libdisplay-info.so)
    else()
        set(DISPLAY_INFO_LIBRARIES ${DISPLAY_INFO_INSTALL_DIR}/lib/libdisplay-info.so)
    endif()
    
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
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(GDK_PIXBUF REQUIRED gdk-pixbuf-2.0)
# pkg_check_modules(GDK3 REQUIRED gdk-3.0)  # Not needed - using gdk-pixbuf only
pkg_check_modules(LIBDRM REQUIRED libdrm)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}  # For generated version.h
    ${WLROOTS_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/wlroots-build/protocol
    ${WAYLAND_SERVER_INCLUDE_DIRS}
    ${XKB_INCLUDE_DIRS}
    ${PIXMAN_INCLUDE_DIRS}
    ${DISPLAY_INFO_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${GDK_PIXBUF_INCLUDE_DIRS}
    # ${GDK3_INCLUDE_DIRS}  # Not needed
    ${LIBDRM_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
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
    src/wayland/LayerSurface.cpp
    src/wayland/NightLight.cpp
    src/wayland/xwayland_compat.c
    # Core layer
    src/core/Seat.cpp
    src/core/Screen.cpp
    src/core/Tag.cpp
    src/core/Client.cpp
    src/core/Events.cpp
    # Config
    src/config/ConfigParser.cpp
    # Utilities
    src/Actions.cpp
    src/KeyBindings.cpp
    # Layout
    src/layout/TilingLayout.cpp
    # UI
    src/ui/StatusBar.cpp
    src/ui/ShmBuffer.cpp
    src/ui/Popover.cpp
    src/ui/WidgetTree.cpp
    src/ui/reusable-widgets/Container.cpp
    src/ui/reusable-widgets/HBox.cpp
    src/ui/reusable-widgets/VBox.cpp
    src/ui/reusable-widgets/ScrollView.cpp
    src/ui/reusable-widgets/Label.cpp
    src/ui/reusable-widgets/Button.cpp
    src/ui/reusable-widgets/TextField.cpp
    src/ui/reusable-widgets/BaseModal.cpp
    src/ui/reusable-widgets/TabBar.cpp
    src/ui/KeybindingHelpModal.cpp
    src/ui/ModalManager.cpp
    src/ui/WidgetPluginManager.cpp
    src/ui/CompositorState.cpp
    src/ui/PluginAPI.cpp
    src/ui/DBusHelper.cpp
    src/ui/NotificationDaemon.cpp
    src/ui/menubar/MenuBar.cpp
    src/ui/menubar/MenuBarManager.cpp
    src/ui/menubar/MenuItemProviders.cpp
    src/ui/menubar/providers/AppsProvider.cpp
    src/ui/menubar/providers/CommandsProvider.cpp
    src/ui/menubar/providers/BookmarksProvider.cpp
    src/ui/IconLoader.cpp
    # IPC
    src/ipc/IPC.cpp
    src/wayland/WallpaperManager.cpp
)

# Headers
set(HEADERS
    # Wayland layer
    include/wayland/Server.hpp
    include/wayland/Output.hpp
    include/wayland/View.hpp
    include/wayland/Input.hpp
    include/wayland/LayerManager.hpp
    include/wayland/LayerSurface.hpp
    # Core layer
    include/core/Seat.hpp
    include/core/Screen.hpp
    include/core/Tag.hpp
    include/core/Client.hpp
    # Config
    include/config/ConfigParser.hpp
    # Utilities
    include/KeyBindings.hpp
    include/Types.hpp
    # Layout
    include/layout/TilingLayout.hpp
    # UI
    include/ui/StatusBar.hpp
    include/ui/menubar/MenuBar.hpp
    include/ui/menubar/MenuBarManager.hpp
    include/ui/menubar/MenuItemProviders.hpp
    include/ui/menubar/providers/AppsProvider.hpp
    include/ui/menubar/providers/CommandsProvider.hpp
    include/ui/menubar/providers/BookmarksProvider.hpp
    # IPC
    include/ipc/IPC.hpp
    include/wayland/WallpaperManager.hpp
    include/wayland/WallpaperEngineRenderer.hpp
)

# Create a shared library for UI components that plugins can link against
add_library(leviathan-ui SHARED
    src/ui/BaseWidget.cpp
    src/ui/DBusHelper.cpp
    src/ui/CompositorState.cpp
    src/ui/PluginAPI.cpp
)

target_include_directories(leviathan-ui PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${GIO_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
)

target_link_libraries(leviathan-ui
    leviathan-logger
    ${GIO_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${GDK_PIXBUF_LIBRARIES}
    # ${GDK3_LIBRARIES}  # Not needed
)

# Export all symbols
set_target_properties(leviathan-ui PROPERTIES
    LINK_FLAGS "-Wl,--export-dynamic"
    ENABLE_EXPORTS ON
)

# Install the library so plugins can find it
install(TARGETS leviathan-ui
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

# Executable
add_executable(leviathan ${SOURCES} ${HEADERS})

# Export symbols for plugins to use
set_target_properties(leviathan PROPERTIES
    ENABLE_EXPORTS ON
)

# Link libraries
target_link_libraries(leviathan
    leviathan-ui
    leviathan-logger
    ${WLROOTS_LIBRARIES}
    ${WAYLAND_SERVER_LIBRARIES}
    ${XKB_LIBRARIES}
    ${PIXMAN_LIBRARIES}
    ${DISPLAY_INFO_LIBRARIES}
    ${LIBINPUT_LIBRARIES}
    ${YAMLCPP_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${GDK_PIXBUF_LIBRARIES}
    # ${GDK3_LIBRARIES}  # Not needed
    ${LIBDRM_LIBRARIES}
    ${GIO_LIBRARIES}
    ${GLIB_LIBRARIES}
    nlohmann_json::nlohmann_json
    dl  # For dynamic plugin loading
)

# leviathanctl utility
add_executable(leviathanctl 
    src/ipc/leviathanctl.cpp
    src/ipc/IPC.cpp
)
target_link_libraries(leviathanctl
    leviathan-logger
    nlohmann_json::nlohmann_json
)

# Add help window tool subdirectory (if exists)
if(EXISTS "${CMAKE_SOURCE_DIR}/tools/help-window")
    add_subdirectory(tools/help-window)
endif()

# Installation
install(TARGETS leviathan leviathanctl DESTINATION bin)
install(TARGETS leviathan-ui LIBRARY DESTINATION lib)

# Install public headers
install(DIRECTORY include/ DESTINATION include/leviathan
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

# Install pkg-config file
configure_file(
    ${CMAKE_SOURCE_DIR}/leviathan.pc.in
    ${CMAKE_BINARY_DIR}/leviathan.pc
    @ONLY
)
install(FILES ${CMAKE_BINARY_DIR}/leviathan.pc DESTINATION lib/pkgconfig)

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


# Installation (duplicate removed)
# install(TARGETS leviathan DESTINATION bin)

# Optional: install config file template (duplicate removed)
# install(FILES config/leviathanrc DESTINATION share/leviathan)

