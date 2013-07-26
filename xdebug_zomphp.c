/*
	ZomPHP extension for Xdebug
	Author: Jean Rouge (jer329@cornell.edu)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "xdebug_zomphp.h"

#define free(x) ZOMPHP_DEBUG("On free %d", x); free(x) // TODO wkpo

#define ZOMPHP_SOCKET_PATH "/tmp/zomphp.socket"

#if ZOMPHP_DEBUG_MODE

#define ZOMPHP_DEBUG_LOG_FILE "/tmp/zomphp_debug.log"

void zomphp_debug(const char *format, ...)
{
	FILE *f ;
	va_list args;
	struct timeval tv;
	f = fopen(ZOMPHP_DEBUG_LOG_FILE, "a+");
	gettimeofday(&tv, NULL);
	fprintf(f, "[ %lu-%lu (%d)] ", (unsigned long)tv.tv_sec, (unsigned long)tv.tv_usec, (int) getpid());
	va_start(args, format);
	vfprintf(f, format, args);
	fprintf(f, "\n");
	va_end(args);
	fclose(f);
}

#endif

#define FLUSH_DELAY -1 // the # of seconds between two automatic flushes // TODO wkpo 300
#define MAX_CONSECUTIVE_ERRORS 10 // the # of successive errors when pushing to the daemon beofre giving up

// {{{ STRING_LIST }}}

string_list* new_string_list()
{
	string_list* result;
	result = (string_list*) malloc(sizeof(string_list));
	if (!result) {
		return NULL;
	}
	result->head = NULL;
	result->tail = NULL;
	return result;
}

// func should return an int
// if func returns something else than 0, means something went wrong, and we stop here
// we return 0 on success, or anything we got from func on failure
int free_and_process_string_list(string_list* sl, process_string func, void* func_additional_arg)
{
	string_list_el *current, *next;
	int result = 0;
	if (sl) {
		next = sl->head;
		while (next) {
			current = next;
			next = current->next;
			if (func) {
				result = func(current->data, func_additional_arg);
				if (result != 0) {
					ZOMPHP_DEBUG("Something went wrong when processing list string");
					// something went wrong, we stop here
					sl->head = current;
					return result;
				}
			}
			free(current->data);
			free(current);
		}
		free(sl);
	}
	return result;
}

void free_string_list(string_list* sl)
{
	free_and_process_string_list(sl, NULL, NULL);
}

void add_string_to_string_list(string_list* sl, const char* s)
{
	string_list_el* new_el;
	char* data;
	if (!sl || !s) {
		return;
	}
	data = strdup(s);
	if (!data) {
		return;
	}
	new_el = (string_list_el*) malloc(sizeof(string_list_el));
	if (!new_el) {
		free(data);
		return;
	}
	new_el->data = data;
	new_el->next = NULL;
	if (!sl->head) {
		sl->head = new_el;
		sl->tail = new_el;
	} else {
		sl->tail->next = new_el;
		sl->tail = new_el;
	}
}

// {{{ END OF STRING_LIST }}}


// {{{ ZOMPHP_EXTENSIBLE_STRING }}}

#define ZOMPHP_EXTENSIBLE_STRING_DELTA_INCR 100

zomphp_extensible_string* zomphp_extensible_strcat(zomphp_extensible_string* ext_string, const int nb_strings, ...)
{
	va_list args;
	char *current_string, *buffer;
	size_t current_pos, current_length;
	int i;

	if (!ext_string) {
		return NULL;
	}

	va_start(args, nb_strings);
	current_pos = 0;
	for (i = 0; i != nb_strings; i++) {
		current_string = va_arg(args, char*);
		current_length = strlen(current_string);
		if (current_pos + current_length >= ext_string->current_length) {
			// we need to make the container bigger!
			ext_string->current_length += current_length > ZOMPHP_EXTENSIBLE_STRING_DELTA_INCR ? current_length : ZOMPHP_EXTENSIBLE_STRING_DELTA_INCR;
			buffer = (char*) malloc(sizeof(char) * ext_string->current_length);
			if (!buffer) {
				// not enough RAM, we're done with that thing
				free_zomphp_extensible_string(ext_string);
				return NULL;
			}
			// copy everything up to that point
			memcpy(buffer, ext_string->data, sizeof(char) * current_pos);
			// and make that the new data
			free(ext_string->data);
			ext_string->data = buffer;
		}
		memcpy(ext_string->data + current_pos, current_string, current_length * sizeof(char));
		current_pos += current_length;
	}
	memcpy(ext_string->data + current_pos, "\0", sizeof(char));
	va_end(args);

	return ext_string;
}

zomphp_extensible_string* new_zomphp_extensible_string()
{
	zomphp_extensible_string* result;
	char* data;

	result = (zomphp_extensible_string*) malloc(sizeof(zomphp_extensible_string));
	data = (char*) malloc(sizeof(char) * ZOMPHP_EXTENSIBLE_STRING_DELTA_INCR);

	if (!result || !data) {
		free_zomphp_extensible_string(result);
		if (data) {
			free(data);
		}
		return NULL;
	}

	memcpy(data, "\0", sizeof(char));
	result->data = data;
	result->current_length = ZOMPHP_EXTENSIBLE_STRING_DELTA_INCR;
	return result;
}

void free_zomphp_extensible_string(zomphp_extensible_string* ext_string)
{
	if (ext_string) {
		if (ext_string->data) {
			free(ext_string->data);
		}
		free(ext_string);
	}
}

int has_content(const zomphp_extensible_string* ext_string)
{
	return ext_string && ext_string->data && strlen(ext_string->data) ?  1 : 0;
}

// {{{ END OF ZOMPHP_EXTENSIBLE_STRING }}}


// {{{ ZOMPHP_DATA }}}

void zomphp_file_hash_el_dtor(void* data)
{
	zomphp_file_hash_el *file;
	if (!data) {
		return;
	}
	file = (zomphp_file_hash_el*) data;
	xdebug_hash_destroy(file->functions);
	free(file->name);
	free(file);
}

void zomphp_function_hash_el_dtor(void* data)
{
	zomphp_function_hash_el* function;
	if (!data) {
		return;
	}
	function = (zomphp_function_hash_el*) data;
	if (function->linenos) {
		free(function->linenos);
	}
	free(function->name);
	free(function);
}

zomphp_data* new_zomphp_data()
{
	zomphp_data* result;
	result = (zomphp_data*) malloc(sizeof(zomphp_data));
	if (!result) {
		return NULL;
	}
	result->files = xdebug_hash_alloc(32768, zomphp_file_hash_el_dtor);
	result->new_data = new_string_list();
	result->buffer = new_zomphp_extensible_string();
	result->last_flush = (struct timeval*) malloc(sizeof(struct timeval));
	if (!result->files || !result->new_data || !result->buffer || !result->last_flush) {
		free_zomphp_data(result);
		return NULL;
	}
	gettimeofday(result->last_flush, NULL);
	result->next_func_name = NULL;
	result->last_file = NULL;
	result->last_func = NULL;
	result->last_line = 0;
	result->nb_consecutive_flushing_errors = 0;
	return result;
}

void free_zomphp_data(zomphp_data* zd)
{
	if (zd) {
		if (zd->files) {
			xdebug_hash_destroy(zd->files);
		}
		if (zd->last_flush) {
			free(zd->last_flush);
		}
		if (zd->next_func_name) {
			free(zd->next_func_name);
		}
		free_string_list(zd->new_data);
		free_zomphp_extensible_string(zd->buffer);
		free(zd);
	}
}

int report_item_to_daemon(const char* s, void* socket_fd)
{
	int* sfd = (int*) socket_fd;
	ZOMPHP_DEBUG("Reporting to daemon! %s (to socket_fd %d)", s, *sfd);
	return write_string_to_socket(*sfd, s) == -1 ? -1 : 0;
}

// returns 0 iff we don't need to give up just yet
int flush_zomphp(zomphp_data* zd)
{
	int socket_fd;
	if (zd) {
		socket_fd = get_zomphp_socket_fd(NULL);
		ZOMPHP_DEBUG("Flushing! (to socket_fd %d)", socket_fd);
		if (free_and_process_string_list(zd->new_data, report_item_to_daemon, (void*) &socket_fd) == 0) {
			// all went well
			zd->new_data = new_string_list();
			gettimeofday(zd->last_flush, NULL);
			zd->nb_consecutive_flushing_errors = 0;
		} else {
			zd->nb_consecutive_flushing_errors++;
			ZOMPHP_DEBUG("Error when flushing... (current count %d, max %d)", zd->nb_consecutive_flushing_errors, MAX_CONSECUTIVE_ERRORS);
			if (zd->nb_consecutive_flushing_errors > MAX_CONSECUTIVE_ERRORS) {
				ZOMPHP_DEBUG("Too many errors! %d VS %d allowed, giving up", zd->nb_consecutive_flushing_errors, MAX_CONSECUTIVE_ERRORS);
				return 1;
			}
		}
	}
	return 0;
}

// same idea as flush_zomphp
int flush_zomphp_automatic(zomphp_data* zd)
{
	struct timeval tv;
	if (zd) {
		gettimeofday(&tv, NULL);
		if (tv.tv_sec - zd->last_flush->tv_sec > FLUSH_DELAY) {
			ZOMPHP_DEBUG("Flushing! It's been %lu secs since last time (vs %lu allowed)", (unsigned long)(tv.tv_sec - zd->last_flush->tv_sec), FLUSH_DELAY);
			return flush_zomphp(zd);
		} else {
			ZOMPHP_DEBUG("No need to flush, it's been only %lu secs (vs %lu allowed)", (unsigned long)(tv.tv_sec - zd->last_flush->tv_sec), FLUSH_DELAY);
		}
	}
	return 0;
}

#define MAX_LINE_NB INT_MAX
#define MAX_LINE_NB_LENGTH ((int) (ceil(log10(MAX_LINE_NB)) + 2))
#define ITEM_DELIMITER "\n"
#define INTRA_DELIMITER ":"

void zomphp_register_function_call(zomphp_data* zd, char* filename, char* funcname, int lineno)
{
	char same_so_far, is_new; // booleans
	char lineno_buffer[MAX_LINE_NB_LENGTH];
	int i, *new_linenos;
	zomphp_file_hash_el* file;
	zomphp_function_hash_el* func;

	ZOMPHP_DEBUG("Processing new func call: %s:%s:%d", filename, funcname, lineno);

	if (!zd || !filename || !funcname || !lineno) {
		return;
	}

	is_new = 0;
	if (zd->last_file && !strcmp(zd->last_file->name, filename)) {
		same_so_far = 1;
		file = zd->last_file;
	} else {
		same_so_far = 0;
		if (!xdebug_hash_find(zd->files, filename, strlen(filename), (void*) &file)) {
			is_new = 1;
			file = malloc(sizeof(zomphp_file_hash_el));
			file->name = strdup(filename);
			file->functions = xdebug_hash_alloc(32, zomphp_function_hash_el_dtor);
			xdebug_hash_add(zd->files, filename, strlen(filename), file);
		}
		zd->last_file = file;
	}

	if (same_so_far && zd->last_func && !strcmp(zd->last_func->name, funcname)) {
		func = zd->last_func;
	} else {
		same_so_far = 0;
		if (is_new || !xdebug_hash_find(file->functions, funcname, strlen(funcname), (void*) &func)) {
			is_new = 1;
			func = malloc(sizeof(zomphp_function_hash_el));
			func->name = strdup(funcname);
			func->linenos = NULL;
			func->n = lineno;
			xdebug_hash_add(file->functions, funcname, strlen(funcname), func);
		}
		zd->last_func = func;
	}

	if (same_so_far && zd->last_line && zd->last_line == lineno) {
		// it's all the same, nothing to do
		ZOMPHP_DEBUG("Cache hit for %s:%s:%d", filename, funcname, lineno);
		return;
	}

	if (!is_new) {
		// if is_new is true at this point, no need to look any further
		// otherwise means the func hasn't been created that time around, let's
		// see if we already know about this lineno
		if (func->n > 0) {
			if (func->n != lineno) {
				// otherwise means we already know it
				is_new = 1;
				// we need to switch to a multi lineno
				func->linenos = (int*) malloc(sizeof(int) * 2);
				func->linenos[0] = func->n;
				func->linenos[1] = lineno;
				func->n = -2;
			}
		} else {
			// it's already a multi, let's see if we already have that line no
			i = 1;
			while(--i != func->n && func->linenos[-i] != lineno);
			if (i == func->n) {
				// we haven't found that lineno
				is_new = 1;
				// we need to enlarge the array
				new_linenos = (int*) malloc(sizeof(int) * (-func->n + 1));
				memcpy(new_linenos, func->linenos, -sizeof(int) * func->n);
				new_linenos[-func->n] = lineno;
				free(func->linenos);
				func->linenos = new_linenos;
				func->n--;
			}
		}
	}
	zd->last_line = lineno;

	if (is_new) {
		ZOMPHP_DEBUG("That func call is new! %s:%s:%d", filename, funcname, lineno);
		// build the new string to be pushed, and append it to the list
		// we need to convert the line nb to a string
		sprintf(lineno_buffer, "%d", lineno);
		zd->buffer = zomphp_extensible_strcat(zd->buffer, 6, filename, INTRA_DELIMITER, funcname, INTRA_DELIMITER, lineno_buffer, ITEM_DELIMITER);
		if (zd->new_data && zd->buffer) {
			add_string_to_string_list(zd->new_data, zd->buffer->data);
		}
	} else {
		ZOMPHP_DEBUG("That func call is nothing new. %s:%s:%d", filename, funcname, lineno);
	}
}

void set_next_func_name(zomphp_data* zd, const char* funcname)
{
	if (zd) {
		if (zd->next_func_name) {
			// shouldn't happen, but let's avoid mem leaks...
			free(zd->next_func_name);
		}
		zd->next_func_name = strdup(funcname);
	}
}

void zomphp_register_line_call(zomphp_data* zd, char* filename, int lineno)
{
	if (zd && zd->next_func_name) {
		zomphp_register_function_call(zd, filename, zd->next_func_name, lineno);
		free(zd->next_func_name);
		zd->next_func_name = NULL;
	}
}

// {{{ END OF ZOMPHP_DATA }}}


// {{{ ZOMPHP SOCKET }}}

// a helper function for the one below
void report_error(const char* msg, zomphp_extensible_string** error)
{
	if (!error || !*error) {
		return;
	}
	*error = zomphp_extensible_strcat(*error, 5, "ZomPHP error! ", msg, ZOMPHP_SOCKET_PATH, " : ", strerror(errno));
	// reset errno
	errno = 0;
}

// tries to connect to ZomPHP's deamon's socket
// if the result is < 0, means that some kind of error occurred
// (more specifically SOCKET_NOT_CREATED if couldn't create the socket, COULD_NOT_CONNECT_TO_SOCKET if coulnd't connect
// - in which case it also sets the error accordingly)
int get_zomphp_socket_fd(zomphp_extensible_string** error)
{
	int socket_fd;
	struct sockaddr_un serv_addr;
	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		ZOMPHP_DEBUG("Could not create socket");
		report_error("Could not create socket ", error);
		return -1;
	}

	serv_addr.sun_family = AF_UNIX;
	memcpy(serv_addr.sun_path, ZOMPHP_SOCKET_PATH, sizeof(char) * (strlen(ZOMPHP_SOCKET_PATH) + 1)); // +1 for the terminating char // TODO wkpo sun.path = ZOMPHP_SOCKET_PATH; ??

	if (connect(socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		ZOMPHP_DEBUG("Could not connect to socket");
		report_error("Could not connect to socket ", error);
		return -1;
	}

	return socket_fd;
}

// returns -1 if any error
size_t write_string_to_socket(int socket_fd, const char* str)
{
	size_t result, str_length;
	str_length = strlen(str);
	if (socket_fd < 0 || !str_length) {
		return -1;
	}
	result = write(socket_fd, str, str_length);
	return result < str_length ? -1 : result;
}

// {{{ END OF ZOMPHP SOCKET }}}
