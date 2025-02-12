#ifndef WLIF_H
#define WLIF_H

#include <stdint.h>
#include <stddef.h>
#include <pixman.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "presentation-time-client-protocol.h"
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
	struct wp_presentation * wp_presentation;
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
	struct wl_list outputs;
	int terminate;
};

enum WLIF_WINDOW_REDRAW_REASON {
	WLIF_WINDOW_REDRAW_REASON_RESIZE,
	WLIF_WINDOW_REDRAW_REASON_DAMAGED
};

struct wlif_window_context {
	int width, height, stride, size, curr_size;
	int buffer_fd;
	char * buffer_a;
	char * buffer_b;
	int (* redraw)(pixman_image_t *, enum WLIF_WINDOW_REDRAW_REASON reason);
	int request_fullscreen;
	int wl_surface_configured;
	int xdg_surface_configured;
	int xdg_toplevel_configured;

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

/* For now these are private
void registry_global_handler(void * data, struct wl_registry * registry, uint32_t name, const char * interface, uint32_t version);
void registry_global_remove_handler(void * data, struct wl_registry * registry, uint32_t name); */

int wlif_initialise();
int wlif_presentation_time(struct wlif_window_context * window_ctx);
int wlif_process_pending(int waitfor_msecs);
int wlif_dispose();

#endif /* WLIF_H */
