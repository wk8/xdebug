#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "phuck_off_mmap.h"

static int failures = 0;
static char backup_template[] = "/tmp/phuck-off.mmap.backup.XXXXXX";
static int backup_exists = 0;

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

    fp = fopen(PHUCK_OFF_MMAP_FILE, "rb");
    assert_true(fp != NULL, "failed to open mmap backing file");
    if (!fp) {
        memset(buffer, 0, byte_count);
        return;
    }

    read_count = fread(buffer, 1, byte_count, fp);
    assert_true(read_count == byte_count, "failed to read mmap backing file");
    fclose(fp);
}

static void backup_existing_file(void) {
    int fd;

    if (access(PHUCK_OFF_MMAP_FILE, F_OK) != 0) {
        backup_exists = 0;
        return;
    }

    fd = mkstemp(backup_template);
    assert_true(fd >= 0, "failed to create mmap backup path");
    if (fd < 0) {
        return;
    }

    close(fd);
    unlink(backup_template);

    assert_true(rename(PHUCK_OFF_MMAP_FILE, backup_template) == 0, "failed to back up existing mmap file");
    if (!failures) {
        backup_exists = 1;
    }
}

static void restore_existing_file(void) {
    phuck_off_mmap_shutdown();

    if (backup_exists) {
        assert_true(rename(backup_template, PHUCK_OFF_MMAP_FILE) == 0, "failed to restore original mmap file");
        backup_exists = 0;
    }
}

static void remove_test_file(void) {
    phuck_off_mmap_shutdown();
    if (unlink(PHUCK_OFF_MMAP_FILE) != 0 && errno != ENOENT) {
        assert_true(0, "failed to remove mmap test file");
    }
}

static void run_invalid_init_case(void) {
    remove_test_file();

    assert_true(!phuck_off_mmap_init(0), "init(0) should fail");
    assert_true(access(PHUCK_OFF_MMAP_FILE, F_OK) != 0, "init(0) should not create a backing file");
}

static void run_create_and_set_case(void) {
    unsigned char file_bytes[2];

    remove_test_file();

    assert_true(phuck_off_mmap_init(10), "init(10) should succeed");
    assert_true(phuck_off_mmap_bytes != NULL, "mapped bytes should be available after init");
    assert_true(file_size(PHUCK_OFF_MMAP_FILE) == 2, "10 bits should allocate 2 bytes");
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
    assert_true(phuck_off_mmap_init(17), "reinit to 17 bits should succeed");
    assert_true(file_size(PHUCK_OFF_MMAP_FILE) == 3, "17 bits should allocate 3 bytes");
    assert_true(phuck_off_mmap_bytes[0] == 0, "reinit should zero the first byte");
    assert_true(phuck_off_mmap_bytes[1] == 0, "reinit should zero the second byte");
    assert_true(phuck_off_mmap_bytes[2] == 0, "reinit should zero the third byte");
}

static void run_shutdown_case(void) {
    phuck_off_mmap_shutdown();
    assert_true(phuck_off_mmap_bytes == NULL, "shutdown should clear the mapped pointer");
    assert_true(access(PHUCK_OFF_MMAP_FILE, F_OK) != 0, "shutdown should remove the backing file");
}

int main(void) {
    backup_existing_file();

    run_invalid_init_case();
    run_create_and_set_case();
    run_reinit_case();
    run_shutdown_case();

    restore_existing_file();

    if (failures) {
        return 1;
    }

    printf("ok\n");
    return 0;
}
