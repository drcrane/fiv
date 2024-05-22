#define _DEFAULT_SOURCE

#include <syscall.h>
#include <unistd.h>
#include <sys/mman.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

#include "dbg.h"
#include "wlif.h"

int main(int argc, char *argv[]) {
	int res = 0;
	int make_fullscreen = 0;

	res = wlif_initialise();
	fprintf(stdout, "wlif_initialise: %i\n", res);

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--fullscreen") == 0) {
			make_fullscreen = 1;
		}
	}

	if (make_fullscreen) {
		window_ctx->request_fullscreen = 1;
	}

	wl_surface_commit(window_ctx->wl_surface);
	//wl_display_roundtrip(global_ctx->display);
	//wl_surface_attach(window_ctx->wl_surface, window_ctx->wl_buffer_a, 0, 0);
	//wl_surface_commit(window_ctx->wl_surface);

	int keep_going = 1;
	do {
		res = wlif_process_pending(1000);
		//fprintf(stdout, "Process Pending : %d\n", res);
		if (window_ctx->request_fullscreen && window_ctx->xdg_toplevel_configured) {
			xdg_toplevel_set_fullscreen(window_ctx->xdg_toplevel, NULL);
			fprintf(stdout, "Setting fullscreen...\n");
			window_ctx->request_fullscreen = 0;
		}
		if (global_ctx->terminate) {
			keep_going = 0;
		}
	} while (keep_going);

	/*
	int wayland_fd = wl_display_get_fd(global_ctx->display);
	struct pollfd fds[1];
	fds[0].fd = wayland_fd;
	fds[0].events = POLLIN;
	int keep_going = 1;
	do {
		if (window_ctx->request_fullscreen && window_ctx->xdg_toplevel_configured) {
			xdg_toplevel_set_fullscreen(window_ctx->xdg_toplevel, NULL);
			fprintf(stdout, "Setting fullscreen...\n");
			window_ctx->request_fullscreen = 0;
		}
		res = wl_display_dispatch_pending(global_ctx->display);
		// This is not good, because if it fails (the socket buffer is too
		// full) then it should not be called again until the socket is ready
		// to accept more data...
		// TODO: Fix this.
		res = wl_display_flush(global_ctx->display);
		fds[0].revents = 0;
		res = poll(fds, 1, 1000);
		if (res > 0) {
			if (fds[0].revents & POLLIN) {
				res = wl_display_prepare_read(global_ctx->display);
				if (res) {
					// This will happen if there are events waiting on the
					// queue or if another thread has requested to read
					// from the socket.
					// Hopefully wl_display_dispatch_pending will empty the
					// queue for us.
					fprintf(stdout, "Could not prepare to read events\n");
				} else {
					res = wl_display_read_events(global_ctx->display);
					fprintf(stdout, "events read from socket\n");
				}
				fds[0].revents &= ~POLLIN;
				// this could be called to read or dispatch events but the
				// logic in wl_display_dispatch() is similar to above and so
				// some steps would be repeated.
				//res = wl_display_dispatch(global_ctx->display);
				//fprintf(stdout, "%d events dispatched\n", res);
			}
		} else if (res == 0) {
			// do nothing
		} else {
			fprintf(stderr, "Connection to wayland server broken?\n");
			keep_going = 0;
		}
		if (global_ctx->terminate) {
			keep_going = 0;
		}
	} while (keep_going);
	*/

	// this event loop is correct but only responds to events from the
	// compositor
	/*
	while ((res = wl_display_dispatch(global_ctx->display)) != -1) {
		if (window_ctx->request_fullscreen && window_ctx->xdg_toplevel_configured) {
			xdg_toplevel_set_fullscreen(window_ctx->xdg_toplevel, NULL);
			fprintf(stdout, "Setting fullscreen...\n");
			window_ctx->request_fullscreen = 0;
		}
		if (global_ctx->terminate) {
			break;
		}
	}
	*/

	fprintf(stdout, "Terminated: %s (%d)\n", strerror(errno), errno);

	//res = wl_display_dispatch(global_ctx->display);
	//fprintf(stdout, "res: %d, errno: %d\n", res, errno);

	//res = wl_display_roundtrip(global_ctx->display);
	//fprintf(stdout, "res: %d, errno: %d\n", res, errno);

	wl_display_disconnect(global_ctx->display);

error:
	return res;
}
