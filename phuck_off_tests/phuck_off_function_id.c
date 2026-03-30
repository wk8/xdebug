#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "phuck_off.c"

static int failures = 0;

static void destroy_file_entry(void *value)
{
	if (value) {
		xdebug_hash_destroy((xdebug_hash *) value);
	}
}

static void assert_true(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "%s\n", message);
		failures = 1;
	}
}

static char *dup_string(const char *value)
{
	size_t len;
	char *copy;

	len = strlen(value);
	copy = (char *) malloc(len + 1);
	assert_true(copy != NULL, "failed to allocate string copy");
	if (!copy) {
		return NULL;
	}

	memcpy(copy, value, len + 1);
	return copy;
}

static void reset_test_handler(void)
{
	if (handler.files) {
		xdebug_hash_destroy(handler.files);
		handler.files = NULL;
	}

	if (handler.user_code_root) {
		free(handler.user_code_root);
		handler.user_code_root = NULL;
	}

	handler.user_code_root_len = 0;
	handler.initialized = 0;
}

static void setup_handler(const char *root)
{
	handler.files = xdebug_hash_alloc(8, destroy_file_entry);
	assert_true(handler.files != NULL, "failed to allocate outer files hash");
	handler.user_code_root = dup_string(root);
	handler.user_code_root_len = strlen(handler.user_code_root);
	handler.initialized = 1;
}

static xdebug_hash *add_line_map(const char *path)
{
	xdebug_hash *line_map;

	line_map = xdebug_hash_alloc(8, NULL);
	assert_true(line_map != NULL, "failed to allocate inner line hash");
	if (!line_map) {
		return NULL;
	}

	assert_true(
		xdebug_hash_add(handler.files, (char *) path, (unsigned int) strlen(path), line_map),
		"failed to add line map to outer hash"
	);
	if (failures) {
		xdebug_hash_destroy(line_map);
		return NULL;
	}

	return line_map;
}

static void run_user_root_case(void)
{
	xdebug_hash *line_map;
	const char *main_path = "/tmp/user/code/main.php";
	const char *ignored_path = "/tmp/user/code/ignored.php";

	reset_test_handler();
	setup_handler("/tmp/user/code");
	line_map = add_line_map(main_path);
	if (!line_map) {
		return;
	}

	assert_true(
		xdebug_hash_index_add(line_map, 10, (void *) (uintptr_t) 17),
		"failed to add first function id"
	);
	assert_true(
		xdebug_hash_index_add(line_map, 20, (void *) (uintptr_t) 31),
		"failed to add second function id"
	);
	assert_true(
		xdebug_hash_add(handler.files, (char *) ignored_path, (unsigned int) strlen(ignored_path), NULL),
		"failed to add ignored file entry"
	);

	assert_true(function_id(main_path, 10) == 17, "wrong function id for main.php:10");
	assert_true(function_id(main_path, 20) == 31, "wrong cached function id for main.php:20");
	assert_true(function_id(main_path, 11) == -1, "missing line should return -1");
	assert_true(function_id(ignored_path, 50) == -1, "ignored file should return -1");
	assert_true(function_id(ignored_path, 51) == -1, "cached ignored file should return -1");
	assert_true(function_id("/tmp/user/code/missing.php", 10) == -1, "missing file should return -1");
	assert_true(function_id("/tmp/user/other.php", 10) == -1, "outside-root path should return -1");
	assert_true(function_id("/tmp/user/codebase/main.php", 10) == -1, "prefix-only path should return -1");
	assert_true(function_id(main_path, 0) == -1, "line 0 should return -1");
}

static void run_root_prefix_case(void)
{
	xdebug_hash *line_map;

	reset_test_handler();
	setup_handler("/tmp");
	line_map = add_line_map("/tmp/anywhere.php");
	if (!line_map) {
		return;
	}

	assert_true(
		xdebug_hash_index_add(line_map, 7, (void *) (uintptr_t) 91),
		"failed to add root-prefix function id"
	);
	assert_true(function_id("/tmp/anywhere.php", 7) == 91, "root prefix should match descendant path");
	assert_true(function_id("/tmpx/anywhere.php", 7) == -1, "root prefix should not match sibling prefix");
}

int main(void)
{
	run_user_root_case();
	run_root_prefix_case();
	reset_test_handler();

	if (failures) {
		return 1;
	}

	printf("ok\n");
	return 0;
}
