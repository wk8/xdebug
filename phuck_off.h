#ifndef __HAVE_PHUCK_OFF_H__
#define __HAVE_PHUCK_OFF_H__

#include "xdebug_private.h"
#include "phuck_off_logger.h"
#include "phuck_off_mmap.h"

#ifndef PHUCK_OFF_ENABLED_ENV_VAR
#define PHUCK_OFF_ENABLED_ENV_VAR "PHUCK_OFF_ENABLED"
#endif

// init/teardown for MINIT/MSHUTDOWN
void phuck_off_init(void);
void phuck_off_shutdown(void);

// meant for RINIT
void phuck_off_request_init(void);

// meant for RSHUTDOWN
void phuck_off_post_request(void);

void phuck_off_process_stackframe(zend_execute_data* zdata);

#endif
