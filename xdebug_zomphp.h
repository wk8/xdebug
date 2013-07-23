/*
	ZomPHP extension for Xdebug
	Author: Jean Rouge (jer329@cornell.edu)
 */

#ifndef __HAVE_XDEBUG_ZOMPHP_H__
#define __HAVE_XDEBUG_ZOMPHP_H__

#include "xdebug_hash.h"


// I admittedly could have re-used the xdebug_llist struct here, but I prefer to stay as largely independent from
// modifs in the xdebug code as possible (plus what I needed here is way simpler, and more efficient that way...
// and hopefully without bug!)
typedef struct string_list_el {
	char* data;
	struct string_list_el* next;
} string_list_el;

typedef struct string_list {
	string_list_el* head;
	string_list_el* tail;
} string_list;

typedef void (*process_string)(const char* s);

string_list* new_string_list();
void free_and_process_string_list(string_list* sl, process_string func);
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


// helper hash structs for zomphp_data below
typedef struct zomphp_file_hash_el {
	char* name;
	xdebug_hash* functions;
} zomphp_file_hash_el;
void zomphp_file_hash_el_dtor(void* data);

typedef struct zomphp_function_hash_el {
	char* name;
	xdebug_hash* linenos;
} zomphp_function_hash_el;
void zomphp_function_hash_el_dtor(void* data);

typedef struct zomphp_line_hash_el {
	char* lineno;
	int count;
} zomphp_line_hash_el;
void zomphp_line_hash_el_dtor(void* data);

// contains all the data: the functions seen so far, the lines we need to push during the next purge,
// and when the last purge was
typedef struct zomphp_data {
	xdebug_hash* files;
	string_list* new_data;

	zomphp_file_hash_el* last_file;
	zomphp_function_hash_el* last_func;
	zomphp_line_hash_el* last_line;

	zomphp_extensible_string* buffer;
	// TODO wkpo last purge, and purge
} zomphp_data;

zomphp_data* new_zomphp_data();
void free_zomphp_data(zomphp_data* zd);
void zomphp_register_function_call(zomphp_data* zd, char* filename, char* funcname, int lneno);

#endif
