/*
 * wkpo questions:
 *  - do xdebug func pointers remain the same? can i use that for caching IDs?
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "xdebug_hash.h"

#include "phuck_off.h"

/******************
 * LOGGER SECTION *
 ******************/

typedef struct phuck_off_logger {
    phuck_off_log_level level;
    int fd;
} phuck_off_logger;

static phuck_off_logger logger = { PHUCK_OFF_LOG_LEVEL_DISABLED, -1 };

static phuck_off_log_level parse_log_level(const char* s, int* invalid) {
    if (!s) return PHUCK_OFF_DEFAULT_LOG_LEVEL;

    if (strcmp(s, "trace") == 0) return PHUCK_OFF_LOG_LEVEL_TRACE;
    if (strcmp(s, "debug") == 0) return PHUCK_OFF_LOG_LEVEL_DEBUG;
    if (strcmp(s, "info") == 0)  return PHUCK_OFF_LOG_LEVEL_INFO;
    if (strcmp(s, "warn") == 0)  return PHUCK_OFF_LOG_LEVEL_WARN;
    if (strcmp(s, "error") == 0) return PHUCK_OFF_LOG_LEVEL_ERROR;
    if (strcmp(s, "disabled") == 0) return PHUCK_OFF_LOG_LEVEL_DISABLED;

    *invalid = 1;
    return PHUCK_OFF_DEFAULT_LOG_LEVEL;
}

static const char* log_level_to_str(phuck_off_log_level level) {
    switch (level) {
        case PHUCK_OFF_LOG_LEVEL_TRACE: return "trace";
        case PHUCK_OFF_LOG_LEVEL_DEBUG: return "debug";
        case PHUCK_OFF_LOG_LEVEL_INFO: return "info";
        case PHUCK_OFF_LOG_LEVEL_WARN: return "warn";
        case PHUCK_OFF_LOG_LEVEL_ERROR: return "error";
        default: return "disabled";
    }
}

static inline void write_best_effort(int fd, const char* buffer, size_t len) {
    while (len > 0) {
        ssize_t rc = write(fd, buffer, len);
        if (rc > 0) {
            buffer += rc;
            len -= (size_t)rc;
            continue;
        }
        if (rc < 0 && errno == EINTR) {
            continue;
        }
        // any other error we just stop
        break;
    }
}

#define TRUNCATED_LOG_MARKER_LEN (int)strlen(PHUCK_OFF_TRUNCATED_LOG_MARKER)
// -1 for the new line after
#define TRUNCATED_LOG_MARKER_OFFSET PHUCK_OFF_MAX_LOG_LINE_LEN - TRUNCATED_LOG_MARKER_LEN - 1

void phuck_off_log(phuck_off_log_level level, const char* format, ...) {
    if (level < logger.level) return;

    char buffer[PHUCK_OFF_MAX_LOG_LINE_LEN];
    struct timeval tv;
    gettimeofday(&tv, NULL);

    int n = snprintf(buffer, PHUCK_OFF_MAX_LOG_LINE_LEN,
                     "[%s] (%lu-%lu) - %d: ",
                     log_level_to_str(level),
                     (unsigned long)tv.tv_sec,
                     (unsigned long)tv.tv_usec,
                     (int)getpid());

    if (n < 0) {
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "snprintf failed: %s", strerror(errno));
        return;
    }

    va_list args;
    va_start(args, format);
    const int m = vsnprintf(buffer + n, PHUCK_OFF_MAX_LOG_LINE_LEN - n, format, args);
    va_end(args);

    if (m < 0) {
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "vsnprintf failed: %s", strerror(errno));
        return;
    }

    n += m;

    if (n >= PHUCK_OFF_MAX_LOG_LINE_LEN) {
        // it's been truncated
        n = PHUCK_OFF_MAX_LOG_LINE_LEN - 1;
        memcpy(buffer + TRUNCATED_LOG_MARKER_OFFSET, PHUCK_OFF_TRUNCATED_LOG_MARKER, TRUNCATED_LOG_MARKER_LEN);
    }

    buffer[n++] = '\n';
    write_best_effort(logger.fd, buffer, (size_t)n);
}

static void init_logger(void) {
   const char* raw_level = getenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR);

    int invalid = 0;
    logger.level = parse_log_level(raw_level, &invalid);

    if (logger.level == PHUCK_OFF_LOG_LEVEL_DISABLED) {
        logger.fd = -1;
        return;
    }

    logger.fd = open(PHUCK_OFF_LOG_FILE, O_CREAT | O_APPEND | O_WRONLY, 0644);
    if (logger.fd < 0) {
        logger.level = PHUCK_OFF_LOG_LEVEL_DISABLED;
        return;
    }

    if (invalid) {
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_WARN, "Unknown log level \"%s\", defaulting to %s", raw_level, log_level_to_str(logger.level));
    } else {
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_INFO, "Logging at level %s", log_level_to_str(logger.level));
    }
}

