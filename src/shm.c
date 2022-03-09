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

static void randname(char * buf) {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	long r = ts.tv_nsec;
	for (int i = 0; i < 6; ++i) {
		buf[i] = 'A' + (r & 15) + (r & 16) * 2;
		r >>= 5;
	}
}

static int create_shm_file() {
	int retries = 10;
	do {
		char name[] = "/wl_shm-XXXXXX";
		randname(name + sizeof(name) - 7);
		--retries;
		// shm_open creates a file in the /dev/shm directory it would seem
		int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
		if (fd >= 0) {
			shm_unlink(name);
			return fd;
		}
	} while (retries > 0 && errno == EEXIST);
	return -1;
}

int shm_realloc(int curr_fd, size_t size) {
	int fd;
	if (curr_fd != -1) {
		fd = curr_fd;
	} else {
		fd = create_shm_file();
	}
	if (fd < 0) {
		return -1;
	}
	int ret;
	do {
		ret = ftruncate(fd, size);
	} while (ret < 0 && errno == EINTR);
	if (ret < 0) {
		close(fd);
		return -1;
	}
	return fd;
}

