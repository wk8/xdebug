// this file contains shims needed to build tests

#ifndef PHUCK_OFF_TESTS_SHIMS_H
#define PHUCK_OFF_TESTS_SHIMS_H

#ifndef XDEBUG_PRIVATE_H
#define XDEBUG_PRIVATE_H

#include <stddef.h>

typedef struct _xdebug_func {
    char* class;
    char* function;
    int   type;
    int   internal;
} xdebug_func;

typedef struct _zend_function_common {
    int type;
    const char* function_name;
    void* scope;
} zend_function_common;

typedef struct _zend_op_array {
    int type;
    const char* function_name;
    void* scope;
    const char* filename;
    int line_start;
    int line_end;
    void* reserved[8];
} zend_op_array;

typedef union _zend_function {
    int type;
    zend_function_common common;
    zend_op_array op_array;
} zend_function;

typedef struct _zend_function_state {
    zend_function* function;
} zend_function_state;

typedef struct _zend_execute_data {
    zend_function_state function_state;
} zend_execute_data;

typedef struct _zend_xdebug_globals {
    int phuck_off_tracker_offset;
} zend_xdebug_globals;

static zend_xdebug_globals phuck_off_test_globals = { 0 };

#define XG(v) (phuck_off_test_globals.v)
#define ZEND_USER_FUNCTION 2

#endif

#endif
