/*
	ZomPHP socket
	Author: Jean Rouge (jer329@cornell.edu)
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#include "SAPI.h"

#include "xdebug_socket.h"
#include "usefulstuff.h"

#define CLI "cli"
#define SOCKET_PATH_PREFIX "/tmp/socketname" // TODO wkpo

#define SOCKET_NOT_CREATED -1
#define COULD_NOT_CONNECT_TO_SOCKET -2

xdebug_socket_error* new_socket_error()
{
	xdebug_socket_error* result;
	char* error_msg;

	result = (xdebug_socket_error*) malloc(sizeof(xdebug_socket_error));
	error_msg = (char*) malloc(sizeof(char) * MAX_ERROR_MSG_LENGTH);

	if (!result || !error_msg) {
		return NULL;
	}

	result->error_msg = error_msg;
	result->has_error = 0;
	return result;
}

void free_socket_error(xdebug_socket_error* e)
{
	if (e) {
		free(e->error_msg);
		free(e);
	}
}

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

// returns the current process's socket name
// TODO wkpo
char* get_socket_name()
{
	return SOCKET_PATH_PREFIX;
}

// asks ZomPHP's deamon to create a socket for this process
void ask_for_socket_creation(xdebug_socket_error* error)
{
	// TODO wkpo
}

void report_error(const char* msg, const char* socket_name, xdebug_socket_error* error, const int silent_errors)
{
	if (silent_errors & errno || !error) {
		// muted errors, or no struct to report it
		return;
	}

	xdebug_strcat(error->error_msg, MAX_ERROR_MSG_LENGTH, 4, msg, socket_name, " : ", strerror(errno));
	error->has_error = 1;
}

// tries to connect to a socket, and returns the file descriptor for it
// or < 0 otherwise (SOCKET_NOT_CREATED if couldn't create the socket, COULD_NOT_CONNECT_TO_SOCKET if coulnd't connect)
int connect_to_socket(const char* socket_name, const int silent_errors, xdebug_socket_error* error)
{
	int socket_fd;
	struct sockaddr_un serv_addr;

	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		report_error("Could not create socket ", socket_name, error, silent_errors);
		return SOCKET_NOT_CREATED;
	}

	serv_addr.sun_family = AF_UNIX;
	memcpy(serv_addr.sun_path, socket_name, sizeof(char) * (strlen(socket_name) + 1)); // +1 for the terminating char

	if (connect(socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		report_error("Could not connect to socket ", socket_name, error, silent_errors);
		return COULD_NOT_CONNECT_TO_SOCKET;
	}

	return socket_fd;
}

// returns the file descriptor for the current process's socket, after having connected
// will ask for the socket's creation if it wasn't found // TODO wkpo
// if the result is < 0, means that some kind of error occurred
int get_socket(int ht, xdebug_socket_error* error)
{
	int socket_fd;

	if (!is_relevant_sapi(ht)) {
		return -1;
	}

	socket_fd = connect_to_socket(get_socket_name(), ENOENT, error);
	if (socket_fd < 0) return 124 + errno;

	if (socket_fd == COULD_NOT_CONNECT_TO_SOCKET && errno == ENOENT) {
		// socket not found, let's ask ZomPHP to create it
		ask_for_socket_creation(error);
	}

	return socket_fd;
}
