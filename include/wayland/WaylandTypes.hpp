/*
 * WaylandTypes.hpp
 * 
 * Wayland and wlroots type definitions #include <wlr/types/wlr_pointer.h>
#include <wlr/util/box.h>
#include <wlr/util/log.h>
#include <libinput.h>
#include <xkbcommon/xkbcommon.h>
}

// Restore static and namespace keywords for C++ code
#undef static
#undef namespace compatibility
 * This header handles the C99 syntax incompatibility between wlroots and C++
 */

#ifndef WAYLAND_TYPES_HPP
#define WAYLAND_TYPES_HPP

// Pre-include all C/C++ standard headers that wayland headers might transitively include
// This prevents #define static from affecting them
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <cmath>
#include <limits>

// Now include C standard headers needed by wayland
extern "C" {
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
}

/*
 * C++ compatibility workaround for C99 'static' in array parameters
 * wlroots uses 'const float color[static 4]' which is valid C but not C++
 * See: https://github.com/swaywm/wlroots/issues/682
 * 
 * We temporarily redefine 'static' to nothing only for wlroots includes
 */
#define static

extern "C" {
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/backend/interface.h>
#include <wlr/backend/wayland.h>
#include <wlr/backend/session.h>
#include <wlr/backend/multi.h>
#include <wlr/backend/libinput.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/allocator.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/util/box.h>
#include <wlr/util/log.h>
#include <libinput.h>
#include <xkbcommon/xkbcommon.h>
}

// Restore static keyword for C++ code
#undef static

#endif // WAYLAND_TYPES_HPP
