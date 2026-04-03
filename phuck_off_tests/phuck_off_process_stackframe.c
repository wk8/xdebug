#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define PHUCK_OFF_FUNCS_PATH "/Users/wk/pushpress/xdebug/phuck_off_tests/fixtures/stackframe.txt"

#include "shims.h"
#include "phuck_off.c"

static int failures = 0;
static char* saved_log_level_env = NULL;
static int log_level_env_was_set = 0;
static char* saved_sampling_env = NULL;
static int sampling_env_was_set = 0;
static char* saved_no_cleanup_env = NULL;
static int no_cleanup_env_was_set = 0;
static char* saved_enabled_env = NULL;
static int enabled_env_was_set = 0;
static char backup_template[] = "/tmp/phuck-off.process-stackframe.backup.XXXXXX";
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
    assert_true(copy != NULL, "failed to allocate process stackframe env copy");
    if (!copy) {
        return NULL;
    }

    memcpy(copy, value, len + 1);
    return copy;
}

static void preserve_environment(void) {
    const char* log_level = getenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR);
    const char* sampling = getenv(PHUCK_OFF_SANITY_CHECK_SAMPLING_ENV_VAR);
    const char* no_cleanup = getenv(PHUCK_OFF_NO_CLEANUP_ENV_VAR);
    const char* enabled = getenv(PHUCK_OFF_ENABLED_ENV_VAR);

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

    if (no_cleanup) {
        saved_no_cleanup_env = dup_string(no_cleanup);
        no_cleanup_env_was_set = 1;
    } else {
        saved_no_cleanup_env = NULL;
        no_cleanup_env_was_set = 0;
    }

    if (enabled) {
        saved_enabled_env = dup_string(enabled);
        enabled_env_was_set = 1;
    } else {
        saved_enabled_env = NULL;
        enabled_env_was_set = 0;
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

    if (no_cleanup_env_was_set) {
        setenv(PHUCK_OFF_NO_CLEANUP_ENV_VAR, saved_no_cleanup_env, 1);
    } else {
        unsetenv(PHUCK_OFF_NO_CLEANUP_ENV_VAR);
    }

    if (enabled_env_was_set) {
        setenv(PHUCK_OFF_ENABLED_ENV_VAR, saved_enabled_env, 1);
    } else {
        unsetenv(PHUCK_OFF_ENABLED_ENV_VAR);
    }

    free(saved_log_level_env);
    saved_log_level_env = NULL;
    log_level_env_was_set = 0;

    free(saved_sampling_env);
    saved_sampling_env = NULL;
    sampling_env_was_set = 0;

    free(saved_no_cleanup_env);
    saved_no_cleanup_env = NULL;
    no_cleanup_env_was_set = 0;

    free(saved_enabled_env);
    saved_enabled_env = NULL;
    enabled_env_was_set = 0;
}

static void backup_existing_log(void) {
    int fd;

    if (access(PHUCK_OFF_LOG_FILE, F_OK) != 0) {
        backup_exists = 0;
        return;
    }

    fd = mkstemp(backup_template);
    assert_true(fd >= 0, "failed to create process stackframe backup log path");
    if (fd < 0) {
        return;
    }

    close(fd);
    unlink(backup_template);

    assert_true(rename(PHUCK_OFF_LOG_FILE, backup_template) == 0, "failed to back up process stackframe log");
    if (!failures) {
        backup_exists = 1;
    }
}

