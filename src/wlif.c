#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <malloc.h>
#include <syscall.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
//#include <assert.h>
#include <linux/input-event-codes.h>

#include "shm.h"
#include "wlif.h"
#include "imageloader.h"

struct wlif_global_context wlif_global_ctx_a;
struct wlif_global_context * global_ctx = &wlif_global_ctx_a;

struct wlif_window_context wlif_window_ctx_a;
struct wlif_window_context * window_ctx = &wlif_window_ctx_a;

struct wlif_ptr_element {
	struct wl_list element_header;
	void * element;
};

struct wlif_output_element {
	struct wl_list element_header;
	struct wl_output * wl_output;
	struct zxdg_output_v1 * xdg_output;
	struct {
		int32_t x;
		int32_t y;
		int32_t width;
		int32_t height;
	} logical;
	char * name;
	char * description;
};

pixman_image_t * icon;

int wlif_adjustbuffer(struct wlif_window_context * ctx);

static void wl_output_handle_geometry(void * data, struct wl_output * wl_output, int32_t x, int32_t y,
		int32_t physical_width, int32_t physical_height, int32_t subpixel, const char * make,
		const char * model, int32_t output_transform) {
	struct wlif_output_element * wlif_output_element = (struct wlif_output_element *)data;
	wlif_output_element->logical.x = x;
	wlif_output_element->logical.y = y;
}

static void wl_output_handle_mode(void * data, struct wl_output * wl_output, uint32_t flags, int32_t width, int32_t height,
		int32_t refresh) {
	struct wlif_output_element * wlif_output_element = (struct wlif_output_element *)data;
	wlif_output_element->logical.width = width;
	wlif_output_element->logical.height = height;
}

static void wl_output_handle_done(void * data, struct wl_output * wl_output) {
	struct wlif_output_element * wlif_output_element = (struct wlif_output_element *)data;
	fprintf(stdout, "Output %s : x %d y %d: %dx%d\n", wlif_output_element->name, wlif_output_element->logical.x, wlif_output_element->logical.y, wlif_output_element->logical.width, wlif_output_element->logical.height);
}

static void wl_output_handle_scale(void * data, struct wl_output * wl_output, int32_t scale) {
	//
}

static void wl_output_handle_name(void * data, struct wl_output * wl_output, const char * name) {
	struct wlif_output_element * wlif_output_element = (struct wlif_output_element *)data;
	wlif_output_element->name = strdup(name);
}

static void wl_output_handle_description(void * data, struct wl_output * wl_output, const char * description) {
}

static const struct wl_output_listener wl_output_listener = {
	.geometry = wl_output_handle_geometry,
	.mode = wl_output_handle_mode,
	.done = wl_output_handle_done,
	.scale = wl_output_handle_scale,
	.name = wl_output_handle_name,
	.description = wl_output_handle_description,
};

void registry_global_handler(void * data, struct wl_registry * registry, uint32_t name, const char * interface, uint32_t version) {
	fprintf(stdout, "interface: '%s', version: %u, name: %u\n", interface, version, name);
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		global_ctx->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 3);
	}
	if (strcmp(interface, wl_shm_interface.name) == 0) {
		global_ctx->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}
	if (strcmp(interface, wl_seat_interface.name) == 0) {
		struct wl_seat * seat;
		seat = wl_registry_bind(registry, name, &wl_seat_interface, 7);
		struct wlif_ptr_element * ele = malloc(sizeof(struct wlif_ptr_element));
		ele->element = seat;
		wl_list_insert(&global_ctx->seats, &ele->element_header);
	}
	if (strcmp(interface, wl_output_interface.name) == 0) {
		struct wlif_output_element * wlif_output_element = malloc(sizeof(struct wlif_output_element));
		memset(wlif_output_element, 0, sizeof(struct wlif_output_element));
		wlif_output_element->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 4);
		wl_list_insert(&global_ctx->outputs, &wlif_output_element->element_header);
		fprintf(stdout, "Added an output proxy (%d)\n", wl_proxy_get_id((struct wl_proxy *)wlif_output_element->wl_output));
		wl_output_add_listener(wlif_output_element->wl_output, &wl_output_listener, wlif_output_element);
	}
	if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		global_ctx->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 2);
	}
	if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
		global_ctx->zxdg_decoration_manager_v1 = wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1);
	}
}

