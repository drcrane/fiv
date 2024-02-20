#ifndef WLIF_H
#define WLIF_H

#include <stdint.h>
#include <stddef.h>
#include <pixman.h>
#include <wayland-client.h>
#include "xdg-shell-protocol.h"
#include "zxdg-decoration-unstable-v1.h"
#include <xkbcommon/xkbcommon.h>

struct wlif_global_context {
	struct wl_display * display;
	struct wl_registry * registry;
	struct wl_registry_listener registry_listener;
	struct wl_compositor * compositor;
	struct wl_shm * shm;
	struct wl_shm_pool * shm_pool;
	struct xdg_wm_base * xdg_wm_base;
	struct xdg_wm_base_listener xdg_wm_base_listener;
	struct zxdg_decoration_manager_v1 * zxdg_decoration_manager_v1;
	struct wl_list seats;
	struct wl_seat_listener seat_listener;
	struct wl_pointer * pointer;
	struct wl_pointer_listener pointer_listener;
	struct wl_touch * touch;
	struct wl_touch_listener touch_listener;
	struct wl_keyboard * keyboard;
	struct wl_keyboard_listener keyboard_listener;
	struct xkb_state * xkb_state;
	struct xkb_context * xkb_context;
	struct xkb_keymap * xkb_keymap;
	int terminate;
};

enum WLIF_WINDOW_REDRAW_REASON {
	WLIF_WINDOW_REDRAW_REASON_RESIZE,
	WLIF_WINDOW_REDRAW_REASON_DAMAGED
};

struct wlif_window_context {
	int width, height, stride, size, curr_size;
	int buffer_fd;
	int configured;
	char * buffer_a;
	char * buffer_b;
	int (* redraw)(pixman_image_t *, enum WLIF_WINDOW_REDRAW_REASON reason);

	struct wl_surface * wl_surface;
	struct wl_buffer * wl_buffer_a;
	struct wl_buffer * wl_buffer_b;
	struct xdg_surface_listener xdg_surface_listener;
	struct xdg_surface * xdg_surface;
	struct xdg_toplevel_listener xdg_toplevel_listener;
	struct xdg_toplevel * xdg_toplevel;
	struct zxdg_toplevel_decoration_v1 * zxdg_toplevel_decoration_v1;
};

extern struct wlif_global_context * global_ctx;
extern struct wlif_window_context * window_ctx;

void registry_global_handler(void * data, struct wl_registry * registry, uint32_t name, const char * interface, uint32_t version);

void registry_global_remove_handler(void * data, struct wl_registry * registry, uint32_t name);

int wlif_initialise();
int wlif_dispose();

#endif /* WLIF_H */
