#ifndef __HAVE_PHUCK_OFF_MMAP_H__
#define __HAVE_PHUCK_OFF_MMAP_H__

#ifndef PHUCK_OFF_MMAP_FILE
#define PHUCK_OFF_MMAP_FILE "/tmp/phuck-off.mmap"
#endif

// Exposed so phuck_off_mmap_set() can stay as a tiny hot-path inline.
extern unsigned char* phuck_off_mmap_bytes;

#if defined(_MSC_VER)
#define PHUCK_OFF_MMAP_INLINE static __inline
#else
#define PHUCK_OFF_MMAP_INLINE static inline
#endif

int phuck_off_mmap_init(const int n);
void phuck_off_mmap_shutdown(void);

PHUCK_OFF_MMAP_INLINE void phuck_off_mmap_set(const int i) {
    phuck_off_mmap_bytes[((unsigned int) i) >> 3] |= (unsigned char) (1u << (((unsigned int) i) & 7u));
}

#undef PHUCK_OFF_MMAP_INLINE

#endif
