#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "phuck_off_parser.h"

typedef struct phuck_off_resize_state {
    int ok;
    char *error;
    size_t error_len;
} phuck_off_resize_state;

static void phuck_off_parser_set_error(char *error, size_t error_len, const char *format, ...)
{
    va_list args;

    if (!error || error_len == 0) {
        return;
    }

    va_start(args, format);
    vsnprintf(error, error_len, format, args);
    va_end(args);
}

static char *phuck_off_parser_strdup(const char *value)
{
    size_t len;
    char *copy;

    if (!value) {
        return NULL;
    }

    len = strlen(value);
    copy = (char *) malloc(len + 1);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, value, len + 1);
    return copy;
}

static char *phuck_off_parser_read_line(FILE *fp)
{
    char *buffer;
    char *tmp;
    size_t capacity = 256;
    size_t length = 0;
    int ch;

    buffer = (char *) malloc(capacity);
    if (!buffer) {
        return NULL;
    }

    while ((ch = fgetc(fp)) != EOF) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            tmp = (char *) realloc(buffer, capacity);
            if (!tmp) {
                free(buffer);
                return NULL;
            }
            buffer = tmp;
        }

        buffer[length++] = (char) ch;
        if (ch == '\n') {
            break;
        }
    }

    if (length == 0 && ch == EOF) {
        free(buffer);
        return NULL;
    }

    buffer[length] = '\0';
    return buffer;
}

static void phuck_off_parser_chomp(char *line)
{
    size_t len;

    if (!line) {
        return;
    }

    len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[--len] = '\0';
    }
}

static void phuck_off_parser_files_dtor(void *value)
{
    if (value) {
        xdebug_hash_destroy((xdebug_hash *) value);
    }
}

static int phuck_off_parser_target_slots(size_t entries)
{
    size_t slots;

    if (entries == 0) {
        return 1;
    }

    slots = entries / 2;
    if (entries % 2 != 0) {
        slots++;
    }

    if (slots > (size_t) INT_MAX) {
        return INT_MAX;
    }

    return (int) slots;
}

static int phuck_off_parser_maybe_grow_hash(xdebug_hash *hash, char *error, size_t error_len)
{
    int target_slots;

    if (!hash) {
        return 1;
    }

    target_slots = phuck_off_parser_target_slots(hash->size);
    if (target_slots <= hash->slots) {
        return 1;
    }

    if (!xdebug_hash_resize(hash, target_slots)) {
        phuck_off_parser_set_error(
            error,
            error_len,
            "failed to resize hash from %d to %d slots",
            hash->slots,
            target_slots
        );
        return 0;
    }

    return 1;
}

static void phuck_off_parser_resize_inner_hashes(void *user, xdebug_hash_element *element)
{
    phuck_off_resize_state *state = (phuck_off_resize_state *) user;
    xdebug_hash *inner = (xdebug_hash *) element->ptr;

    if (!state->ok || !inner) {
        return;
    }

    state->ok = phuck_off_parser_maybe_grow_hash(inner, state->error, state->error_len);
}

static xdebug_hash *phuck_off_parser_get_or_create_file_lines(xdebug_hash *files, const char *path)
{
    xdebug_hash *line_map;
    void *existing = NULL;

    if (xdebug_hash_find(files, (char *) path, (unsigned int) strlen(path), &existing)) {
        return (xdebug_hash *) existing;
    }

    line_map = xdebug_hash_alloc(PHUCK_OFF_FILE_LINES_INITIAL_SLOTS, NULL);
    if (!line_map) {
        return NULL;
    }

    if (!xdebug_hash_add(files, (char *) path, (unsigned int) strlen(path), line_map)) {
        xdebug_hash_destroy(line_map);
        return NULL;
    }

    return line_map;
}

static int phuck_off_parser_add_function_entry(
    xdebug_hash *files,
    char *line,
    unsigned long input_line_no,
    char *error,
    size_t error_len
)
{
    char *separator;
    char *path;
    char *end = NULL;
    unsigned long function_line_no;
    xdebug_hash *line_map;

    separator = strrchr(line, ':');
    if (!separator || separator == line || separator[1] == '\0') {
        phuck_off_parser_set_error(error, error_len, "invalid function entry on line %lu", input_line_no);
        return 0;
    }

    *separator = '\0';
    path = line;

    errno = 0;
    function_line_no = strtoul(separator + 1, &end, 10);
    if (errno != 0 || !end || *end != '\0') {
        phuck_off_parser_set_error(error, error_len, "invalid line number on line %lu", input_line_no);
        return 0;
    }

    line_map = phuck_off_parser_get_or_create_file_lines(files, path);
    if (!line_map) {
        phuck_off_parser_set_error(error, error_len, "failed to allocate line map for \"%s\"", path);
        return 0;
    }

    if (!xdebug_hash_index_add(line_map, function_line_no, (void *) (uintptr_t) input_line_no)) {
        phuck_off_parser_set_error(error, error_len, "failed to store function entry for \"%s\"", path);
        return 0;
    }

    return 1;
}

