/*
	ZomPHP extension for Xdebug
	Author: Jean Rouge (jer329@cornell.edu)
 */

#include <stdlib.h>
#include <string.h>

#include "xdebug_zomphp.h"

#include <sys/time.h> // TODO wkpo

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

void free_and_process_string_list(string_list* sl, process_string func)
{
	string_list_el *current, *next;
	if (sl) {
		next = sl->head;
		while (next) {
			current = next;
			next = current->next;
			if (func) {
				func(current->data);
			}
			free(current->data);
			free(current);
		}
		free(sl);
	}
}

void add_string(string_list* sl, const char* s)
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




// TODO wkpo

#include <stdio.h>
void just_print(const char* s) {
	printf("El: %s\n", s);
}

void test_sl()
{
	string_list* sl = new_string_list();
	add_string(sl, "coucou");
	add_string(sl, "wk");
	add_string(sl, "po");
	free_and_process_string_list(sl, just_print);
	// free_and_process_string_list(sl, NULL);
}

int main()
{
	// test_sl();
	return 0;
}

