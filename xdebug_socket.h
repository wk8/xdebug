/*
	ZomPHP socket
	Author: Jean Rouge (jer329@cornell.edu)
 */

#ifndef __HAVE_XDEBUG_SOCKET_H__
#define __HAVE_XDEBUG_SOCKET_H__

#include "usefulstuff.h"

typedef struct xdebug_socket_error {
	char                      has_error;
	xdebug_extensible_string* error_msg;
} xdebug_socket_error;


xdebug_socket_error* new_socket_error();
void free_socket_error(xdebug_socket_error* e);

int get_socket(xdebug_socket_error* error);
size_t write_string_to_socket(int socket_fd, const char* str, xdebug_socket_error* error);

#endif
