#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "phuck_off_logger.h"
#include "phuck_off_mmap.h"

#define PHUCK_OFF_MMAP_FLUSH_INTERVAL_SECONDS 3
#define PHUCK_OFF_MMAP_PATH_TEMPLATE "/tmp/phuck_off_map_%ld"

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

static void phuck_off_mmap_log_init_error(const char* path, const int n, const char* reason) {
    phuck_off_log(
        PHUCK_OFF_LOG_LEVEL_ERROR,
        "Failed to initialize phuck-off mmap path=\"%s\" functions=%d: %s",
        path ? path : "(null)",
        n,
        reason
    );
}

static void phuck_off_mmap_log_init_errno_error(const char* path, const int n, const char* operation, const int saved_errno) {
    phuck_off_log(
        PHUCK_OFF_LOG_LEVEL_ERROR,
        "Failed to initialize phuck-off mmap path=\"%s\" functions=%d: %s: %s (%d)",
        path,
        n,
        operation,
        strerror(saved_errno),
        saved_errno
    );
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

int phuck_off_mmap_init_for_pid(const int n) {
    char path[64];
    const int written = snprintf(path, sizeof(path), PHUCK_OFF_MMAP_PATH_TEMPLATE, (long) getpid());

    if (written <= 0 || (size_t) written >= sizeof(path)) {
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "Failed to build phuck-off mmap path for pid %ld", (long) getpid());
        return 0;
    }

    return phuck_off_mmap_init(path, n);
}

int phuck_off_mmap_init(const char* path, const int n) {
    size_t byte_count;
    void* mapping;
    int fd;
    char* path_copy;
    time_t now;
    int saved_errno;

    if (!path || path[0] == '\0') {
        phuck_off_mmap_log_init_error(path, n, "invalid path");
        return 0;
    }

    if (n <= 0) {
        phuck_off_mmap_log_init_error(path, n, "invalid function count");
        return 0;
    }

    phuck_off_mmap_shutdown();

    byte_count = phuck_off_mmap_byte_count(n);
    path_copy = phuck_off_mmap_strdup(path);
    if (!path_copy) {
        phuck_off_mmap_log_init_error(path, n, "memory allocation failed");
        return 0;
    }

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        saved_errno = errno;
        free(path_copy);
        phuck_off_mmap_log_init_errno_error(path, n, "open() failed", saved_errno);
        return 0;
    }

    if (ftruncate(fd, (off_t) byte_count) != 0) {
        saved_errno = errno;
        close(fd);
        unlink(path);
        free(path_copy);
        phuck_off_mmap_log_init_errno_error(path, n, "ftruncate() failed", saved_errno);
        return 0;
    }

    mapping = mmap(NULL, byte_count, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapping == MAP_FAILED) {
        saved_errno = errno;
        close(fd);
        unlink(path);
        free(path_copy);
        phuck_off_mmap_log_init_errno_error(path, n, "mmap() failed", saved_errno);
        return 0;
    }

    phuck_off_mmap_state.fd = fd;
    phuck_off_mmap_state.byte_count = byte_count;
    phuck_off_mmap_state.path = path_copy;
    now = time(NULL);
    phuck_off_mmap_state.last_flush_at = now == (time_t) -1 ? 0 : now;
    phuck_off_mmap_bytes = (unsigned char*) mapping;
    if (now == (time_t) -1) {
        saved_errno = errno;
        phuck_off_log(
            PHUCK_OFF_LOG_LEVEL_ERROR,
            "time() failed during phuck-off mmap init for \"%s\": %s (%d)",
            path,
            strerror(saved_errno),
            saved_errno
        );
    }
    phuck_off_log(PHUCK_OFF_LOG_LEVEL_INFO, "Initialized phuck-off mmap path=\"%s\" functions=%d", path, n);

    return 1;
}

void phuck_off_mmap_post_request(void) {
    time_t now;
    int saved_errno;
    long elapsed_since_flush;
    long duration_sec;
    long duration_usec;
    struct timeval start_time;
    struct timeval end_time;
    unsigned long duration_us;

    if (phuck_off_mmap_bytes == NULL || phuck_off_mmap_state.byte_count == 0) {
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_TRACE, "phuck_off_mmap_post_request: syncing=no reason=no-active-mmap");
        return;
    }

    now = time(NULL);
    if (now == (time_t) -1) {
        saved_errno = errno;
        phuck_off_log(
            PHUCK_OFF_LOG_LEVEL_ERROR,
            "time() failed during phuck_off_mmap_post_request: %s (%d)",
            strerror(saved_errno),
            saved_errno
        );
        return;
    }

    elapsed_since_flush = phuck_off_mmap_state.last_flush_at == 0 ? -1 : (long) (now - phuck_off_mmap_state.last_flush_at);
    if (phuck_off_mmap_state.last_flush_at != 0 && elapsed_since_flush <= PHUCK_OFF_MMAP_FLUSH_INTERVAL_SECONDS) {
        phuck_off_log(
            PHUCK_OFF_LOG_LEVEL_TRACE,
            "phuck_off_mmap_post_request: syncing=no reason=throttled elapsed=%ld threshold=%d path=\"%s\"",
            elapsed_since_flush,
            PHUCK_OFF_MMAP_FLUSH_INTERVAL_SECONDS,
            phuck_off_mmap_state.path
        );
        return;
    }

    if (gettimeofday(&start_time, NULL) != 0) {
        saved_errno = errno;
        phuck_off_log(
            PHUCK_OFF_LOG_LEVEL_ERROR,
            "gettimeofday() failed before msync() for \"%s\": %s (%d)",
            phuck_off_mmap_state.path,
            strerror(saved_errno),
            saved_errno
        );
        return;
    }

    if (msync((void*) phuck_off_mmap_bytes, phuck_off_mmap_state.byte_count, MS_SYNC) != 0) {
        saved_errno = errno;
        phuck_off_log(
            PHUCK_OFF_LOG_LEVEL_ERROR,
            "msync() failed for \"%s\": %s (%d)",
            phuck_off_mmap_state.path,
            strerror(saved_errno),
            saved_errno
        );
        return;
    }

    if (gettimeofday(&end_time, NULL) != 0) {
        saved_errno = errno;
        phuck_off_log(
            PHUCK_OFF_LOG_LEVEL_ERROR,
            "gettimeofday() failed after msync() for \"%s\": %s (%d)",
            phuck_off_mmap_state.path,
            strerror(saved_errno),
            saved_errno
        );
        return;
    }

    duration_sec = end_time.tv_sec - start_time.tv_sec;
    duration_usec = end_time.tv_usec - start_time.tv_usec;
    if (duration_usec < 0) {
        duration_sec--;
        duration_usec += 1000000;
    }
    duration_us = (unsigned long) duration_sec * 1000000ul + (unsigned long) duration_usec;
    phuck_off_mmap_state.last_flush_at = now;
    phuck_off_log(
        PHUCK_OFF_LOG_LEVEL_DEBUG,
        "phuck_off_mmap_post_request: syncing=yes bytes=%lu duration_us=%lu path=\"%s\"",
        (unsigned long) phuck_off_mmap_state.byte_count,
        duration_us,
        phuck_off_mmap_state.path
    );
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
