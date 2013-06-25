/*
	ZomPHP socket
	Author: Jean Rouge (jer329@cornell.edu)
 */

#include <string.h>

#include "SAPI.h"

#include "xdebug_socket.h"

#define CLI "cli"
#define SOCKET_PATH_PREFIX "/tmp/socketname"


// returns 0 iff we don't want to use ZomPHP for this SAPI
// current policy : allow ZomPHP for any other SAPI than CLI
int is_relevant_sapi(int ht)
{
	if (zend_parse_parameters_none() == FAILURE || !sapi_module.name) {
		// don't know what the current SAPI is, better not do anything
		return 0;
	}
	return strcmp(sapi_module.name, CLI);
}

// returns the file descriptor for the current thread's socket, after having connected
// if the result is <= 0, means that some kind of error occurred
int get_socket(int ht)
{
	if (!is_relevant_sapi(ht)) {
		return 0;
	}
	
}
