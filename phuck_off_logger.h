#ifndef __HAVE_PHUCK_OFF_LOGGER_H__
#define __HAVE_PHUCK_OFF_LOGGER_H__

#include "phuck_off.h"

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

void phuck_off_logger_init(void);
void phuck_off_logger_shutdown(void);

#endif
