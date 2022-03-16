#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
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

#include "shm.h"
#include "wlif.h"
#include "imageloader.h"

struct wlif_global_context wlif_global_ctx_a;
struct wlif_global_context * global_ctx = &wlif_global_ctx_a;

struct wlif_window_context wlif_window_ctx_a;
struct wlif_window_context * window_ctx = &wlif_window_ctx_a;

pixman_image_t * icon;

int wlif_adjustbuffer(struct wlif_window_context * ctx);

void registry_global_handler(void * data, struct wl_registry * registry, uint32_t name, const char * interface, uint32_t version) {
	fprintf(stdout, "interface: '%s', version: %u, name: %u\n", interface, version, name);
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		global_ctx->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 3);
	}
	if (strcmp(interface, "wl_shm") == 0) {
		global_ctx->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}
	if (strcmp(interface, "xdg_wm_base") == 0) {
		global_ctx->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 2);
	}
	if (strcmp(interface, "zxdg_decoration_manager_v1") == 0) {
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
	// PIXMAN_a8r8g8b8 like WL_SHM_FORMAT_XRGB8888
	image = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, width, height, framebuffer, width * 4);
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
					buffer[y*ctx->width+x] = 0xFF668866;
				} else {
					buffer[y*ctx->width+x] = 0xFF99BB99;
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


int wlif_initialise() {
	int rc = imageloader_load(&icon, "testdata/image-x-generic.png");
	if (rc != 0) {
		fprintf(stderr, "cannot load image\n");
		return -1;
	}

	window_ctx->buffer_fd = -1;
	window_ctx->redraw = NULL;
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

	if (!(global_ctx->compositor && global_ctx->shm && global_ctx->xdg_wm_base)) {
		fprintf(stderr, "could not get compositor, shm and/or xdg_wm_base from registry\n");
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

	//window_ctx->zxdg_decoration_manager_v1 = zxdg_decoration_manager_v1_get_toplevel_decoration(global_ctx->);
	window_ctx->zxdg_toplevel_decoration_v1 = zxdg_decoration_manager_v1_get_toplevel_decoration(global_ctx->zxdg_decoration_manager_v1, window_ctx->xdg_toplevel);

	//xdg_surface_set_window_geometry(window_ctx->xdg_surface, 0, 0, 200, 200);

	return 0;
}

int wlif_dispose() {
	pixman_image_unref(icon);
	return 0;
}