static void restore_existing_log(void) {
    phuck_off_shutdown();
    if (unlink(PHUCK_OFF_LOG_FILE) != 0 && errno != ENOENT) {
        assert_true(0, "failed to remove process stackframe log");
    }

    if (backup_exists) {
        assert_true(rename(backup_template, PHUCK_OFF_LOG_FILE) == 0, "failed to restore original process stackframe log");
        backup_exists = 0;
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

    assert_true(fseek(fp, 0, SEEK_END) == 0, "failed to seek process stackframe log");
    length = ftell(fp);
    assert_true(length >= 0, "failed to get process stackframe log length");
    assert_true(fseek(fp, 0, SEEK_SET) == 0, "failed to rewind process stackframe log");
    if (failures) {
        fclose(fp);
        return NULL;
    }

    buffer = (char*) malloc((size_t) length + 1);
    assert_true(buffer != NULL, "failed to allocate process stackframe log buffer");
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    read_bytes = fread(buffer, 1, (size_t) length, fp);
    assert_true(read_bytes == (size_t) length, "failed to read process stackframe log");
    buffer[read_bytes] = '\0';
    fclose(fp);
    return buffer;
}

static off_t file_size(const char* path) {
    struct stat sb;

    if (stat(path, &sb) != 0) {
        return -1;
    }

    return sb.st_size;
}

static void expected_mmap_path(char* buffer, size_t buffer_len) {
    const int written = snprintf(buffer, buffer_len, "/tmp/phuck_off_map_%ld", (long) getpid());

    assert_true(written > 0 && (size_t) written < buffer_len, "failed to build expected mmap path");
}

static zend_execute_data make_frame(zend_function* function, const char* path, int line_no, int type) {
    zend_execute_data zdata;

    memset(&zdata, 0, sizeof(zdata));
    memset(function, 0, sizeof(*function));
    function->type = type;
    function->op_array.filename = path;
    function->op_array.line_start = line_no;
    zdata.function_state.function = function;

    return zdata;
}

static xdebug_hash* line_map_for(const char* path) {
    void* file_entry = NULL;

    assert_true(
        xdebug_hash_find(handler.files, (char*) path, (unsigned int) strlen(path), &file_entry),
        "failed to find line map for stackframe fixture path"
    );

    return (xdebug_hash*) file_entry;
}

static void run_process_stackframe_case(void) {
    char mmap_path[64];
    char* log_content;
    zend_function main_function;
    zend_execute_data main_zdata;
    zend_function missing_function;
    zend_execute_data missing_zdata;
    zend_function internal_function;
    zend_execute_data internal_zdata;
    xdebug_hash* main_line_map;

    setenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR, "info", 1);
    setenv(PHUCK_OFF_SANITY_CHECK_SAMPLING_ENV_VAR, "100", 1);
    unsetenv(PHUCK_OFF_NO_CLEANUP_ENV_VAR);
    setenv(PHUCK_OFF_ENABLED_ENV_VAR, "1", 1);
    XG(phuck_off_tracker_offset) = 3;

    expected_mmap_path(mmap_path, sizeof(mmap_path));
    unlink(mmap_path);

    phuck_off_init();

    assert_true(handler.initialized == 1, "phuck_off_init should initialize handler");
    assert_true(phuck_off_mmap_bytes == NULL, "phuck_off_init should not initialize mmap");
    assert_true(access(mmap_path, F_OK) != 0 && errno == ENOENT, "phuck_off_init should not create mmap file");

    phuck_off_request_init();

    assert_true(phuck_off_mmap_bytes != NULL, "phuck_off_request_init should initialize mmap");
    assert_true(access(mmap_path, F_OK) == 0, "phuck_off_request_init should create mmap file");
    assert_true(file_size(mmap_path) == 1, "2 parsed functions should allocate a 1-byte mmap file");

    log_content = read_log_file();
    assert_contains(log_content, "Initialized phuck-off mmap path=\"", "phuck_off_request_init should log mmap path");
    assert_contains(log_content, mmap_path, "phuck_off_request_init log should contain exact mmap path");
    free(log_content);

    main_zdata = make_frame(&main_function, "/tmp/phuck-off-root/app/main.php", 10, ZEND_USER_FUNCTION);
    phuck_off_process_stackframe(&main_zdata);
    assert_true((intptr_t) main_function.op_array.reserved[3] == 1, "main.php:10 should cache function id 1");
    assert_true(phuck_off_mmap_bytes[0] == 0x01, "function id 1 should set mmap bit 0");

    main_line_map = line_map_for("/tmp/phuck-off-root/app/main.php");
    assert_true(
        xdebug_hash_index_update(main_line_map, 10, (void*) (uintptr_t) 2),
        "failed to update stackframe fixture line map"
    );

    phuck_off_process_stackframe(&main_zdata);
    assert_true((intptr_t) main_function.op_array.reserved[3] == 2, "sampled cache mismatch should update cached function id");
    assert_true((phuck_off_mmap_bytes[0] & 0x03u) == 0x03u, "sampled cache mismatch should set mmap bit 1");

    missing_zdata = make_frame(&missing_function, "/tmp/phuck-off-root/app/missing.php", 77, ZEND_USER_FUNCTION);
    phuck_off_process_stackframe(&missing_zdata);
    assert_true((intptr_t) missing_function.op_array.reserved[3] == -1, "missing function should cache -1");

    internal_zdata = make_frame(&internal_function, "/tmp/phuck-off-root/app/other.php", 20, 0);
    phuck_off_process_stackframe(&internal_zdata);
    assert_true(internal_function.op_array.reserved[3] == NULL, "internal function should not touch cache");

    phuck_off_shutdown();

    assert_true(phuck_off_mmap_bytes == NULL, "phuck_off_shutdown should unmap bytes");
    assert_true(access(mmap_path, F_OK) != 0 && errno == ENOENT, "phuck_off_shutdown should remove mmap file");

    log_content = read_log_file();
    assert_contains(log_content, "Cache error!! function /tmp/phuck-off-root/app/main.php:10 is ID 2, but cached is 1", "sampled mismatch should log a cache error");
    free(log_content);

}

