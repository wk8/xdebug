/*
	ZomPHP extension for Xdebug
	Author: Jean Rouge (jer329@cornell.edu)
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

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

void free_string_list(string_list* sl)
{
	free_and_process_string_list(sl, NULL);
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
	xdebug_hash_destroy(function->linenos);
	free(function->name);
	free(function);
}

void zomphp_line_hash_el_dtor(void* data)
{
	zomphp_line_hash_el* line;
	if (!data) {
		return;
	}
	line = (zomphp_line_hash_el*) data;
	free(line->lineno);
	free(line);
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
	if (!result->files || !result->new_data) {
		free_zomphp_data(result);
		return NULL;
	}
	result->last_file = NULL;
	result->last_func = NULL;
	result->last_line = NULL;
	return result;
}

void free_zomphp_data(zomphp_data* zd)
{
	if (zd) {
		if (zd->files) {
			xdebug_hash_destroy(zd->files);
		}
		free_string_list(zd->new_data);
		free(zd);
	}
}

#define MAX_LINE_NB INT_MAX
#define MAX_LINE_NB_LENGTH ((int) (ceil(log10(MAX_LINE_NB)) + 2))

void zomphp_register_function_call(zomphp_data* zd, char* filename, char* funcname, int lneno)
{
	char same_so_far;
	char lineno[MAX_LINE_NB_LENGTH];
	zomphp_file_hash_el* file;
	zomphp_function_hash_el* func;
	zomphp_line_hash_el* line;

	if (!zd || !filename || !funcname || !lineno) {
		return;
	}

	// we need to convert the line nb to a string to be able to use xdebug hashes
	if (lneno > MAX_LINE_NB) {
		lneno = MAX_LINE_NB;
	}
	sprintf(lineno, "%d", lneno);

	if (zd->last_file && !strcmp(zd->last_file->name, filename)) {
		same_so_far = 1;
		file = zd->last_file;
	} else {
		same_so_far = 0;
		if (!xdebug_hash_find(zd->files), filename, strlen(filename), (void*) &file) {
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
		if (!xdebug_hash_find(file->functions, funcname, strlen(funcname), (void*) &func)) {
			func = malloc(sizeof(zomphp_function_hash_el));
			func->name = strdup(funcname);
			func->linenos = xdebug_hash_alloc(4, zomphp_line_hash_el_dtor);
			xdebug_hash_add(file->functions, funcname, strlen(funcname), func);
		}
		zd->last_func = func;
	}

	if (same_so_far && zd->last_line && !strcmp(zd->last_line->lineno, lineno)) {
		line = zd->last_line;
	} else {
		if (!xdebug_hash_find(func->linenos, lineno, strlen(lineno), (void*) &line)) {
			line = malloc(sizeof(zomphp_line_hash_el));
			line->lineno = strdup(lineno);
			line->count = 0;
			if (zd->new_data) {
				// TODO wkpo construire et pusher le new string
			}
		}
		zd->last_line = line;
	}
	line->count++;
}

// {{{ END OF ZOMPHP_DATA }}}

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

