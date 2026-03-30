#ifndef __HAVE_PHUCK_OFF_H__
#define __HAVE_PHUCK_OFF_H__

#ifdef PHUCK_OFF_STANDALONE_TEST
typedef struct _xdebug_func {
	char *class;
	char *function;
	int   type;
	int   internal;
} xdebug_func;
#else
#include "xdebug_private.h"
#endif

// see https://chatgpt.com/c/6940b9e0-a0dc-8330-bdd0-2424f2dd0d85
// wkpo remove ^

typedef enum {
    PHUCK_OFF_LOG_LEVEL_TRACE    = 0,
    PHUCK_OFF_LOG_LEVEL_DEBUG    = 1,
    PHUCK_OFF_LOG_LEVEL_INFO     = 2,
    PHUCK_OFF_LOG_LEVEL_WARN     = 3,
    PHUCK_OFF_LOG_LEVEL_ERROR    = 4,
    PHUCK_OFF_LOG_LEVEL_DISABLED = 5
} phuck_off_log_level;

#define PHUCK_OFF_DEFAULT_LOG_LEVEL    PHUCK_OFF_LOG_LEVEL_TRACE // PHUCK_OFF_LOG_LEVEL_INFO wkpo!!
#define PHUCK_OFF_LOG_FILE             "/tmp/phuck-off.log"
#define PHUCK_OFF_LOG_LEVEL_ENV_VAR    "PHUCK_OFF_LOG_LEVEL"
#define PHUCK_OFF_MAX_LOG_LINE_LEN     2048 // wkpo test with truncated??
#define PHUCK_OFF_TRUNCATED_LOG_MARKER " ... (truncated)"

#ifndef PHUCK_OFF_FUNCS_PATH
#define PHUCK_OFF_FUNCS_PATH "/etc/funcs.txt"
#endif
#define PHUCK_OFF_FUNCS_FILE PHUCK_OFF_FUNCS_PATH
// tune to keep the ratio of funcs to hash size between 0.5 and 0.7
#define PHUCK_OFF_FUNCS_HASH_SIZE 4096
#define PHUCK_OFF_MAX_FUNC_NAME_LEN 128

// init/teardown for MINIT/MSHUTDOWN
void phuck_off_init(void);
void phuck_off_shutdown(void);

void phuck_off_log(phuck_off_log_level level, const char* format, ...);

void phuck_off_handle_stack_function(xdebug_func f);

#endif
