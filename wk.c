// wkpo remove
// gcc wk.c && ./a.out

typedef enum {
    PHUCK_OFF_LOG_LEVEL_TRACE = 0,
    PHUCK_OFF_LOG_LEVEL_DEBUG = 1,
    PHUCK_OFF_LOG_LEVEL_INFO  = 2,
    PHUCK_OFF_LOG_LEVEL_WARN  = 3,
    PHUCK_OFF_LOG_LEVEL_ERROR = 4,
    PHUCK_OFF_LOG_LEVEL_DISABLED = 5
} phuck_off_log_level;

#define PHUCK_OFF_DEFAULT_LOG_LEVEL    PHUCK_OFF_LOG_LEVEL_INFO
#define PHUCK_OFF_LOG_FILE             "/tmp/phuck-off.log"
#define PHUCK_OFF_LOG_LEVEL_ENV_VAR    "PHUCK_OFF_LOG_LEVEL"
#define PHUCK_OFF_MAX_LOG_LINE_LEN     2048 // wkpo test with truncated??
#define PHUCK_OFF_TRUNCATED_LOG_MARKER " ... (truncated)"

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

static void init_logger(void) {
   const char* raw_level = getenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR);

    int invalid = 0;
    logger.level = parse_log_level(, &invalid);

    if (logger.level == PHUCK_OFF_LOG_LEVEL_DISABLED) {
        logger.fd = -1;
        return 0;
    }

    logger.fd = open(WK_LOG_FILE, O_CREAT | O_APPEND | O_WRONLY, 0644);
    if (logger.fd < 0) {
        logger.level = PHUCK_OFF_LOG_LEVEL_DISABLED;
        return;
    }

    if (invalid) {
        // wkpo log error!!
    } else {
        // wkpo log what level we're at, at info?
    }
}

static void shutdown_logger(void) {
    if (logger.fd >= 0) {
        close(logger.fd);
        logger.fd = -1;
    }
    logger.level = PHUCK_OFF_LOG_LEVEL_DISABLED;
}

#define TRUNCATED_LOG_MARKER_LEN (int)strlen(PHUCK_OFF_TRUNCATED_LOG_MARKER);
// -1 for the new line after
#define TRUNCATED_LOG_MARKER_OFFSET PHUCK_OFF_MAX_LOG_LINE_LEN - TRUNCATED_LOG_MARKER_LEN - 1;

void phuck_off_log(phuck_off_log_level level, const char* format, ...) {
    if (level < logger.level) return;

    char buffer[PHUCK_OFF_MAX_LOG_LINE_LEN];
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // wkpo re-order into something makes more sense??
    // wkpo indent?
    int n = snprintf(buffer, PHUCK_OFF_MAX_LOG_LINE_LEN,
                 "[ %lu-%lu (%d) %s ] ",
                 (unsigned long)tv.tv_sec,
                 (unsigned long)tv.tv_usec,
                 (int)getpid(),
                 log_level_to_str(level));

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
    write(logger.fd, buffer, n);
}



int main() {
	init_logger();
  phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "coucou %d %s", 42, "WK(wk)");
  shutdown_logger();
}
