#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "phuck_off_mmap.h"

static int failures = 0;
static char test_path[] = "/tmp/phuck-off.mmap.test.XXXXXX";

static void assert_true(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        failures = 1;
    }
}

static off_t file_size(const char* path) {
    struct stat sb;

    if (stat(path, &sb) != 0) {
        return -1;
    }

    return sb.st_size;
}

static void read_file_bytes(unsigned char* buffer, size_t byte_count) {
    FILE* fp;
    size_t read_count;

    fp = fopen(test_path, "rb");
    assert_true(fp != NULL, "failed to open mmap backing file");
    if (!fp) {
        memset(buffer, 0, byte_count);
        return;
    }

    read_count = fread(buffer, 1, byte_count, fp);
    assert_true(read_count == byte_count, "failed to read mmap backing file");
    fclose(fp);
}

static void remove_test_file(void) {
    phuck_off_mmap_shutdown();
    if (unlink(test_path) != 0 && errno != ENOENT) {
        assert_true(0, "failed to remove mmap test file");
    }
}

static void run_invalid_init_case(void) {
    remove_test_file();

    assert_true(!phuck_off_mmap_init(test_path, 0), "init(0) should fail");
    assert_true(access(test_path, F_OK) != 0, "init(0) should not create a backing file");
}

static void run_create_and_set_case(void) {
    unsigned char file_bytes[2];

    remove_test_file();

    assert_true(phuck_off_mmap_init(test_path, 10), "init(10) should succeed");
    assert_true(phuck_off_mmap_bytes != NULL, "mapped bytes should be available after init");
    assert_true(file_size(test_path) == 2, "10 bits should allocate 2 bytes");
    assert_true(phuck_off_mmap_bytes[0] == 0, "first byte should be zero after init");
    assert_true(phuck_off_mmap_bytes[1] == 0, "second byte should be zero after init");

    phuck_off_mmap_set(0);
    phuck_off_mmap_set(4);
    phuck_off_mmap_set(9);
    phuck_off_mmap_set(9);

    assert_true(phuck_off_mmap_bytes[0] == 0x11, "bits 0 and 4 should be set in the first byte");
    assert_true(phuck_off_mmap_bytes[1] == 0x02, "bit 9 should be set in the second byte");

    assert_true(msync((void*) phuck_off_mmap_bytes, 2, MS_SYNC) == 0, "failed to flush mmap backing file");
    read_file_bytes(file_bytes, sizeof(file_bytes));
    assert_true(file_bytes[0] == 0x11, "backing file first byte mismatch");
    assert_true(file_bytes[1] == 0x02, "backing file second byte mismatch");
}

static void run_reinit_case(void) {
    assert_true(phuck_off_mmap_init(test_path, 17), "reinit to 17 bits should succeed");
    assert_true(file_size(test_path) == 3, "17 bits should allocate 3 bytes");
    assert_true(phuck_off_mmap_bytes[0] == 0, "reinit should zero the first byte");
    assert_true(phuck_off_mmap_bytes[1] == 0, "reinit should zero the second byte");
    assert_true(phuck_off_mmap_bytes[2] == 0, "reinit should zero the third byte");
}

static void run_shutdown_case(void) {
    phuck_off_mmap_shutdown();
    assert_true(phuck_off_mmap_bytes == NULL, "shutdown should clear the mapped pointer");
    assert_true(access(test_path, F_OK) != 0, "shutdown should remove the backing file");
}

int main(void) {
    int fd;

    fd = mkstemp(test_path);
    assert_true(fd >= 0, "failed to allocate mmap test path");
    if (fd < 0) {
        return 1;
    }

    close(fd);
    unlink(test_path);

    run_invalid_init_case();
    run_create_and_set_case();
    run_reinit_case();
    run_shutdown_case();

    if (failures) {
        return 1;
    }

    printf("ok\n");
    return 0;
}
