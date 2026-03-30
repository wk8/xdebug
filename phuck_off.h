#ifndef __HAVE_PHUCK_OFF_H__
#define __HAVE_PHUCK_OFF_H__

#include "xdebug_private.h"

typedef enum {
    PHUCK_OFF_LOG_LEVEL_TRACE    = 0,
    PHUCK_OFF_LOG_LEVEL_DEBUG    = 1,
    PHUCK_OFF_LOG_LEVEL_INFO     = 2,
    PHUCK_OFF_LOG_LEVEL_WARN     = 3,
    PHUCK_OFF_LOG_LEVEL_ERROR    = 4,
    PHUCK_OFF_LOG_LEVEL_DISABLED = 5
} phuck_off_log_level;

// init/teardown for MINIT/MSHUTDOWN
void phuck_off_init(void);
void phuck_off_shutdown(void);

void phuck_off_log(phuck_off_log_level level, const char* format, ...);

void phuck_off_handle_stack_function(xdebug_func f);

#endif