void registry_global_remove_handler(void * data, struct wl_registry * registry, uint32_t name) {
	fprintf(stdout, "removed: %u\n", name);
}

void xdg_surface_configure_handler(void * data, struct xdg_surface * xdg_surface, uint32_t serial) {
	struct wlif_window_context * ctx = (struct wlif_window_context *)data;
	xdg_surface_ack_configure(xdg_surface, serial);
	if (ctx->configured == 0) {
		wlif_adjustbuffer(ctx);
		ctx->configured = 1;
	}
	fprintf(stdout, "xdg_surface_configure_ack\n");
}

void xdg_toplevel_configure_handler(void * data, struct xdg_toplevel * xdg_toplevel, int32_t width, int32_t height, struct wl_array * states) {
	bool is_activated = false;
	bool is_fullscreen = false;
	bool is_maximised = false;
	bool is_tiled_left = false;
	bool is_tiled_right = false;
	bool is_tiled_top = false;
	bool is_tiled_bottom = false;
	bool is_resizing = false;
	bool is_suspended = false;

	enum xdg_toplevel_state *state;
	wl_array_for_each(state, states) {
		switch (*state) {
			case XDG_TOPLEVEL_STATE_ACTIVATED:    is_activated = true; break;
			case XDG_TOPLEVEL_STATE_FULLSCREEN:   is_fullscreen = true; break;
			case XDG_TOPLEVEL_STATE_MAXIMIZED:    is_maximised = true; break;
			case XDG_TOPLEVEL_STATE_TILED_LEFT:   is_tiled_left = true; break;
			case XDG_TOPLEVEL_STATE_TILED_RIGHT:  is_tiled_right = true; break;
			case XDG_TOPLEVEL_STATE_TILED_TOP:    is_tiled_top = true; break;
			case XDG_TOPLEVEL_STATE_TILED_BOTTOM: is_tiled_bottom = true; break;
			case XDG_TOPLEVEL_STATE_RESIZING:     is_resizing = true; break;
			case XDG_TOPLEVEL_STATE_SUSPENDED:    is_suspended = true; break;
		}
	}

	if (is_activated) {
		fprintf(stdout, "is_activated\n");
	}
	if (is_maximised) {
		fprintf(stdout, "is_maximised\n");
	}
	if (is_fullscreen) {
		fprintf(stdout, "is_fullscreen\n");
	}
	if (is_tiled_left) {
		fprintf(stdout, "is_tiled_left\n");
	}
	if (is_tiled_right) {
		fprintf(stdout, "is_tiled_right\n");
	}
	if (is_tiled_top) {
		fprintf(stdout, "is_tiled_top\n");
	}
	if (is_tiled_bottom) {
		fprintf(stdout, "is_tiled_bottom\n");
	}
	if (is_resizing) {
		fprintf(stdout, "is_resizing\n");
	}
	if (is_suspended) {
		fprintf(stdout, "is_suspended\n");
	}

	printf("toplevel_configure: %dx%d\n", width, height);
	if ((width != window_ctx->width || height != window_ctx->height) && width && height) {
		window_ctx->width = width;
		window_ctx->height = height;
		wlif_adjustbuffer(window_ctx);
		//wl_surface_damage_buffer(window_ctx->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
	}
}

void xdg_toplevel_close_handler(void * data, struct xdg_toplevel * xdg_toplevel) {
	printf("toplevel_close\n");
	global_ctx->terminate = 1;
}

