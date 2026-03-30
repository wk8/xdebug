// this file contains shims needed to build tests

#ifndef PHUCK_OFF_TESTS_SHIMS_H
#define PHUCK_OFF_TESTS_SHIMS_H

#ifndef XDEBUG_PRIVATE_H
#define XDEBUG_PRIVATE_H

typedef struct _xdebug_func {
    char* class;
    char* function;
    int   type;
    int   internal;
} xdebug_func;

#endif

#endif
