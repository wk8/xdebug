/*
	ZomPHP extension for Xdebug
	Author: Jean Rouge (jer329@cornell.edu)
 */

#ifndef __HAVE_XDEBUG_ZOMPHP_H__
#define __HAVE_XDEBUG_ZOMPHP_H__

#include <sys/time.h>

#include "xdebug_hash.h"

// only used for debugging - not thread/process-safe!
#define ZOMPHP_DEBUG_MODE 1 // TODO wkpo
#if ZOMPHP_DEBUG_MODE
void zomphp_debug(const char *format, ...);
#define ZOMPHP_DEBUG(format, ...) zomphp_debug(format, ##__VA_ARGS__)
#else
#define ZOMPHP_DEBUG(format, ...)
#endif

// I admittedly could have re-used the xdebug_llist struct here, but I prefer to stay as largely independent from
// modifs in the xdebug code as possible (plus what I needed here is way simpler, and more efficient that way...
// and hopefully without bugs!)
typedef struct string_list_el {
	char* data;
	struct string_list_el* next;
} string_list_el;

typedef struct string_list {
	string_list_el* head;
	string_list_el* tail;
} string_list;

typedef int (*process_string)(const char* s, void* func_additional_arg);

string_list* new_string_list();
int free_and_process_string_list(string_list* sl, process_string func, void* func_additional_arg);
void free_string_list(string_list* sl);
void add_string_to_string_list(string_list* sl, const char* s);


// a magically extensible buffer
typedef struct zomphp_extensible_string {
	size_t current_length;
	char*  data;
} zomphp_extensible_string;

zomphp_extensible_string* new_zomphp_extensible_string();
void free_zomphp_extensible_string(zomphp_extensible_string* ext_string);
zomphp_extensible_string* zomphp_extensible_strcat(zomphp_extensible_string* ext_string, const int nb_strings, ...);
int has_content(const zomphp_extensible_string* ext_string);


// helper hash structs for zomphp_data below
typedef struct zomphp_file_hash_el {
	char* name;
	xdebug_hash* functions;
} zomphp_file_hash_el;
void zomphp_file_hash_el_dtor(void* data);

typedef struct zomphp_function_hash_el {
	char* name;
	int n; // a line # if > 0, or an array length if < 0 (dirty, but economic...)
	int* linenos;
} zomphp_function_hash_el;
void zomphp_function_hash_el_dtor(void* data);

// contains all the data: the functions seen so far, the lines we need to push during the next flush,
// and when the last flush was
typedef struct zomphp_data {
	xdebug_hash* files;
	string_list* new_data;

	char* next_func_name;

	zomphp_file_hash_el* last_file;
	zomphp_function_hash_el* last_func;
	int last_line;

	zomphp_extensible_string* buffer;

	struct timeval* last_flush;

	int nb_consecutive_flushing_errors;
} zomphp_data;

zomphp_data* new_zomphp_data();
void free_zomphp_data(zomphp_data* zd);
void zomphp_register_function_call(zomphp_data* zd, char* filename, char* funcname, int lineno);
int flush_zomphp(zomphp_data* zd);
int flush_zomphp_automatic(zomphp_data* zd);
void set_next_func_name(zomphp_data* zd, const char* funcname);
void zomphp_register_line_call(zomphp_data* zd, char* filename, int lineno);


// the goodies to connect to the socket
int get_zomphp_socket_fd(zomphp_extensible_string** error);
size_t write_string_to_socket(int socket_fd, const char* str);

#endif
