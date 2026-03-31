#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "phuck_off_logger.h"
#include "phuck_off_sanity_check.h"

static int failures = 0;
static char* saved_log_level_env = NULL;
static int log_level_env_was_set = 0;
static char* saved_sampling_env = NULL;
static int sampling_env_was_set = 0;
static char backup_template[] = "/tmp/phuck-off.sanity-check.backup.XXXXXX";
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
    assert_true(copy != NULL, "failed to allocate sanity-check env copy");
    if (!copy) {
        return NULL;
    }

    memcpy(copy, value, len + 1);
    return copy;
}

static void preserve_environment(void) {
    const char* log_level = getenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR);
    const char* sampling = getenv(PHUCK_OFF_SANITY_CHECK_SAMPLING_ENV_VAR);

    if (log_level) {
        saved_log_level_env = dup_string(log_level);
        log_level_env_was_set = 1;
    } else {
        saved_log_level_env = NULL;
        log_level_env_was_set = 0;
    }

    if (sampling) {
        saved_sampling_env = dup_string(sampling);
        sampling_env_was_set = 1;
    } else {
        saved_sampling_env = NULL;
        sampling_env_was_set = 0;
    }
}

static void restore_environment(void) {
    if (log_level_env_was_set) {
        setenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR, saved_log_level_env, 1);
    } else {
        unsetenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR);
    }

    if (sampling_env_was_set) {
        setenv(PHUCK_OFF_SANITY_CHECK_SAMPLING_ENV_VAR, saved_sampling_env, 1);
    } else {
        unsetenv(PHUCK_OFF_SANITY_CHECK_SAMPLING_ENV_VAR);
    }

    free(saved_log_level_env);
    saved_log_level_env = NULL;
    log_level_env_was_set = 0;

    free(saved_sampling_env);
    saved_sampling_env = NULL;
    sampling_env_was_set = 0;
}

static void backup_existing_log(void) {
    int fd;

    if (access(PHUCK_OFF_LOG_FILE, F_OK) != 0) {
        backup_exists = 0;
        return;
    }

    fd = mkstemp(backup_template);
    assert_true(fd >= 0, "failed to create sanity-check backup log path");
    if (fd < 0) {
        return;
    }

    close(fd);
    unlink(backup_template);

    assert_true(rename(PHUCK_OFF_LOG_FILE, backup_template) == 0, "failed to back up sanity-check log");
    if (!failures) {
        backup_exists = 1;
    }
}

static void restore_existing_log(void) {
    phuck_off_logger_shutdown();
    if (unlink(PHUCK_OFF_LOG_FILE) != 0 && errno != ENOENT) {
        assert_true(0, "failed to remove sanity-check log");
    }

    if (backup_exists) {
        assert_true(rename(backup_template, PHUCK_OFF_LOG_FILE) == 0, "failed to restore original sanity-check log");
        backup_exists = 0;
    }
}

static void remove_test_log(void) {
    phuck_off_logger_shutdown();
    if (unlink(PHUCK_OFF_LOG_FILE) != 0 && errno != ENOENT) {
        assert_true(0, "failed to remove sanity-check test log");
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

    assert_true(fseek(fp, 0, SEEK_END) == 0, "failed to seek sanity-check log");
    length = ftell(fp);
    assert_true(length >= 0, "failed to get sanity-check log length");
    assert_true(fseek(fp, 0, SEEK_SET) == 0, "failed to rewind sanity-check log");
    if (failures) {
        fclose(fp);
        return NULL;
    }

    buffer = (char*) malloc((size_t) length + 1);
    assert_true(buffer != NULL, "failed to allocate sanity-check log buffer");
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    read_bytes = fread(buffer, 1, (size_t) length, fp);
    assert_true(read_bytes == (size_t) length, "failed to read sanity-check log");
    buffer[read_bytes] = '\0';
    fclose(fp);
    return buffer;
}

static void run_zero_percent_case(void) {
    int i;

    setenv(PHUCK_OFF_SANITY_CHECK_SAMPLING_ENV_VAR, "0", 1);
    phuck_off_sanity_check_init();

    for (i = 0; i < 128; i++) {
        assert_true(!phuck_off_sanity_check_should_sample(), "0 percent sampling should never sample");
    }
}

static void run_hundred_percent_case(void) {
    int i;

    setenv(PHUCK_OFF_SANITY_CHECK_SAMPLING_ENV_VAR, "100", 1);
    phuck_off_sanity_check_init();

    for (i = 0; i < 128; i++) {
        assert_true(phuck_off_sanity_check_should_sample(), "100 percent sampling should always sample");
    }
}

static void run_invalid_env_case(void) {
    char* log_content;

    remove_test_log();
    setenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR, "trace", 1);
    phuck_off_logger_init();
    setenv(PHUCK_OFF_SANITY_CHECK_SAMPLING_ENV_VAR, "not-a-percent", 1);

    phuck_off_sanity_check_init();

    log_content = read_log_file();
    assert_contains(log_content, "Invalid PHUCK_OFF_SANITY_CHECK_SAMPLING=\"not-a-percent\", defaulting to 5", "invalid sampling env should log a warning");
    free(log_content);

    phuck_off_logger_shutdown();
}

int main(void) {
    preserve_environment();
    backup_existing_log();

    run_zero_percent_case();
    run_hundred_percent_case();
    run_invalid_env_case();

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
