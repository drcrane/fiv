#ifndef SHM_H
#define SHM_H

#include <stddef.h>

int shm_realloc(int curr_fd, size_t size);

#endif /* SHM_H */