static int phuck_off_parser_add_ignored_file(
    xdebug_hash *files,
    const char *path,
    unsigned long input_line_no,
    char *error,
    size_t error_len
)
{
    if (!xdebug_hash_update(files, (char *) path, (unsigned int) strlen(path), NULL)) {
        phuck_off_parser_set_error(error, error_len, "failed to store ignored file on line %lu", input_line_no);
        return 0;
    }

    return 1;
}

int phuck_off_parse_funcs_file(
    const char *path,
    xdebug_hash **files_out,
    char **user_code_root_out,
    char *error,
    size_t error_len
)
{
    enum {
        PHUCK_OFF_PARSE_FUNCTIONS = 0,
        PHUCK_OFF_PARSE_ROOT = 1,
        PHUCK_OFF_PARSE_IGNORED = 2
    } state = PHUCK_OFF_PARSE_FUNCTIONS;

    FILE *fp;
    xdebug_hash *files = NULL;
    char *line = NULL;
    char *user_code_root = NULL;
    unsigned long input_line_no = 0;
    phuck_off_resize_state resize_state = { 1, error, error_len };

    if (files_out) {
        *files_out = NULL;
    }
    if (user_code_root_out) {
        *user_code_root_out = NULL;
    }
    if (error && error_len > 0) {
        error[0] = '\0';
    }

    fp = fopen(path, "r");
    if (!fp) {
        phuck_off_parser_set_error(error, error_len, "failed to open \"%s\": %s", path, strerror(errno));
        return 0;
    }

    files = xdebug_hash_alloc(PHUCK_OFF_FILES_INITIAL_SLOTS, phuck_off_parser_files_dtor);
    if (!files) {
        phuck_off_parser_set_error(error, error_len, "failed to allocate files hash");
        fclose(fp);
        return 0;
    }

    while ((line = phuck_off_parser_read_line(fp)) != NULL) {
        input_line_no++;
        phuck_off_parser_chomp(line);

        if (state != PHUCK_OFF_PARSE_ROOT && line[0] == '\0') {
            free(line);
            line = NULL;
            continue;
        }

        if (state == PHUCK_OFF_PARSE_FUNCTIONS) {
            if (strcmp(line, PHUCK_OFF_GENERATED_FOR_MARKER) == 0) {
                state = PHUCK_OFF_PARSE_ROOT;
            } else if (!phuck_off_parser_add_function_entry(files, line, input_line_no, error, error_len)) {
                free(line);
                fclose(fp);
                xdebug_hash_destroy(files);
                return 0;
            }
        } else if (state == PHUCK_OFF_PARSE_ROOT) {
            if (line[0] == '\0') {
                phuck_off_parser_set_error(error, error_len, "missing user_code_root on line %lu", input_line_no);
                free(line);
                fclose(fp);
                xdebug_hash_destroy(files);
                return 0;
            }

            user_code_root = phuck_off_parser_strdup(line);
            if (!user_code_root) {
                phuck_off_parser_set_error(error, error_len, "failed to allocate user_code_root");
                free(line);
                fclose(fp);
                xdebug_hash_destroy(files);
                return 0;
            }
            state = PHUCK_OFF_PARSE_IGNORED;
        } else if (!phuck_off_parser_add_ignored_file(files, line, input_line_no, error, error_len)) {
            free(line);
            fclose(fp);
            free(user_code_root);
            xdebug_hash_destroy(files);
            return 0;
        }

        free(line);
        line = NULL;
    }

    if (ferror(fp)) {
        phuck_off_parser_set_error(error, error_len, "failed while reading \"%s\"", path);
        fclose(fp);
        free(user_code_root);
        xdebug_hash_destroy(files);
        return 0;
    }

    fclose(fp);

    if (state == PHUCK_OFF_PARSE_FUNCTIONS) {
        phuck_off_parser_set_error(error, error_len, "missing \"%s\" marker", PHUCK_OFF_GENERATED_FOR_MARKER);
        free(user_code_root);
        xdebug_hash_destroy(files);
        return 0;
    }

    if (state == PHUCK_OFF_PARSE_ROOT || !user_code_root) {
        phuck_off_parser_set_error(error, error_len, "missing user_code_root after \"%s\"", PHUCK_OFF_GENERATED_FOR_MARKER);
        free(user_code_root);
        xdebug_hash_destroy(files);
        return 0;
    }

    if (!phuck_off_parser_maybe_grow_hash(files, error, error_len)) {
        free(user_code_root);
        xdebug_hash_destroy(files);
        return 0;
    }

    xdebug_hash_apply(files, &resize_state, phuck_off_parser_resize_inner_hashes);
    if (!resize_state.ok) {
        free(user_code_root);
        xdebug_hash_destroy(files);
        return 0;
    }

    if (files_out) {
        *files_out = files;
    }
    if (user_code_root_out) {
        *user_code_root_out = user_code_root;
    }

    return 1;
}
