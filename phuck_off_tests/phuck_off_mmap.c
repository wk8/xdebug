#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "phuck_off_logger.h"
#include "phuck_off_mmap.h"

static int failures = 0;
static char test_path[] = "/tmp/phuck-off.mmap.test.XXXXXX";
static char* saved_env = NULL;
static int env_was_set = 0;
static char backup_template[] = "/tmp/phuck-off.log.backup.XXXXXX";
static int backup_exists = 0;

static void assert_true(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        failures = 1;
    }
}

static void assert_contains(const char* haystack, const char* needle, const char* message) {
    assert_true(haystack != NULL && strstr(haystack, needle) != NULL, message);
}

static char* dup_string(const char* value) {
    size_t len;
    char* copy;

    if (!value) {
        return NULL;
    }

    len = strlen(value);
    copy = (char*) malloc(len + 1);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, value, len + 1);
    return copy;
}

static void preserve_environment(void) {
    const char* value = getenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR);

    if (value) {
        saved_env = dup_string(value);
        env_was_set = 1;
    } else {
        saved_env = NULL;
        env_was_set = 0;
    }
}

static void restore_environment(void) {
    if (env_was_set) {
        setenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR, saved_env, 1);
    } else {
        unsetenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR);
    }

    free(saved_env);
    saved_env = NULL;
    env_was_set = 0;
}

static void backup_existing_log(void) {
    int fd;

    if (access(PHUCK_OFF_LOG_FILE, F_OK) != 0) {
        backup_exists = 0;
        return;
    }

    fd = mkstemp(backup_template);
    assert_true(fd >= 0, "failed to create backup path for mmap log file");
    if (fd < 0) {
        return;
    }

    close(fd);
    unlink(backup_template);

    assert_true(rename(PHUCK_OFF_LOG_FILE, backup_template) == 0, "failed to back up existing mmap log file");
    if (failures) {
        return;
    }

    backup_exists = 1;
}

static void restore_existing_log(void) {
    phuck_off_logger_shutdown();
    unlink(PHUCK_OFF_LOG_FILE);

    if (backup_exists) {
        assert_true(rename(backup_template, PHUCK_OFF_LOG_FILE) == 0, "failed to restore original mmap log file");
        backup_exists = 0;
    }
}

static void remove_test_log(void) {
    phuck_off_logger_shutdown();
    if (unlink(PHUCK_OFF_LOG_FILE) != 0 && errno != ENOENT) {
        assert_true(0, "failed to remove mmap test log file");
    }
}

static char* read_log_file(void) {
    FILE* fp;
    long length;
    size_t read_bytes;
    char* buffer;

    fp = fopen(PHUCK_OFF_LOG_FILE, "rb");
    if (!fp) {
        return NULL;
    }

    assert_true(fseek(fp, 0, SEEK_END) == 0, "failed to seek to end of mmap log file");
    length = ftell(fp);
    assert_true(length >= 0, "failed to get mmap log file length");
    assert_true(fseek(fp, 0, SEEK_SET) == 0, "failed to rewind mmap log file");
    if (failures) {
        fclose(fp);
        return NULL;
    }

    buffer = (char*) malloc((size_t) length + 1);
    assert_true(buffer != NULL, "failed to allocate mmap log buffer");
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    read_bytes = fread(buffer, 1, (size_t) length, fp);
    assert_true(read_bytes == (size_t) length, "failed to read mmap log file");
    buffer[read_bytes] = '\0';
    fclose(fp);
    return buffer;
}

static int count_occurrences(const char* haystack, const char* needle) {
    int count = 0;
    const char* cursor = haystack;
    size_t needle_len = strlen(needle);

    if (!haystack || needle_len == 0) {
        return 0;
    }

    while ((cursor = strstr(cursor, needle)) != NULL) {
        count++;
        cursor += needle_len;
    }

    return count;
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

static void run_post_request_case(void) {
    unsigned char file_bytes[2];
    char* log_content;

    remove_test_file();
    remove_test_log();
    setenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR, "trace", 1);
    phuck_off_logger_init();

    assert_true(phuck_off_mmap_init(test_path, 10), "init(10) should succeed for post_request");
    phuck_off_mmap_set(0);
    phuck_off_mmap_set(4);
    phuck_off_mmap_set(9);

    phuck_off_mmap_post_request();
    log_content = read_log_file();
    assert_contains(log_content, "syncing=no reason=throttled", "post_request should trace throttled skip");
    assert_true(count_occurrences(log_content, "syncing=yes") == 0, "post_request should not sync on first immediate call");
    free(log_content);

    sleep(4);
    phuck_off_mmap_post_request();
    log_content = read_log_file();
    assert_true(count_occurrences(log_content, "syncing=yes") == 1, "post_request should sync once after 3 seconds");
    free(log_content);
    read_file_bytes(file_bytes, sizeof(file_bytes));
    assert_true(file_bytes[0] == 0x11, "post_request first flush first byte mismatch");
    assert_true(file_bytes[1] == 0x02, "post_request first flush second byte mismatch");

    phuck_off_mmap_set(1);
    phuck_off_mmap_post_request();
    log_content = read_log_file();
    assert_true(count_occurrences(log_content, "syncing=no reason=throttled") == 2, "post_request should throttle a second immediate flush");
    free(log_content);

    sleep(4);
    phuck_off_mmap_post_request();
    log_content = read_log_file();
    assert_true(count_occurrences(log_content, "syncing=yes") == 2, "post_request should sync again after another 3 seconds");
    free(log_content);
    read_file_bytes(file_bytes, sizeof(file_bytes));
    assert_true(file_bytes[0] == 0x13, "post_request second flush first byte mismatch");
    assert_true(file_bytes[1] == 0x02, "post_request second flush second byte mismatch");

    phuck_off_logger_shutdown();
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

    preserve_environment();
    backup_existing_log();

    fd = mkstemp(test_path);
    assert_true(fd >= 0, "failed to allocate mmap test path");
    if (fd < 0) {
        restore_existing_log();
        restore_environment();
        return 1;
    }

    close(fd);
    unlink(test_path);

    run_invalid_init_case();
    run_create_and_set_case();
    run_post_request_case();
    run_reinit_case();
    run_shutdown_case();

    if (failures) {
        restore_existing_log();
        restore_environment();
        return 1;
    }

    restore_existing_log();
    restore_environment();
    printf("ok\n");
    return 0;
}
