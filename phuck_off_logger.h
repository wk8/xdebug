#ifndef __HAVE_PHUCK_OFF_LOGGER_H__
#define __HAVE_PHUCK_OFF_LOGGER_H__

typedef enum {
    PHUCK_OFF_LOG_LEVEL_TRACE    = 0,
    PHUCK_OFF_LOG_LEVEL_DEBUG    = 1,
    PHUCK_OFF_LOG_LEVEL_INFO     = 2,
    PHUCK_OFF_LOG_LEVEL_WARN     = 3,
    PHUCK_OFF_LOG_LEVEL_ERROR    = 4,
    PHUCK_OFF_LOG_LEVEL_DISABLED = 5
} phuck_off_log_level;

#ifndef PHUCK_OFF_DEFAULT_LOG_LEVEL
#define PHUCK_OFF_DEFAULT_LOG_LEVEL PHUCK_OFF_LOG_LEVEL_TRACE
#endif

#ifndef PHUCK_OFF_LOG_FILE
#define PHUCK_OFF_LOG_FILE "/tmp/phuck-off.log"
#endif

#ifndef PHUCK_OFF_LOG_LEVEL_ENV_VAR
#define PHUCK_OFF_LOG_LEVEL_ENV_VAR "PHUCK_OFF_LOG_LEVEL"
#endif

#ifndef PHUCK_OFF_MAX_LOG_LINE_LEN
#define PHUCK_OFF_MAX_LOG_LINE_LEN 2048
#endif

#ifndef PHUCK_OFF_TRUNCATED_LOG_MARKER
#define PHUCK_OFF_TRUNCATED_LOG_MARKER " ... (truncated)"
#endif

void phuck_off_log(phuck_off_log_level level, const char* format, ...);
void phuck_off_logger_init(void);
void phuck_off_logger_shutdown(void);

#endif
