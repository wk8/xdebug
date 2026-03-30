// wkpo use php_log_err when we can't log???

// wkpo make the scooper get the error logs?

// wkpo make sure no tabs in this file??

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "phuck_off.h"
#include "phuck_off_parser.h"

typedef struct phuck_off {
    int initialized;
    // the root of where the user defined code lives,
    // as set by the func dumper
    char* user_code_root;
    size_t user_code_root_len;
    // maps each absolute file path to a xdebug_hash* mapping
    // its functions' line numbers to their line # in the input file, or NULL if the file is ignored
    xdebug_hash* files;
} phuck_off;

static phuck_off handler = { 0, NULL, 0, NULL };

static int function_id(const char* path, const int line_no) {
    size_t path_len;
    void* file_entry;
    void* line_entry = NULL;
    xdebug_hash* line_map;

    if (strncmp(path, handler.user_code_root, handler.user_code_root_len) != 0
        || path[handler.user_code_root_len] == '\0'
        || path[handler.user_code_root_len] != '/'
    ) {
        // not part of the directory we care about
        return -1;
    }

    path_len = strlen(path);
    if (!xdebug_hash_find(handler.files, (char*) path, (unsigned int) path_len, &file_entry)) {
        // we found a file that the dumper missed
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "No function map entry for path \"%s\"", path);
        return -1;
    }

    if (file_entry == NULL) {
        // means we ignore this file
        return -1;
    }

    line_map = (xdebug_hash*) file_entry;
    if (!xdebug_hash_index_find(line_map, (unsigned long) line_no, &line_entry)) {
        // we found a function that the dumper missed
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "No function id entry for \"%s\":%d", path, line_no);
        return -1;
    }

    return (int) (uintptr_t) line_entry;
}

static void shutdown_handler(void);

static void init_handler(void) {
    char error[512];

    shutdown_handler();

    if (!phuck_off_parse_funcs_file(PHUCK_OFF_FUNCS_PATH, &handler.files, &handler.user_code_root, error, sizeof(error))) {
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "Failed to initialize handler from %s: %s", PHUCK_OFF_FUNCS_PATH, error);
        handler.initialized = 0;
        return;
    }

    handler.user_code_root_len = strlen(handler.user_code_root);
    handler.initialized = 1;
}

static void shutdown_handler(void) {
    if (handler.files != NULL) {
        xdebug_hash_destroy(handler.files);
        handler.files = NULL;
    }
    if (handler.user_code_root != NULL) {
        free(handler.user_code_root);
        handler.user_code_root = NULL;
    }
    handler.user_code_root_len = 0;
    handler.initialized = 0;
}


void phuck_off_init(void) {
    phuck_off_logger_init();
    init_handler();
}

void phuck_off_shutdown(void) {
    shutdown_handler();
    phuck_off_logger_shutdown();
}
