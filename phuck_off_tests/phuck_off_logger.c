#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shims.h"
#include "phuck_off.h"
#include "phuck_off_logger.h"

static int failures = 0;
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

static void assert_not_contains(const char* haystack, const char* needle, const char* message) {
    assert_true(haystack == NULL || strstr(haystack, needle) == NULL, message);
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
    assert_true(fd >= 0, "failed to create backup path for log file");
    if (fd < 0) {
        return;
    }

    close(fd);
    unlink(backup_template);

    assert_true(rename(PHUCK_OFF_LOG_FILE, backup_template) == 0, "failed to back up existing log file");
    if (failures) {
        return;
    }

    backup_exists = 1;
}

static void restore_existing_log(void) {
    phuck_off_logger_shutdown();
    unlink(PHUCK_OFF_LOG_FILE);

    if (backup_exists) {
        assert_true(rename(backup_template, PHUCK_OFF_LOG_FILE) == 0, "failed to restore original log file");
        backup_exists = 0;
    }
}

static void remove_test_log(void) {
    phuck_off_logger_shutdown();
    if (unlink(PHUCK_OFF_LOG_FILE) != 0 && errno != ENOENT) {
        assert_true(0, "failed to remove test log file");
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

    assert_true(fseek(fp, 0, SEEK_END) == 0, "failed to seek to end of log file");
    length = ftell(fp);
    assert_true(length >= 0, "failed to get log file length");
    assert_true(fseek(fp, 0, SEEK_SET) == 0, "failed to rewind log file");
    if (failures) {
        fclose(fp);
        return NULL;
    }

    buffer = (char*) malloc((size_t) length + 1);
    assert_true(buffer != NULL, "failed to allocate log file buffer");
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    read_bytes = fread(buffer, 1, (size_t) length, fp);
    assert_true(read_bytes == (size_t) length, "failed to read log file");
    buffer[read_bytes] = '\0';
    fclose(fp);
    return buffer;
}

static void run_default_level_case(void) {
    char* content;

    remove_test_log();
    unsetenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR);

    phuck_off_logger_init();
    phuck_off_log(PHUCK_OFF_LOG_LEVEL_TRACE, "trace message from default");
    phuck_off_logger_shutdown();

    content = read_log_file();
    assert_true(content != NULL, "default level should create a log file");
    assert_contains(content, "Logging at level trace", "default logger init message missing");
    assert_contains(content, "[trace]", "trace log prefix missing");
    assert_contains(content, "trace message from default", "default trace message missing");
    free(content);
}

static void run_invalid_level_case(void) {
    char* content;

    remove_test_log();
    setenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR, "bogus-level", 1);

    phuck_off_logger_init();
    phuck_off_log(PHUCK_OFF_LOG_LEVEL_DEBUG, "debug message after invalid level");
    phuck_off_logger_shutdown();

    content = read_log_file();
    assert_true(content != NULL, "invalid level should still create a log file");
    assert_contains(content, "Unknown log level \"bogus-level\", defaulting to trace", "invalid-level warning missing");
    assert_contains(content, "[debug]", "debug log prefix missing after invalid level");
    assert_contains(content, "debug message after invalid level", "debug message missing after invalid level");
    free(content);
}

static void run_warn_filter_case(void) {
    char* content;

    remove_test_log();
    setenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR, "warn", 1);

    phuck_off_logger_init();
    phuck_off_log(PHUCK_OFF_LOG_LEVEL_DEBUG, "debug should be filtered");
    phuck_off_log(PHUCK_OFF_LOG_LEVEL_WARN, "warn should be logged");
    phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "error should be logged");
    phuck_off_logger_shutdown();
    phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "after shutdown should not log");

    content = read_log_file();
    assert_true(content != NULL, "warn level should create a log file");
    assert_not_contains(content, "Logging at level warn", "warn level init info should be filtered");
    assert_not_contains(content, "debug should be filtered", "debug log was not filtered");
    assert_contains(content, "[warn]", "warn log prefix missing");
    assert_contains(content, "warn should be logged", "warn message missing");
    assert_contains(content, "[error]", "error log prefix missing");
    assert_contains(content, "error should be logged", "error message missing");
    assert_not_contains(content, "after shutdown should not log", "logger wrote after shutdown");
    free(content);
}

static void run_append_case(void) {
    char* content;
    char* first;
    char* second;

    remove_test_log();
    setenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR, "error", 1);

    phuck_off_logger_init();
    phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "first append message");
    phuck_off_logger_shutdown();

    phuck_off_logger_init();
    phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "second append message");
    phuck_off_logger_shutdown();

    content = read_log_file();
    assert_true(content != NULL, "append case should create a log file");
    assert_contains(content, "first append message", "first append message missing");
    assert_contains(content, "second append message", "second append message missing");

    first = strstr(content, "first append message");
    second = strstr(content, "second append message");
    assert_true(first != NULL && second != NULL && first < second, "append order is wrong");
    free(content);
}

static void run_truncation_case(void) {
    char* content;
    char* long_message;

    remove_test_log();
    setenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR, "trace", 1);

    long_message = (char*) malloc(PHUCK_OFF_MAX_LOG_LINE_LEN * 3);
    assert_true(long_message != NULL, "failed to allocate long log message");
    if (!long_message) {
        return;
    }

    memset(long_message, 'x', PHUCK_OFF_MAX_LOG_LINE_LEN * 3 - 1);
    long_message[PHUCK_OFF_MAX_LOG_LINE_LEN * 3 - 1] = '\0';

    phuck_off_logger_init();
    phuck_off_log(PHUCK_OFF_LOG_LEVEL_INFO, "%s", long_message);
    phuck_off_logger_shutdown();

    content = read_log_file();
    assert_true(content != NULL, "truncation case should create a log file");
    assert_contains(content, PHUCK_OFF_TRUNCATED_LOG_MARKER, "truncation marker missing");
    assert_true(content[strlen(content) - 1] == '\n', "truncated log should end with a newline");

    free(content);
    free(long_message);
}

static void run_disabled_case(void) {
    remove_test_log();
    setenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR, "disabled", 1);

    phuck_off_logger_init();
    phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "disabled logger should not write");
    phuck_off_logger_shutdown();

    assert_true(access(PHUCK_OFF_LOG_FILE, F_OK) != 0, "disabled logger should not create a log file");
}

int main(void) {
    preserve_environment();
    backup_existing_log();

    if (!failures) {
        run_default_level_case();
        run_invalid_level_case();
        run_warn_filter_case();
        run_append_case();
        run_truncation_case();
        run_disabled_case();
    }

    restore_existing_log();
    restore_environment();

    if (failures) {
        return 1;
    }

    printf("ok\n");
    return 0;
}