static void run_request_init_fork_case(void) {
    int pipe_fds[2];
    pid_t child_pid;
    char child_mmap_path[64];
    char parent_mmap_path[64];
    int child_success = 0;
    int child_status = 0;
    ssize_t child_read;
    const int parent_written = snprintf(parent_mmap_path, sizeof(parent_mmap_path), "/tmp/phuck_off_map_%ld", (long) getpid());

    assert_true(parent_written > 0 && (size_t) parent_written < sizeof(parent_mmap_path), "failed to build parent mmap path");
    if (unlink(parent_mmap_path) != 0 && errno != ENOENT) {
        assert_true(0, "failed to remove stale parent mmap path");
    }

    setenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR, "info", 1);
    unsetenv(PHUCK_OFF_SANITY_CHECK_SAMPLING_ENV_VAR);
    setenv(PHUCK_OFF_NO_CLEANUP_ENV_VAR, "1", 1);
    setenv(PHUCK_OFF_ENABLED_ENV_VAR, "1", 1);
    phuck_off_init();
    assert_true(phuck_off_mmap_bytes == NULL, "fork case should start without mmap after phuck_off_init");

    assert_true(pipe(pipe_fds) == 0, "failed to create fork case pipe");
    if (failures) {
        phuck_off_shutdown();
        return;
    }

    child_pid = fork();
    assert_true(child_pid >= 0, "failed to fork for per-worker mmap case");
    if (child_pid < 0) {
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        phuck_off_shutdown();
        return;
    }

    if (child_pid == 0) {
        const int written = snprintf(child_mmap_path, sizeof(child_mmap_path), "/tmp/phuck_off_map_%ld", (long) getpid());

        close(pipe_fds[0]);
        if (written > 0 && (size_t) written < sizeof(child_mmap_path)) {
            phuck_off_request_init();
            child_success = phuck_off_mmap_bytes != NULL && access(child_mmap_path, F_OK) == 0;
            if (write(pipe_fds[1], &child_success, sizeof(child_success)) != (ssize_t) sizeof(child_success)) {
                child_success = 0;
            }
        }
        close(pipe_fds[1]);
        phuck_off_shutdown();
        _exit(child_success ? 0 : 1);
    }

    close(pipe_fds[1]);
    child_read = read(pipe_fds[0], &child_success, sizeof(child_success));
    close(pipe_fds[0]);
    assert_true(child_read == (ssize_t) sizeof(child_success), "failed to read child mmap result");
    assert_true(waitpid(child_pid, &child_status, 0) == child_pid, "failed to wait for child mmap process");
    assert_true(WIFEXITED(child_status) && WEXITSTATUS(child_status) == 0, "child should initialize its own mmap");
    assert_true(child_success == 1, "child should report successful worker-local mmap init");

    phuck_off_request_init();
    assert_true(phuck_off_mmap_bytes != NULL, "parent should initialize its own mmap after fork");
    assert_true(access(parent_mmap_path, F_OK) == 0, "parent should create its own mmap path after fork");
    assert_true(snprintf(child_mmap_path, sizeof(child_mmap_path), "/tmp/phuck_off_map_%ld", (long) child_pid) > 0, "failed to rebuild child mmap path");
    assert_true(strcmp(parent_mmap_path, child_mmap_path) != 0, "parent and child should use different mmap paths");
    assert_true(access(child_mmap_path, F_OK) == 0, "child mmap file should be preserved for verification");

    phuck_off_shutdown();
    assert_true(unlink(child_mmap_path) == 0, "failed to remove child mmap file after verification");
}

