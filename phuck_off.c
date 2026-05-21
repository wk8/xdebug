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

static int phuck_off_is_enabled(void) {
    const char* enabled = getenv(PHUCK_OFF_ENABLED_ENV_VAR);

    return enabled != NULL && strcmp(enabled, "1") == 0;
}

static int function_id(const char* path, const int line_no, const char* lookup_name) {
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
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "No function map entry for \"%s\":%d:%s", path, line_no, lookup_name);
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
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "No function id entry for \"%s\":%d:%s", path, line_no, lookup_name);
        return -1;
    }

    return (int) (uintptr_t) line_entry;
}

static const char* include_type_name(const int include_type) {
    switch (include_type) {
        case XFUNC_INCLUDE:
            return "include";
        case XFUNC_INCLUDE_ONCE:
            return "include_once";
        case XFUNC_REQUIRE:
            return "require";
        case XFUNC_REQUIRE_ONCE:
            return "require_once";
        default:
            return NULL;
    }
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
    if (!phuck_off_is_enabled()) {
        phuck_off_logger_shutdown();
        phuck_off_mmap_shutdown();
        shutdown_handler();
        return;
    }

    phuck_off_logger_init();
    phuck_off_sanity_check_init();
    init_handler();
}

void phuck_off_request_init(void) {
    if (!handler.initialized) {
        return;
    }

    phuck_off_mmap_init_for_pid((int) handler.function_count);
}

void phuck_off_post_request(void) {
    if (!handler.initialized) {
        return;
    }

    phuck_off_mmap_post_request();
}

void phuck_off_shutdown(void) {
    phuck_off_mmap_shutdown();
    shutdown_handler();
    phuck_off_logger_shutdown();
}

static void phuck_off_process_tracked_op_array(
    zend_op_array* op_array,
    const int line_no,
    const char* lookup_name,
    const char* cache_subject
) {
    if (!op_array || !handler.initialized) {
        return;
    }

    const char* path = op_array->filename;
    if (!path) {
        return;
    }

    const int phuck_off_offset = XG(phuck_off_tracker_offset);
    const int cached_id = (int) (intptr_t) op_array->reserved[phuck_off_offset];
    int retrieve_from_handler = cached_id == 0 ? 1 : 0;
    int tracked_id = cached_id;

    if (!retrieve_from_handler) {
        retrieve_from_handler = phuck_off_sanity_check_should_sample();
    }

    if (retrieve_from_handler) {
        tracked_id = function_id(path, line_no, lookup_name);

        if (cached_id == 0) {
            op_array->reserved[phuck_off_offset] = (void*) (intptr_t) tracked_id;
            phuck_off_log(PHUCK_OFF_LOG_LEVEL_TRACE, "Caching: %s %s:%d is ID %d", cache_subject, path, line_no, tracked_id);
        } else if (cached_id != tracked_id) {
            op_array->reserved[phuck_off_offset] = (void*) (intptr_t) tracked_id;
            phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "Cache error!! %s %s:%d is ID %d, but cached is %d", cache_subject, path, line_no, tracked_id, cached_id);
        } else {
            phuck_off_log(PHUCK_OFF_LOG_LEVEL_TRACE, "Cache is consistent, %s %s:%d is ID %d", cache_subject, path, line_no, tracked_id);
        }
    } else {
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_TRACE, "Already cached: %s %s:%d is ID %d", cache_subject, path, line_no, tracked_id);
    }

    if (tracked_id > 0 && phuck_off_mmap_bytes != NULL) {
        phuck_off_mmap_set(tracked_id - 1);
    }
}

// this is the meat of our whole fork
// the idea is simple: when a new stack frame appears, if it's a user function that we care about,
// we set the mmap bit for that function to 1
// the added subtlety is that we also cache the function ID in the zen struct for it, to avoid
// repeated hash table lookups
void phuck_off_process_stackframe(zend_execute_data* zdata, zend_op_array* op_array) {
    if (!zdata || !op_array || !handler.initialized) {
        return;
    }

    zend_function* func = zdata->function_state.function;
    if (!func || func->type != ZEND_USER_FUNCTION) {
        return;
    }

    const char* function_name = func->common.function_name;
    if (!function_name || strcmp(function_name, "{main}") == 0) {
        // include frame at top-level
        return;
    }

    if (!op_array->filename) {
        return;
    }

    const int line_no = op_array->line_start;
    phuck_off_log(PHUCK_OFF_LOG_LEVEL_TRACE, "Frame calling user function %s at %s:%d", function_name, op_array->filename, line_no);
    phuck_off_process_tracked_op_array(op_array, line_no, function_name, "function");
}

void phuck_off_process_include(zend_op_array* op_array, const int include_type) {
    if (!op_array || !handler.initialized) {
        return;
    }

    const char* include_name = include_type_name(include_type);
    if (!include_name || !op_array->filename) {
        return;
    }

    phuck_off_log(PHUCK_OFF_LOG_LEVEL_TRACE, "Frame including user file via %s at %s:0", include_name, op_array->filename);
    phuck_off_process_tracked_op_array(op_array, 0, include_name, "included file");
}
