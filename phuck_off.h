#ifndef __HAVE_PHUCK_OFF_H__
#define __HAVE_PHUCK_OFF_H__

#include "xdebug_private.h"
#include "phuck_off_logger.h"

// init/teardown for MINIT/MSHUTDOWN
void phuck_off_init(void);
void phuck_off_shutdown(void);

void phuck_off_handle_stack_function(xdebug_func f);

#endif