static void run_disabled_case(void) {
    char mmap_path[64];
    zend_function function;
    zend_execute_data zdata;

    setenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR, "trace", 1);
    setenv(PHUCK_OFF_SANITY_CHECK_SAMPLING_ENV_VAR, "100", 1);
    unsetenv(PHUCK_OFF_NO_CLEANUP_ENV_VAR);
    unsetenv(PHUCK_OFF_ENABLED_ENV_VAR);
    XG(phuck_off_tracker_offset) = 3;

    expected_mmap_path(mmap_path, sizeof(mmap_path));
    if (unlink(mmap_path) != 0 && errno != ENOENT) {
        assert_true(0, "failed to remove stale disabled-case mmap path");
    }
    if (unlink(PHUCK_OFF_LOG_FILE) != 0 && errno != ENOENT) {
        assert_true(0, "failed to remove stale disabled-case log");
    }

    zdata = make_frame(&function, "/tmp/phuck-off-root/app/main.php", 10, ZEND_USER_FUNCTION);
    phuck_off_init();
    assert_true(handler.initialized == 0, "phuck_off_init should skip handler init when disabled");
    assert_true(phuck_off_mmap_bytes == NULL, "disabled phuck_off_init should not initialize mmap");
    assert_true(access(PHUCK_OFF_LOG_FILE, F_OK) != 0, "disabled phuck_off_init should not create a log file");

    phuck_off_request_init();
    assert_true(phuck_off_mmap_bytes == NULL, "disabled phuck_off_request_init should be a no-op");
    assert_true(access(mmap_path, F_OK) != 0 && errno == ENOENT, "disabled phuck_off_request_init should not create mmap file");

    phuck_off_post_request();
    assert_true(access(PHUCK_OFF_LOG_FILE, F_OK) != 0, "disabled phuck_off_post_request should not create a log file");

    phuck_off_process_stackframe(&zdata);
    assert_true(function.op_array.reserved[3] == NULL, "disabled phuck_off_process_stackframe should not touch cache");
    assert_true(access(PHUCK_OFF_LOG_FILE, F_OK) != 0, "disabled phuck_off_process_stackframe should not create a log file");
    assert_true(access(mmap_path, F_OK) != 0, "disabled phuck_off_process_stackframe should not create mmap file");

    phuck_off_shutdown();
    assert_true(access(PHUCK_OFF_LOG_FILE, F_OK) != 0, "disabled phuck_off_shutdown should not create a log file");
}

int main(void) {
    preserve_environment();
    backup_existing_log();

    run_process_stackframe_case();
    run_request_init_fork_case();
    run_disabled_case();

    restore_existing_log();
    restore_environment();

    if (failures) {
        phuck_off_shutdown();
        return 1;
    }

    printf("ok\n");
    return 0;
}