void xdg_wm_base_ping_handler(void * data, struct xdg_wm_base * xdg_wm_base, uint32_t serial) {
	xdg_wm_base_pong(xdg_wm_base, serial);
	printf("xdg_wm_base_ping-pong\n");
}

static void wl_buffer_release_handler(void * data, struct wl_buffer * wl_buffer) {
	wl_buffer_destroy(wl_buffer);
	printf("wl_buffer_release_handled\n");
	if (window_ctx->wl_buffer_a == wl_buffer) {
		window_ctx->wl_buffer_a = NULL;
	}
	if (window_ctx->wl_buffer_b == wl_buffer) {
		window_ctx->wl_buffer_b = NULL;
	}
}

static const struct wl_buffer_listener wl_buffer_listener = {
	.release = wl_buffer_release_handler
};

static pixman_image_t * wlif_pixmanimage_create(uint32_t * framebuffer, int width, int height) {
	pixman_image_t * image;
	// PIXMAN_x8r8g8b8 like WL_SHM_FORMAT_XRGB8888
	image = pixman_image_create_bits_no_clear(PIXMAN_x8r8g8b8, width, height, framebuffer, width * 4);
	return image;
}

int wlif_adjustbuffer(struct wlif_window_context * ctx) {
	int new_fd = -1;
	uint32_t * buffer;
	pixman_image_t * image;
	ctx->stride = ctx->width * 4;
	ctx->size = ctx->stride * ctx->height * 2;
	if (ctx->curr_size < ctx->size) {
		wl_shm_pool_destroy(global_ctx->shm_pool);
		new_fd = shm_realloc(ctx->buffer_fd, ctx->size);
		ctx->curr_size = ctx->size;
		if (new_fd > 0) {
			ctx->buffer_fd = new_fd;
			fprintf(stdout, "buffer adjusted\n");
			global_ctx->shm_pool = wl_shm_create_pool(global_ctx->shm, ctx->buffer_fd, ctx->size);
		}
	}
	// mmap the buffer and draw a pattern
	buffer = (uint32_t *)mmap(NULL, ctx->size, PROT_READ | PROT_WRITE, MAP_SHARED, ctx->buffer_fd, 0);
	if (buffer != NULL) {
		image = wlif_pixmanimage_create(buffer, ctx->width, ctx->height);
		if (ctx->redraw) {
			ctx->redraw(image, WLIF_WINDOW_REDRAW_REASON_RESIZE);
		}
		for (int y = 0; y < ctx->height; ++y) {
			for (int x = 0; x < ctx->width; ++x) {
				if ((x + y / 8 * 8) % 16 < 8) {
					buffer[y*ctx->width+x] = 0x80668866;
				} else {
					buffer[y*ctx->width+x] = 0x8099BB99;
				}
			}
		}
		imageloader_render(image, 0,0, icon, imageloader_render_fittype_actual);
		munmap(buffer, ctx->size);
		pixman_image_unref(image);
	}
	struct wl_buffer * wl_buffer = wl_shm_pool_create_buffer(global_ctx->shm_pool, 0, ctx->width, ctx->height, ctx->stride, WL_SHM_FORMAT_XRGB8888);
	wl_buffer_add_listener(wl_buffer, &wl_buffer_listener, NULL);
	wl_surface_attach(ctx->wl_surface, wl_buffer, 0, 0);
	wl_surface_commit(ctx->wl_surface);
	if (ctx->wl_buffer_a == NULL) {
		ctx->wl_buffer_a = wl_buffer;
	} else
	if (ctx->wl_buffer_b == NULL) {
		ctx->wl_buffer_b = wl_buffer;
	}
	//wl_surface_attach(window_ctx->wl_surface, window_ctx->wl_buffer_a, 0, 0);
	return new_fd;
}

// **** POINTER HANDLING

