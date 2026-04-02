#ifndef __HAVE_PHUCK_OFF_MMAP_H__
#define __HAVE_PHUCK_OFF_MMAP_H__

#include <sys/types.h>

#ifndef PHUCK_OFF_NO_CLEANUP_ENV_VAR
#define PHUCK_OFF_NO_CLEANUP_ENV_VAR "PHUCK_OFF_NO_CLEANUP"
#endif

// Exposed so phuck_off_mmap_set() can stay as a tiny hot-path inline.
extern unsigned char* phuck_off_mmap_bytes;

int phuck_off_mmap_init_for_pid(const int n);
int phuck_off_mmap_init(const char* path, const int n);
void phuck_off_mmap_post_request(void);
void phuck_off_mmap_shutdown(void);

static inline void phuck_off_mmap_set(const int i) {
    phuck_off_mmap_bytes[((unsigned int) i) >> 3] |= (unsigned char) (1u << (((unsigned int) i) & 7u));
}

#endif
