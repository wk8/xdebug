/*
 * wkpo questions:
 *  - do xdebug func pointers remain the same? can i use that for caching IDs?
 */

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

static inline void write_best_effort(int fd, const char* buf, size_t len)
{
    while (len > 0) {
        ssize_t rc = write(fd, buf, len);
        if (rc > 0) {
            buf += rc;
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
 *****************/

static void strip_newline(char* s, size_t* len) {
    while (*len > 0) {
        char c = s[*len - 1];
        if (c != '\n' && c != '\r') {
            return;
        }

        s[--(*len)] = '\0';
    }
}

static xdebug_hash* build_funcs_hash(void)
{
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

    char* buf = NULL;
    size_t cap = 0;
    ssize_t nread;
    uint32_t line_no = 0;

    while ((nread = getline(&buf, &cap, fp)) != -1) {
        size_t len = (size_t)nread;
        strip_newline(buf, &len);

        if (len != 0) {
            phuck_off_log(PHUCK_OFF_LOG_LEVEL_TRACE, "func at line %d: %s", line_no, buf);
            xdebug_hash_add(h, buf, (int)len, (void*)(uintptr_t)line_no);
        }

        line_no++;
    }
    free(buf);

    fclose(fp);
    return h;
}

/***********************
 * END OF MAIN SECTION *
 ***********************/

/*************************
 * INIT/SHUTDOWN SECTION *
 *************************/

void phuck_off_init(void) {
    init_logger();

    build_funcs_hash(); // wkpo pas la jeter la....

    // wkpo
}

void phuck_off_shutdown(void) {
    // wkpo

    // wkpo free the map...

    shutdown_logger();
}

/********************************
 * END OF INIT/SHUTDOWN SECTION *
 ********************************/
