/*
	ZomPHP socket
	Author: Jean Rouge (jer329@cornell.edu)
 */

#ifndef __HAVE_XDEBUG_SOCKET_H__
#define __HAVE_XDEBUG_SOCKET_H__

#define MAX_ERROR_MSG_LENGTH 1000

typedef struct xdebug_socket_error {
	char  has_error;
	char* error_msg;
} xdebug_socket_error;

xdebug_socket_error* new_socket_error();
void free_socket_error(xdebug_socket_error* e);

int get_socket(int ht, xdebug_socket_error* error);

#endif
