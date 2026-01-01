#ifndef XWAYLAND_COMPAT_HPP
#define XWAYLAND_COMPAT_HPP

#include <cstdint>

/*
 * C++ compatibility wrapper for wlr/xwayland.h
 * 
 * The xwayland.h header uses 'class' as a struct member name, which is a C++ keyword.
 * We cannot include xwayland.h in C++ files. Instead, we use forward declarations
 * and access problematic fields through C wrapper functions.
 */

// Forward declare the xwayland types
struct wlr_xwayland_surface;
struct wlr_xwayland;
struct wlr_surface;
struct wl_display;
struct wlr_compositor;
struct wlr_xcursor_manager;

// C wrapper functions to access xwayland_surface fields
extern "C" {
    const char* xwayland_surface_get_class(struct wlr_xwayland_surface* surf);
    const char* xwayland_surface_get_title(struct wlr_xwayland_surface* surf);
    bool xwayland_surface_get_override_redirect(struct wlr_xwayland_surface* surf);
    struct wlr_surface* xwayland_surface_get_surface(struct wlr_xwayland_surface* surf);
    int16_t xwayland_surface_get_x(struct wlr_xwayland_surface* surf);
    int16_t xwayland_surface_get_y(struct wlr_xwayland_surface* surf);
    uint16_t xwayland_surface_get_width(struct wlr_xwayland_surface* surf);
    uint16_t xwayland_surface_get_height(struct wlr_xwayland_surface* surf);
    struct wl_signal* xwayland_surface_get_events_commit(struct wlr_xwayland_surface* surf);
    struct wl_signal* xwayland_surface_get_events_map(struct wlr_xwayland_surface* surf);
    struct wl_signal* xwayland_surface_get_events_unmap(struct wlr_xwayland_surface* surf);
    struct wl_signal* xwayland_surface_get_events_destroy(struct wlr_xwayland_surface* surf);
    struct wl_signal* xwayland_surface_get_events_request_move(struct wlr_xwayland_surface* surf);
    struct wl_signal* xwayland_surface_get_events_request_resize(struct wlr_xwayland_surface* surf);
    struct wl_signal* xwayland_surface_get_events_request_maximize(struct wlr_xwayland_surface* surf);
    struct wl_signal* xwayland_surface_get_events_request_fullscreen(struct wlr_xwayland_surface* surf);
    
    // Access wlr_xwayland fields
    const char* xwayland_get_display_name(struct wlr_xwayland* xwayland);
    struct wl_signal* xwayland_get_ready_signal(struct wlr_xwayland* xwayland);
    struct wl_signal* xwayland_get_new_surface_signal(struct wlr_xwayland* xwayland);
}

// These wlroots functions are already declared in wlroots library with C linkage
// We just need to forward declare them for use in C++ without including xwayland.h
extern "C" {
    struct wlr_xwayland* wlr_xwayland_create(struct wl_display*, struct wlr_compositor*, bool lazy);
    void wlr_xwayland_set_cursor(struct wlr_xwayland*, uint8_t* pixels, uint32_t stride, uint32_t width, uint32_t height, int32_t hotspot_x, int32_t hotspot_y);
}

// Helper macros for convenience
#define XWAYLAND_CLASS(xsurf) xwayland_surface_get_class(xsurf)
#define XWAYLAND_TITLE(xsurf) xwayland_surface_get_title(xsurf)
#define XWAYLAND_OVERRIDE_REDIRECT(xsurf) xwayland_surface_get_override_redirect(xsurf)
#define XWAYLAND_SURFACE(xsurf) xwayland_surface_get_surface(xsurf)

#endif // XWAYLAND_COMPAT_HPP
