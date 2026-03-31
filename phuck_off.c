// wkpo use php_log_err when we can't log???
// wkpo make the scooper get the error logs?

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifndef XG
#include "php_xdebug.h"
ZEND_EXTERN_MODULE_GLOBALS(xdebug)
#endif

#include "phuck_off.h"
#include "phuck_off_parser.h"
#include "phuck_off_sanity_check.h"

typedef struct phuck_off {
    int initialized;

    // the root of where the user defined code lives,
    // as set by the func dumper
    char* user_code_root;
    size_t user_code_root_len;
    size_t function_count;
    // maps each absolute file path to a xdebug_hash* mapping
    // its functions' line numbers to their line # in the input file, or NULL if the file is ignored
    xdebug_hash* files;
} phuck_off;

static phuck_off handler = { 0, NULL, 0, 0, NULL };

static int function_id(const char* path, const int line_no) {
    if (line_no == 0) {
        // file is being required
        return -1;
    }

    if (strncmp(path, handler.user_code_root, handler.user_code_root_len) != 0
        || path[handler.user_code_root_len] == '\0'
        || path[handler.user_code_root_len] != '/'
    ) {
        // not part of the directory we care about
        return -1;
    }

    const size_t path_len = strlen(path);
    void* file_entry;
    if (!xdebug_hash_find(handler.files, (char*) path, (unsigned int) path_len, &file_entry)) {
        // we found a file that the dumper missed
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "No function map entry for path \"%s\"", path);
        return -1;
    }

    if (file_entry == NULL) {
        // means we ignore this file
        return -1;
    }

    xdebug_hash* line_map = (xdebug_hash*) file_entry;
    void* line_entry = NULL;
    if (!xdebug_hash_index_find(line_map, (unsigned long) line_no, &line_entry)) {
        // we found a function that the dumper missed
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "No function id entry for \"%s\":%d", path, line_no);
        return -1;
    }

    return (int) (uintptr_t) line_entry;
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
    handler.function_count = 0;
    handler.initialized = 0;
}

static void init_handler(void) {
    char error[512];

    shutdown_handler();

    if (!phuck_off_parse_funcs_file(PHUCK_OFF_FUNCS_PATH, &handler.files, &handler.user_code_root, &handler.function_count, error, sizeof(error))) {
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "Failed to initialize handler from %s: %s", PHUCK_OFF_FUNCS_PATH, error);
        handler.initialized = 0;
        return;
    }

    handler.user_code_root_len = strlen(handler.user_code_root);
    handler.initialized = 1;
}

void phuck_off_init(void) {
    phuck_off_logger_init();
    phuck_off_sanity_check_init();
    init_handler();
    if (handler.initialized && !phuck_off_mmap_init_for_pid((int) handler.function_count)) {
        shutdown_handler();
    }
}

void phuck_off_post_request(void) {
    phuck_off_mmap_post_request();
}

void phuck_off_shutdown(void) {
    phuck_off_mmap_shutdown();
    shutdown_handler();
    phuck_off_logger_shutdown();
}

// this is the meat of our whole fork
// the idea is simple: when a new stack frame appears, if it's a user function that we care about,
// we set the mmap bit for that function to 1
// the added subtlety is that we also cache the function ID in the zen struct for it, to avoid
// repeated hash table lookups
void phuck_off_process_stackframe(zend_execute_data* zdata) {
    if (!zdata || !handler.initialized) {
        return;
    }

    zend_function* func = zdata->function_state.function;
    if (!func || func->type != ZEND_USER_FUNCTION) {
        return;
    }

    zend_op_array* oa = &func->op_array;
    const char* path = oa->filename;
    if (!path) {
        return;
    }

    const int phuck_off_offset = XG(phuck_off_tracker_offset);
    const int line_no = oa->line_start;
    const int cached_id = (int) (intptr_t) oa->reserved[phuck_off_offset];
    int retrieve_from_handler = cached_id == 0 ? 1 : 0;
    int func_id = cached_id;

    phuck_off_log(PHUCK_OFF_LOG_LEVEL_TRACE, "Frame calling user function %s:%d", path, line_no);
    if (!retrieve_from_handler) {
        retrieve_from_handler = phuck_off_sanity_check_should_sample();
    }

    if (retrieve_from_handler) {
        func_id = function_id(path, line_no);

        if (cached_id == 0) {
            oa->reserved[phuck_off_offset] = (void*) (intptr_t) func_id;
            phuck_off_log(PHUCK_OFF_LOG_LEVEL_TRACE, "Caching: function %s:%d is ID %d", path, line_no, func_id);
        } else if (cached_id != func_id) {
            oa->reserved[phuck_off_offset] = (void*) (intptr_t) func_id;
            phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "Cache error!! function %s:%d is ID %d, but cached is %d",
                          path, line_no, func_id, cached_id);
        } else {
            phuck_off_log(PHUCK_OFF_LOG_LEVEL_TRACE, "Cache is consistent, function %s:%d is ID %d", path, line_no, func_id);
        }
    } else {
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_TRACE, "Already cached: function %s:%d is ID %d", path, line_no, func_id);
    }

    if (func_id > 0) {
        phuck_off_mmap_set(func_id - 1);
    }
}
