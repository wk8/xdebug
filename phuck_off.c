// wkpo use php_log_err when we can't log???

// wkpo make the scooper get the error logs?

// wkpo make sure no tabs in this file??

#include <stdlib.h>

#include "phuck_off.h"
#include "phuck_off_logger.h"
#include "phuck_off_parser.h"

/****************
 * MAIN SECTION *
 ****************/

typedef struct phuck_off {
    int initialized;
    // the root of where the user defined code lives,
    // as set by the func dumper
    char *user_code_root;
    // maps each absolute file path to a xdebug_hash* mapping
    // its functions' line numbers to their line # in the input file, or NULL if the file is ignored
    xdebug_hash *files;
} phuck_off;

static phuck_off handler = { 0, NULL, NULL };


/***********************
 * END OF MAIN SECTION *
 ***********************/

/***********************
 * PARSER/INIT SECTION *
 ***********************/

static void shutdown_handler(void);

static void init_handler(void) {
    char error[512];

    shutdown_handler();

    if (!phuck_off_parse_funcs_file(PHUCK_OFF_FUNCS_PATH, &handler.files, &handler.user_code_root, error, sizeof(error))) {
        phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "Failed to initialize handler from %s: %s", PHUCK_OFF_FUNCS_PATH, error);
        handler.initialized = 0;
        return;
    }

    handler.initialized = 1;
}

/******************************
 * END OF PARSER/INIT SECTION *
 ******************************/

//

static void shutdown_handler(void) {
    if (handler.files != NULL) {
        xdebug_hash_destroy(handler.files);
        handler.files = NULL;
    }
    if (handler.user_code_root != NULL) {
        free(handler.user_code_root);
        handler.user_code_root = NULL;
    }
    handler.initialized = 0;
}

// static inline int normalize_func_name(xdebug_func f, char buffer[PHUCK_OFF_MAX_FUNC_NAME_LEN]) {
//     int n = 0;
//     switch (f.type) {
//         case XFUNC_NORMAL:
//             n = snprintf(buffer, PHUCK_OFF_MAX_FUNC_NAME_LEN, "%s", f.function);
//             break;
//
//         case XFUNC_STATIC_MEMBER:
//         case XFUNC_MEMBER: {
//             const char* cls;
//             const char* fn;
//             int log_warning = 0;
//
//             if (f.class) {
//                 cls = f.class;
//             } else {
//                 cls = "?";
//                 log_warning = 1;
//             }
//             if (f.function) {
//                 fn = f.function;
//             } else {
//                 fn = "?";
//                 log_warning = 1;
//             }
//
//             n = snprintf(buffer, PHUCK_OFF_MAX_FUNC_NAME_LEN, "%s::%s", cls, fn);
//
//             if (log_warning) {
//                 phuck_off_log(PHUCK_OFF_LOG_LEVEL_WARN, "function \"%s\" missing class or name", buffer);
//             }
//
//             break;
//         }
//     }
//
//     if (n >= PHUCK_OFF_MAX_FUNC_NAME_LEN) {
//         phuck_off_log(PHUCK_OFF_LOG_LEVEL_ERROR, "normalized function name \"%s\" too long (total length %d)", buffer, n);
//         n = 0;
//     }
//
//     buffer[n] = '\0';
//
//     if (n) {
//         for (char* p = buffer; *p; p++) {
//             *p = (char) tolower((unsigned char)*p);
//         }
//     }
//
//     return n;
// }
//
void phuck_off_handle_stack_function(xdebug_func f) {
    (void) f;

//     if (!handler.initialized) return;
//
//     char name[PHUCK_OFF_MAX_FUNC_NAME_LEN];
//     const int name_len = normalize_func_name(f, name);
//
//     if (name_len == 0) return;
//
//     void *val = NULL;
//     int line_no = -1;
//     if (xdebug_hash_find(handler.funcs, name, (unsigned int)name_len, &val)) {
//         line_no = (int)((uintptr_t)val);
//     }
//
//     phuck_off_log(PHUCK_OFF_LOG_LEVEL_DEBUG, "function \"%s\" => line %d", name, line_no);
//
//     // wkpo NEXT: cache the lookup in op_array, see the last 3-4 items of
//     // https://chatgpt.com/c/6940b9e0-a0dc-8330-bdd0-2424f2dd0d85
}

/***********************
 * END OF MAIN SECTION *
 ***********************/

/*************************
 * INIT/SHUTDOWN SECTION *
 *************************/

void phuck_off_init(void) {
    phuck_off_logger_init();
    init_handler();
}

void phuck_off_shutdown(void) {
    shutdown_handler();
    phuck_off_logger_shutdown();
}

/********************************
 * END OF INIT/SHUTDOWN SECTION *
 ********************************/
