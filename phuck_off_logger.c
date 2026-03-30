#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "phuck_off_logger.h"

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

void phuck_off_logger_init(void) {
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

void phuck_off_logger_shutdown(void) {
    if (logger.fd >= 0) {
        close(logger.fd);
        logger.fd = -1;
    }
    logger.level = PHUCK_OFF_LOG_LEVEL_DISABLED;
}
