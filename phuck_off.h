#ifndef __HAVE_PHUCK_OFF_H__
#define __HAVE_PHUCK_OFF_H__

#include "xdebug_private.h"
#include "phuck_off_logger.h"
#include "phuck_off_mmap.h"

// init/teardown for MINIT/MSHUTDOWN
void phuck_off_init(void);
void phuck_off_shutdown(void);

// meant for RSHUTDOWN
void phuck_off_post_request(void);

void phuck_off_process_stackframe(zend_execute_data* zdata);

#endif
