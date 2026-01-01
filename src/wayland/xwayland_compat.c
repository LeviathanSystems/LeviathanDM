/*
 * xwayland_compat.c
 * 
 * C wrapper to access wlr_xwayland_surface fields that use C++ keywords
 */

#define WLR_USE_UNSTABLE
#include <wlr/xwayland/xwayland.h>
#include <wlr/types/wlr_compositor.h>
#include <stdbool.h>
#include <stdint.h>

const char* xwayland_surface_get_class(struct wlr_xwayland_surface* surf) {
    if (!surf) return NULL;
    return surf->class;
}

const char* xwayland_surface_get_title(struct wlr_xwayland_surface* surf) {
    if (!surf) return NULL;
    return surf->title;
}

bool xwayland_surface_get_override_redirect(struct wlr_xwayland_surface* surf) {
    if (!surf) return false;
    return surf->override_redirect;
}

struct wlr_surface* xwayland_surface_get_surface(struct wlr_xwayland_surface* surf) {
    if (!surf) return NULL;
    return surf->surface;
}

int16_t xwayland_surface_get_x(struct wlr_xwayland_surface* surf) {
    if (!surf) return 0;
    return surf->x;
}

int16_t xwayland_surface_get_y(struct wlr_xwayland_surface* surf) {
    if (!surf) return 0;
    return surf->y;
}

uint16_t xwayland_surface_get_width(struct wlr_xwayland_surface* surf) {
    if (!surf) return 0;
    return surf->width;
}

uint16_t xwayland_surface_get_height(struct wlr_xwayland_surface* surf) {
    if (!surf) return 0;
    return surf->height;
}

struct wl_signal* xwayland_surface_get_events_commit(struct wlr_xwayland_surface* surf) {
    if (!surf || !surf->surface) return NULL;
    return &surf->surface->events.commit;
}

struct wl_signal* xwayland_surface_get_events_map(struct wlr_xwayland_surface* surf) {
    if (!surf || !surf->surface) return NULL;
    return &surf->surface->events.map;
}

struct wl_signal* xwayland_surface_get_events_unmap(struct wlr_xwayland_surface* surf) {
    if (!surf || !surf->surface) return NULL;
    return &surf->surface->events.unmap;
}

struct wl_signal* xwayland_surface_get_events_destroy(struct wlr_xwayland_surface* surf) {
    if (!surf) return NULL;
    return &surf->events.destroy;
}

struct wl_signal* xwayland_surface_get_events_request_move(struct wlr_xwayland_surface* surf) {
    if (!surf) return NULL;
    return &surf->events.request_move;
}

struct wl_signal* xwayland_surface_get_events_request_resize(struct wlr_xwayland_surface* surf) {
    if (!surf) return NULL;
    return &surf->events.request_resize;
}

struct wl_signal* xwayland_surface_get_events_request_maximize(struct wlr_xwayland_surface* surf) {
    if (!surf) return NULL;
    return &surf->events.request_maximize;
}

struct wl_signal* xwayland_surface_get_events_request_fullscreen(struct wlr_xwayland_surface* surf) {
    if (!surf) return NULL;
    return &surf->events.request_fullscreen;
}

const char* xwayland_get_display_name(struct wlr_xwayland* xwayland) {
    if (!xwayland) return NULL;
    return xwayland->display_name;
}

struct wl_signal* xwayland_get_ready_signal(struct wlr_xwayland* xwayland) {
    if (!xwayland) return NULL;
    return &xwayland->events.ready;
}

struct wl_signal* xwayland_get_new_surface_signal(struct wlr_xwayland* xwayland) {
    if (!xwayland) return NULL;
    return &xwayland->events.new_surface;
}
