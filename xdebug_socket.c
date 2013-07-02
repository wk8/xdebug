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
#include <limits.h>
#include <math.h>

#include "SAPI.h"

#include "xdebug_socket.h"
#include "xdebug_code_coverage.h"

#define SOCKET_NOT_CREATED -1
#define COULD_NOT_CONNECT_TO_SOCKET -2
#define CLI "cli"
#define SOCKET_PATH_PREFIX "/tmp/zomphp_socket"
#define SOCKET_PATH_PREFIX_LENGTH strlen(SOCKET_PATH_PREFIX)
#define MAX_PID UINT_MAX // the max PID possible (not even the current one, we just need a gross bound)
#define MAX_PID_LENGTH ((int) (ceil(log10(MAX_PID)) + 1))
#define IN_SOCKET_PATH "/tmp/zomphp_socket_in"
#define IN_SOCKET_PATH_CLI "/tmp/zomphp_socket_in/cli"
#define OUT_SOCKET_PATH "/tmp/zomphp_socket_out"
#define MAX_WAIT_ON_CLI 250 // max number of milliseconds we're prepared to wait TOTAL in a CLI script to get our socket
#define MAX_TRIES_ON_CLI 5 // max number of times we'll retry on CLI
#define WAIT_PER_TRY_ON_CLI (MAX_WAIT_ON_CLI / MAX_TRIES_ON_CLI)

// fills socket_name with the current process's socket name
void get_socket_name(char* socket_name)
{
	sprintf(socket_name, "%s_%d", SOCKET_PATH_PREFIX, (int) getpid());
}

xdebug_socket_error* new_socket_error()
{
	xdebug_socket_error* result;
	xdebug_extensible_string* error_msg;

	result = (xdebug_socket_error*) malloc(sizeof(xdebug_socket_error));
	error_msg = new_xdebug_extensible_string();

	if (!result || !error_msg) {
		if (result) {
			free(result);
		}
		return NULL;
	}

	result->error_msg = error_msg;
	result->has_error = 0;
	return result;
}

void free_socket_error(xdebug_socket_error* e)
{
	if (e) {
		free_xdebug_extensible_string(e->error_msg);
		free(e);
	}
}

// returns 1 iff we're using the CLI SAPI
int is_cli_sapi()
{
	if (!sapi_module.name) {
		// don't know what the current SAPI is, better not do anything
		return 0;
	}
	return !strcmp(sapi_module.name, CLI);
}

void report_error(const char* msg, const char* socket_name, xdebug_socket_error* error, const int silent_error)
{
	if (errno == silent_error || !error) {
		// muted error, or no struct to report it
		return;
	}

	if(error->error_msg = xdebug_extensible_strcat(error->error_msg, 5, "ZomPHP Error: ", msg, socket_name, " : ", strerror(errno))) {
		error->has_error = 1;
	}

	// reset errno
	errno = 0;
}

// tries to connect to a socket, and returns the file descriptor for it
// or < 0 otherwise (SOCKET_NOT_CREATED if couldn't create the socket, COULD_NOT_CONNECT_TO_SOCKET if coulnd't connect)
int connect_to_socket(const char* socket_name, const int silent_error, xdebug_socket_error* error)
{
	int socket_fd;
	struct sockaddr_un serv_addr;

	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		report_error("Could not create socket ", socket_name, error, silent_error);
		return SOCKET_NOT_CREATED;
	}

	serv_addr.sun_family = AF_UNIX;
	memcpy(serv_addr.sun_path, socket_name, sizeof(char) * (strlen(socket_name) + 1)); // +1 for the terminating char

	if (connect(socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		report_error("Could not connect to socket ", socket_name, error, silent_error);
		return COULD_NOT_CONNECT_TO_SOCKET;
	}

	return socket_fd;
}

// returns < 0 if any error
size_t write_string_to_socket(int socket_fd, const char* str, xdebug_socket_error* error)
{
	size_t result, str_length;
	str_length = strlen(str);

	if (socket_fd < 0 || !str_length) {
		return -1;
	}

	result = write(socket_fd, str, str_length);

	if (result < str_length) {
		report_error("Could not send data to socket ", "", error, 0);
		return -1;
	}

	return result;
}

// tries the current process' PID to that socket
// returns 1 iff succesful, 0 otherwhise
int report_pid_to_socket(const char* socket_name, xdebug_socket_error* error)
{
	int socket_fd, result;
	char pid[MAX_PID_LENGTH + strlen(FUNCTION_DELIMITER) + 1];

	result = 0;

	socket_fd = connect_to_socket(socket_name, 0, NULL);

	if (socket_fd == COULD_NOT_CONNECT_TO_SOCKET) {
		// means the ZomPHP daemon hasn't been started, let's notify the user
		report_error("Looks like the ZomPHP daemon has not been started. Could not connect to ", socket_name, error, 0);
	} else if (socket_fd >= 0) { // otherwise not much we can do at that point
		sprintf(pid, "%d%s", (int) getpid(), FUNCTION_DELIMITER);
		if (write_string_to_socket(socket_fd, pid, NULL) >= 0) {
			result = 1;
		}

		// we need to close the connection to let the daemon process the data
		close(socket_fd);
	}

	return result;
}

// meant to be called when a SAPI is shutting down, if it has used ZomPHP at all
void notify_zomphp_dying_sapi()
{
	report_pid_to_socket(OUT_SOCKET_PATH, NULL);
}

// returns the file descriptor for the current process's socket, after having connected
// will ask for the socket's creation if it wasn't found
// if the result is < 0, means that some kind of error occurred
int get_socket(xdebug_socket_error* error)
{
	int socket_fd, is_cli, nb_retries;
	char socket_name[SOCKET_PATH_PREFIX_LENGTH + MAX_PID_LENGTH + 2];

	is_cli = is_cli_sapi();

	// is CLI, then let's ask for socket creation, and wait for the deamon to create it (just a reasonable amount of time)
	if (is_cli && !report_pid_to_socket(IN_SOCKET_PATH_CLI, error)) {
		// didn't find the socket for that
		return -1;
	}

	nb_retries = is_cli ? MAX_TRIES_ON_CLI : 1; // don't wait for anything else than CLI
	get_socket_name(socket_name);

	do {
		socket_fd = connect_to_socket(socket_name, ENOENT, error);
		nb_retries--;
	} while(socket_fd < 0 && nb_retries >= 0 && !usleep(WAIT_PER_TRY_ON_CLI));

	if (socket_fd == COULD_NOT_CONNECT_TO_SOCKET && errno == ENOENT && !is_cli) {
		// socket not found, let's ask ZomPHP to create it
		report_pid_to_socket(IN_SOCKET_PATH, error);
	}

	return socket_fd;
}