static void shutdown_logger(void) {
    if (logger.fd >= 0) {
        close(logger.fd);
        logger.fd = -1;
    }
    logger.level = PHUCK_OFF_LOG_LEVEL_DISABLED;
}

/*************************
 * END OF LOGGER SECTION *
 *************************/

/****************
 * MAIN SECTION *
 ****************/

typedef struct phuck_off {
    int initialized;
    xdebug_hash* funcs;
} phuck_off;

static phuck_off handler = { 0, NULL };

static void strip_newline(char* s, size_t* len) {
    while (*len > 0) {
        char c = s[*len - 1];
        if (c != '\n' && c != '\r') {
            return;
        }

        s[--(*len)] = '\0';
    }
}

static xdebug_hash* build_funcs_hash(void) {
    phuck_off_log(PHUCK_OFF_LOG_LEVEL_INFO, "reading funcs from %s", PHUCK_OFF_FUNCS_FILE);

    FILE* fp = fopen("/etc/funcs.txt", "rb");
    if (!fp) {
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "failed to read funcs from %s", PHUCK_OFF_FUNCS_FILE);
        return NULL;
    }

    xdebug_hash* h = xdebug_hash_alloc(PHUCK_OFF_FUNCS_HASH_SIZE, NULL);
    if (!h) {
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "unable to build hash map");
        fclose(fp);
        return NULL;
    }

    char* buffer = NULL;
    size_t cap = 0;
    ssize_t nread;
    uint32_t line_no = 1;

    while ((nread = getline(&buffer, &cap, fp)) != -1) {
        size_t len = (size_t)nread;
        strip_newline(buffer, &len);

        if (len != 0) {
            phuck_off_log(PHUCK_OFF_LOG_LEVEL_TRACE, "func at line %d: %s", line_no, buffer);

            // wkpo aqui would make sense to check that < PHUCK_OFF_MAX_FUNC_NAME_LEN; then we can remove the error log below!!

            xdebug_hash_add(h, buffer, (int)len, (void*)(uintptr_t)line_no);
        }

        line_no++;
    }
    free(buffer);

    fclose(fp);
    return h;
}

static void init_handler(void) {
    handler.funcs = build_funcs_hash();

    handler.initialized = handler.funcs != NULL;
}

static void shutdown_handler(void) {
    if (handler.funcs != NULL) {
        xdebug_hash_destroy(handler.funcs);
        handler.funcs = NULL;
    }
    handler.initialized = 0;
}

static inline const int normalize_func_name(xdebug_func f, char buffer[PHUCK_OFF_MAX_FUNC_NAME_LEN]) {
    int n = 0;
    switch (f.type) {
        case XFUNC_NORMAL:
            n = snprintf(buffer, PHUCK_OFF_MAX_FUNC_NAME_LEN, "%s", f.function);
            break;

        case XFUNC_STATIC_MEMBER:
        case XFUNC_MEMBER:
            const char* cls;
            const char* fn;
            int log_warning = 0;

            if (f.class) {
                cls = f.class;
            } else {
                cls = "?";
                log_warning = 1;
            }
            if (f.function) {
                fn = f.function;
            } else {
                fn = "?";
                log_warning = 1;
            }

            n = snprintf(buffer, PHUCK_OFF_MAX_FUNC_NAME_LEN, "%s::%s", cls, fn);

            if (log_warning) {
                phuck_off_log(PHUCK_OFF_LOG_LEVEL_WARN, "function \"%s\" missing class or name", buffer);
            }

            break;
    }

    if (n >= PHUCK_OFF_MAX_FUNC_NAME_LEN) {
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "normalized function name \"%s\" too long (total length %d)", buffer, n);
        n = 0;
    }

    buffer[n] = '\0';

    if (n) {
        for (char* p = buffer; *p; p++) {
            *p = (char) tolower((unsigned char)*p);
        }
    }

    return n;
}

void phuck_off_handle_stack_function(xdebug_func f) {
    if (!handler.initialized) return;

    char name[PHUCK_OFF_MAX_FUNC_NAME_LEN];
    const int name_len = normalize_func_name(f, name);

    if (name_len == 0) return;

    void *val = NULL;
    int line_no = -1;
    if (xdebug_hash_find(handler.funcs, name, (unsigned int) name_len, &val)) {
        line_no = (int)((uintptr_t)val);
    }

    phuck_off_log(PHUCK_OFF_LOG_LEVEL_DEBUG, "function \"%s\" => line %d", name, line_no);
}

/***********************
 * END OF MAIN SECTION *
 ***********************/

/*************************
 * INIT/SHUTDOWN SECTION *
 *************************/

void phuck_off_init(void) {
    init_logger();
    init_handler();
}

void phuck_off_shutdown(void) {
    shutdown_handler();
    shutdown_logger();
}

/********************************
 * END OF INIT/SHUTDOWN SECTION *
 ********************************/
