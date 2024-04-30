#define _DEFAULT_SOURCE

#include <syscall.h>
#include <unistd.h>
#include <sys/mman.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

#include "dbg.h"
#include "wlif.h"

int main(int argc, char *argv[]) {
	int res = 0;

	res = wlif_initialise();
	fprintf(stdout, "wlif_initialise: %i\n", res);

	wl_surface_commit(window_ctx->wl_surface);
	//wl_display_roundtrip(global_ctx->display);
	//wl_surface_attach(window_ctx->wl_surface, window_ctx->wl_buffer_a, 0, 0);
	//wl_surface_commit(window_ctx->wl_surface);

	while (res = wl_display_dispatch(global_ctx->display) != -1) {
		if (global_ctx->terminate) {
			break;
		}
	}

	fprintf(stdout, "Terminated: %s (%d)\n", strerror(errno), errno);

	res = wl_display_dispatch(global_ctx->display);
	fprintf(stdout, "res: %d, errno: %d\n", res, errno);

	wl_display_disconnect(global_ctx->display);

error:
	return res;
}
