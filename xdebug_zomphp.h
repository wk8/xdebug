/*
	ZomPHP extension for Xdebug
	Author: Jean Rouge (jer329@cornell.edu)
 */

#ifndef __HAVE_XDEBUG_ZOMPHP_H__
#define __HAVE_XDEBUG_ZOMPHP_H__

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
void add_string(string_list* sl, const char* s);


// contains all the data: the functions seen so far, the lines we need to push during the next purge,
// and when the last purge was
typedef struct zomphp_data {
	
} zomphp_data;

#endif
