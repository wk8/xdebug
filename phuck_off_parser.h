#ifndef __HAVE_PHUCK_OFF_PARSER_H__
#define __HAVE_PHUCK_OFF_PARSER_H__

#include <stddef.h>

#include "xdebug_hash.h"

#define PHUCK_OFF_FILES_INITIAL_SLOTS 1024
#define PHUCK_OFF_FILE_LINES_INITIAL_SLOTS 8
#define PHUCK_OFF_GENERATED_FOR_MARKER "### GENERATED FOR ###"

int phuck_off_parse_funcs_file(
    const char *path,
    xdebug_hash **files_out,
    char **user_code_root_out,
    char *error,
    size_t error_len
);

#endif
