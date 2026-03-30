#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "phuck_off_mmap.h"

#define PHUCK_OFF_MMAP_FLUSH_INTERVAL_SECONDS 3

typedef struct phuck_off_mmap {
    int fd;
    size_t byte_count;
    char* path;
    time_t last_flush_at;
} phuck_off_mmap;

unsigned char* phuck_off_mmap_bytes = NULL;

static phuck_off_mmap phuck_off_mmap_state = { -1, 0, NULL, 0 };

static size_t phuck_off_mmap_byte_count(const int n) {
    return (((size_t) n) + 7u) >> 3;
}

static char* phuck_off_mmap_strdup(const char* path) {
    size_t len;
    char* copy;

    len = strlen(path);
    copy = (char*) malloc(len + 1);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, path, len + 1);
    return copy;
}

int phuck_off_mmap_init(const char* path, const int n) {
    size_t byte_count;
    void* mapping;
    int fd;
    char* path_copy;
    time_t now;

    if (!path || path[0] == '\0' || n <= 0) {
        return 0;
    }

    phuck_off_mmap_shutdown();

    byte_count = phuck_off_mmap_byte_count(n);
    path_copy = phuck_off_mmap_strdup(path);
    if (!path_copy) {
        return 0;
    }

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        free(path_copy);
        return 0;
    }

    if (ftruncate(fd, (off_t) byte_count) != 0) {
        close(fd);
        unlink(path);
        free(path_copy);
        return 0;
    }

    mapping = mmap(NULL, byte_count, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapping == MAP_FAILED) {
        close(fd);
        unlink(path);
        free(path_copy);
        return 0;
    }

    phuck_off_mmap_state.fd = fd;
    phuck_off_mmap_state.byte_count = byte_count;
    phuck_off_mmap_state.path = path_copy;
    now = time(NULL);
    phuck_off_mmap_state.last_flush_at = now == (time_t) -1 ? 0 : now;
    phuck_off_mmap_bytes = (unsigned char*) mapping;

    return 1;
}

int phuck_off_mmap_post_request(void) {
    time_t now;

    if (phuck_off_mmap_bytes == NULL || phuck_off_mmap_state.byte_count == 0) {
        return 0;
    }

    now = time(NULL);
    if (now == (time_t) -1) {
        return 0;
    }

    if (phuck_off_mmap_state.last_flush_at != 0 && now - phuck_off_mmap_state.last_flush_at <= PHUCK_OFF_MMAP_FLUSH_INTERVAL_SECONDS) {
        return 0;
    }

    if (msync((void*) phuck_off_mmap_bytes, phuck_off_mmap_state.byte_count, MS_SYNC) != 0) {
        return -1;
    }

    phuck_off_mmap_state.last_flush_at = now;
    return 1;
}

void phuck_off_mmap_shutdown(void) {
    if (phuck_off_mmap_bytes != NULL) {
        munmap((void*) phuck_off_mmap_bytes, phuck_off_mmap_state.byte_count);
        phuck_off_mmap_bytes = NULL;
    }

    if (phuck_off_mmap_state.fd >= 0) {
        close(phuck_off_mmap_state.fd);
        phuck_off_mmap_state.fd = -1;
    }

    phuck_off_mmap_state.byte_count = 0;
    phuck_off_mmap_state.last_flush_at = 0;
    if (phuck_off_mmap_state.path != NULL) {
        unlink(phuck_off_mmap_state.path);
        free(phuck_off_mmap_state.path);
        phuck_off_mmap_state.path = NULL;
    }
}
