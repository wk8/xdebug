#include <fcntl.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "phuck_off_mmap.h"

typedef struct phuck_off_mmap {
    int fd;
    size_t byte_count;
} phuck_off_mmap;

unsigned char* phuck_off_mmap_bytes = NULL;

static phuck_off_mmap phuck_off_mmap_state = { -1, 0 };

static size_t phuck_off_mmap_byte_count(const int n) {
    return (((size_t) n) + 7u) >> 3;
}

int phuck_off_mmap_init(const int n) {
    size_t byte_count;
    void* mapping;
    int fd;

    if (n <= 0) {
        return 0;
    }

    phuck_off_mmap_shutdown();

    byte_count = phuck_off_mmap_byte_count(n);
    fd = open(PHUCK_OFF_MMAP_FILE, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        return 0;
    }

    if (ftruncate(fd, (off_t) byte_count) != 0) {
        close(fd);
        unlink(PHUCK_OFF_MMAP_FILE);
        return 0;
    }

    mapping = mmap(NULL, byte_count, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapping == MAP_FAILED) {
        close(fd);
        unlink(PHUCK_OFF_MMAP_FILE);
        return 0;
    }

    phuck_off_mmap_state.fd = fd;
    phuck_off_mmap_state.byte_count = byte_count;
    phuck_off_mmap_bytes = (unsigned char*) mapping;

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
    unlink(PHUCK_OFF_MMAP_FILE);
}