static void wl_pointer_enter_handler(void * data, struct wl_pointer * wl_pointer, uint32_t serial, struct wl_surface * wl_surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
	fprintf(stdout, "pointer_enter\n");
}

static void wl_pointer_leave_handler(void * data, struct wl_pointer * wl_pointer, uint32_t serial, struct wl_surface * wl_surface) {
	fprintf(stdout, "pointer_leave\n");
}

static void wl_pointer_motion_handler(void * data, struct wl_pointer * wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
	//
}

static void wl_pointer_button_handler(void * data, struct wl_pointer * wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
	//
}

static void wl_pointer_axis_handler(void * data, struct wl_pointer * wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {
	//
}

static void wl_pointer_frame_handler(void * data, struct wl_pointer * wl_pointer) {
	fprintf(stdout, "pointer_frame\n");
}

static void wl_pointer_axis_source_handler(void * data, struct wl_pointer * wl_pointer, uint32_t axis_source) {
	//
}

static void wl_pointer_axis_stop_handler(void * data, struct wl_pointer * wl_pointer, uint32_t time, uint32_t axis) {
	//
}

static void wl_pointer_axis_discrete_handler(void * data, struct wl_pointer * wl_pointer, uint32_t axis, int32_t discrete) {
	//
}

// **** KEYBOARD HANDLING

static void wl_keyboard_keymap_handler(void * data, struct wl_keyboard * wl_keyboard, uint32_t format, int32_t fd, uint32_t size) {
	// assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);
	if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
		fprintf(stderr, "unexpected keymap format %u\n", format);
		exit(1);
	}
	char * map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	// assert(map_shm != MAP_FAILED);
	if (map_shm == MAP_FAILED) { fprintf(stderr, "keymap memory mapping failed\n"); goto error; }
	struct xkb_keymap * xkb_keymap = xkb_keymap_new_from_string(global_ctx->xkb_context, map_shm,
			XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
	munmap(map_shm, size);
	close(fd);

	struct xkb_state * xkb_state = xkb_state_new(xkb_keymap);
	xkb_keymap_unref(global_ctx->xkb_keymap);
	xkb_state_unref(global_ctx->xkb_state);
	global_ctx->xkb_keymap = xkb_keymap;
	global_ctx->xkb_state = xkb_state;
	return;
error:
	close(fd);
}

static void wl_keyboard_enter_handler(void * data, struct wl_keyboard * wl_keyboard, uint32_t serial, struct wl_surface * wl_surface, struct wl_array * keys) {
	if (global_ctx->xkb_state) {
		fprintf(stdout, "keyboard enter, keys pressed:\n");
		uint32_t * key;
		wl_array_for_each(key, keys) {
			char buf[128];
			xkb_keysym_t sym = xkb_state_key_get_one_sym(global_ctx->xkb_state, *key + 8);
			xkb_keysym_get_name(sym, buf, sizeof(buf));
			fprintf(stdout, "sym: %-12s (%d), ", buf, sym);
			xkb_state_key_get_utf8(global_ctx->xkb_state, *key + 8, buf, sizeof(buf));
			if (buf[0] == '\r' || buf[0] == '\n') { strcpy(buf, "."); }
			fprintf(stdout, "utf8: '%s'\n", buf);
		}
	}
}

static void wl_keyboard_key_handler(void * data, struct wl_keyboard * wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
	if (global_ctx->xkb_state) {
		char buf[128];
		uint32_t keycode = key + 8;
		xkb_keysym_t sym = xkb_state_key_get_one_sym(global_ctx->xkb_state, keycode);
		xkb_keysym_get_name(sym, buf, sizeof(buf));
		const char * action = state == WL_KEYBOARD_KEY_STATE_PRESSED ? "press" : "release";
		fprintf(stdout, "scancode %x - key %s: sym: %-12s (%d), ", key, action, buf, sym);
		xkb_state_key_get_utf8(global_ctx->xkb_state, keycode, buf, sizeof(buf));
		if (buf[0] == '\r' || buf[0] == '\n') { strcpy(buf, "."); }
		fprintf(stdout, "utf8: '%s'\n", buf);
	} else {
		fprintf(stdout, "new key scancode %d\n", key);
	}
}

static void wl_keyboard_leave_handler(void * data, struct wl_keyboard * wl_keyboard, uint32_t serial, struct wl_surface * wl_surface) {
	fprintf(stdout, "keyboard leave\n");
}

static void wl_keyboard_modifiers_handler(void * data, struct wl_keyboard * wl_keyboard, uint32_t serial,
		uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
	if (global_ctx->xkb_state) {
		xkb_state_update_mask(global_ctx->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
	}
}

static void wl_keyboard_repeat_info_handler(void * data, struct wl_keyboard * wl_keyboard, int32_t rate, int32_t delay) {
	/* TODO: left as an exercise for the reader */
}

// **** TOUCH HANDLING

static void wl_touch_down_handler(void * data, struct wl_touch * wl_touch, uint32_t serial, uint32_t time, struct wl_surface * wl_surface, int32_t id, wl_fixed_t x, wl_fixed_t y) {
	//
}

static void wl_touch_up_handler(void * data, struct wl_touch * wl_touch, uint32_t serial, uint32_t time, int32_t id) {

}

static void wl_touch_motion_handler(void * data, struct wl_touch * wl_touch, uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y) {

}

static void wl_touch_frame_handler(void * data, struct wl_touch * wl_touch) {

}

static void wl_touch_cancel_handler(void * data, struct wl_touch * wl_touch) {

}

static void wl_touch_shape_handler(void * data, struct wl_touch * wl_touch, int32_t id, wl_fixed_t major, wl_fixed_t minor) {

}

static void wl_touch_orientation_handler(void * data, struct wl_touch * wl_touch, int32_t id, wl_fixed_t orientation) {

}

void wl_seat_capabilities_handler(void * data, struct wl_seat * wl_seat, uint32_t capabilities) {
	// WL_SEAT_CAPABILITY_POINTER WL_SEAT_CAPABILITY_KEYBOARD WL_SEAT_CAPABILITY_TOUCH
	bool have_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
	if (have_pointer && global_ctx->pointer == NULL) {
		fprintf(stdout, "cap: POINTER\n");
		global_ctx->pointer = wl_seat_get_pointer(wl_seat);
		global_ctx->pointer_listener.enter = &wl_pointer_enter_handler;
		global_ctx->pointer_listener.leave = &wl_pointer_leave_handler;
		global_ctx->pointer_listener.motion = &wl_pointer_motion_handler;
		global_ctx->pointer_listener.button = &wl_pointer_button_handler;
		global_ctx->pointer_listener.axis = &wl_pointer_axis_handler;
		global_ctx->pointer_listener.frame = &wl_pointer_frame_handler;
		global_ctx->pointer_listener.axis_source = &wl_pointer_axis_source_handler;
		global_ctx->pointer_listener.axis_stop = &wl_pointer_axis_stop_handler;
		global_ctx->pointer_listener.axis_discrete = &wl_pointer_axis_discrete_handler;
		wl_pointer_add_listener(global_ctx->pointer, &global_ctx->pointer_listener, NULL);
	}

	bool have_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD ? 1 : 0;
	if (have_keyboard && global_ctx->keyboard == NULL) {
		fprintf(stdout, "cap: KEYBOARD\n");
		global_ctx->keyboard = wl_seat_get_keyboard(wl_seat);
		global_ctx->keyboard_listener.keymap = &wl_keyboard_keymap_handler;
		global_ctx->keyboard_listener.enter = &wl_keyboard_enter_handler;
		global_ctx->keyboard_listener.leave = &wl_keyboard_leave_handler;
		global_ctx->keyboard_listener.key = &wl_keyboard_key_handler;
		global_ctx->keyboard_listener.modifiers = &wl_keyboard_modifiers_handler;
		global_ctx->keyboard_listener.repeat_info = &wl_keyboard_repeat_info_handler;
		wl_keyboard_add_listener(global_ctx->keyboard, &global_ctx->keyboard_listener, NULL);
	} else
	if (!have_keyboard && global_ctx->keyboard != NULL) {
		wl_keyboard_release(global_ctx->keyboard);
		global_ctx->keyboard = NULL;
		fprintf(stdout, "cap: KEYBOARD (GONE)\n");
	}

	bool have_touch = capabilities & WL_SEAT_CAPABILITY_TOUCH;
	if (have_touch && global_ctx->touch == NULL) {
		fprintf(stdout, "cap: TOUCH\n");
		global_ctx->touch = wl_seat_get_touch(wl_seat);
		global_ctx->touch_listener.down = &wl_touch_down_handler;
		global_ctx->touch_listener.up = &wl_touch_up_handler;
		global_ctx->touch_listener.motion = &wl_touch_motion_handler;
		global_ctx->touch_listener.frame = &wl_touch_frame_handler;
		global_ctx->touch_listener.cancel = &wl_touch_cancel_handler;
		global_ctx->touch_listener.shape = &wl_touch_shape_handler;
		global_ctx->touch_listener.orientation = &wl_touch_orientation_handler;
		wl_touch_add_listener(global_ctx->touch, &global_ctx->touch_listener, NULL);
	}
}

void wl_seat_name_handler(void * data, struct wl_seat * wl_seat, const char * name) {
	fprintf(stdout, "seat name: %s\n", name);
}

int wlif_initialise() {
	int rc = imageloader_load(&icon, "testdata/image-x-generic.png");
	if (rc != 0) {
		fprintf(stderr, "cannot load image\n");
		return -1;
	}

	window_ctx->buffer_fd = -1;
	window_ctx->redraw = NULL;
	wl_list_init(&global_ctx->seats);
	wl_list_init(&global_ctx->outputs);
	global_ctx->display = wl_display_connect(NULL);
	if (!global_ctx->display) {
		fprintf(stderr, "wl_display_connect() failed\n");
		return -1;
	}
	global_ctx->registry = wl_display_get_registry(global_ctx->display);
	if (!global_ctx->registry) {
		fprintf(stderr, "wl_display_get_registry() failed\n");
		return -1;
	}
	global_ctx->registry_listener.global = registry_global_handler;
	global_ctx->registry_listener.global_remove = registry_global_remove_handler;
	wl_registry_add_listener(global_ctx->registry, &global_ctx->registry_listener, NULL);

	wl_display_roundtrip(global_ctx->display);

	fprintf(stdout, "compositor %p\n", (void *)global_ctx->compositor);
	fprintf(stdout, "shm %p\n", (void *)global_ctx->shm);
	fprintf(stdout, "xdg_wm_base %p\n", (void *)global_ctx->xdg_wm_base);
	fprintf(stdout, "zxdg_decoration_manager_v1 %p\n", (void *)global_ctx->zxdg_decoration_manager_v1);
	struct wlif_ptr_element * ele;
	struct wl_seat * seat = NULL;
	wl_list_for_each (ele, &global_ctx->seats, element_header) {
		seat = (struct wl_seat *)ele->element;
		fprintf(stdout, "seat %p\n", (void *)seat);
	}

	if (!(global_ctx->compositor && global_ctx->shm && global_ctx->xdg_wm_base)) {
		fprintf(stderr, "could not get compositor, shm and/or xdg_wm_base from registry\n");
		return -1;
	}

	if (wl_list_length(&global_ctx->seats) == 0) {
		fprintf(stderr, "could not get seat\n");
		return -1;
	}

	global_ctx->xdg_wm_base_listener.ping = xdg_wm_base_ping_handler;
	xdg_wm_base_add_listener(global_ctx->xdg_wm_base, &global_ctx->xdg_wm_base_listener, NULL);

	window_ctx->wl_surface = wl_compositor_create_surface(global_ctx->compositor);
	window_ctx->width = 200;
	window_ctx->height = 200;
	window_ctx->stride = window_ctx->width * 4;
	window_ctx->size = window_ctx->stride * window_ctx->height * 2;
	window_ctx->curr_size = window_ctx->size;
	window_ctx->buffer_fd = shm_realloc(-1, window_ctx->size);

	if (window_ctx->buffer_fd < 0) {
		fprintf(stderr, "error creating buffer shm file errno: %d\n", errno);
		return -1;
	}

	global_ctx->shm_pool = wl_shm_create_pool(global_ctx->shm, window_ctx->buffer_fd, window_ctx->size);

	fprintf(stdout, "shm_pool %p\n", (void *)global_ctx->shm_pool);
	if (!(global_ctx->shm_pool)) {
		fprintf(stderr, "could not create shm pool\n");
		return -1;
	}

	window_ctx->wl_buffer_a = wl_shm_pool_create_buffer(global_ctx->shm_pool, 0, window_ctx->width, window_ctx->height, window_ctx->stride, WL_SHM_FORMAT_XRGB8888);
	wl_buffer_add_listener(window_ctx->wl_buffer_a, &wl_buffer_listener, NULL);

	window_ctx->xdg_surface = xdg_wm_base_get_xdg_surface(global_ctx->xdg_wm_base, window_ctx->wl_surface);

//

	fprintf(stdout, "wl_surface %p\n", (void *)window_ctx->wl_surface);
	fprintf(stdout, "wl_buffer_a %p\n", (void *)window_ctx->wl_buffer_a);
	fprintf(stdout, "xdg_surface %p\n", (void *)window_ctx->xdg_surface);

	if (!(window_ctx->wl_surface && window_ctx->wl_buffer_a && window_ctx->xdg_surface)) {
		fprintf(stderr, "could not create surface or buffer\n");
		return -1;
	}

	window_ctx->xdg_surface_listener.configure = xdg_surface_configure_handler;
	xdg_surface_add_listener(window_ctx->xdg_surface, &window_ctx->xdg_surface_listener, (void *)window_ctx);

	window_ctx->xdg_toplevel = xdg_surface_get_toplevel(window_ctx->xdg_surface);
	xdg_toplevel_set_title(window_ctx->xdg_toplevel, "fiv");
	window_ctx->xdg_toplevel_listener.configure = xdg_toplevel_configure_handler;
	window_ctx->xdg_toplevel_listener.close = xdg_toplevel_close_handler;
	xdg_toplevel_add_listener(window_ctx->xdg_toplevel, &window_ctx->xdg_toplevel_listener, NULL);

	// CSD/SSD stuff
	if (global_ctx->zxdg_decoration_manager_v1) {
		window_ctx->zxdg_toplevel_decoration_v1 = zxdg_decoration_manager_v1_get_toplevel_decoration(global_ctx->zxdg_decoration_manager_v1, window_ctx->xdg_toplevel);
	}

	global_ctx->seat_listener.capabilities = &wl_seat_capabilities_handler;
	global_ctx->seat_listener.name = &wl_seat_name_handler;

	{
		seat = NULL;
		wl_list_for_each(ele, &global_ctx->seats, element_header) {
			seat = (struct wl_seat *)ele->element;
			break;
		}
		if (seat == NULL) {
			fprintf(stderr, "no seat!\n");
		} else {
			wl_seat_add_listener(seat, &global_ctx->seat_listener, NULL);
		}
	}

	//xdg_surface_set_window_geometry(window_ctx->xdg_surface, 0, 0, 200, 200);

	global_ctx->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	return 0;
}

int wlif_dispose() {
	pixman_image_unref(icon);
	return 0;
}

