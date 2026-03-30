#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "phuck_off_parser.h"

static int failures = 0;

static void assert_true(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        failures = 1;
    }
}

static void write_fixture_file(FILE* fp) {
    int i;

    for (i = 0; i < 17; i++) {
        fprintf(fp, "/tmp/user/code/main.php:%d\n", 10 + i);
    }
    fprintf(fp, "/tmp/user/code/ignored_later.php:50\n");
    fprintf(fp, "%s\n", PHUCK_OFF_GENERATED_FOR_MARKER);
    fprintf(fp, "/tmp/user/code\n");
    fprintf(fp, "/tmp/user/code/ignored_later.php\n");
    for (i = 0; i < 2047; i++) {
        fprintf(fp, "/tmp/user/code/vendor/file_%04d.php\n", i);
    }
}

int main(void) {
    char path_template[] = "/tmp/phuck_off_parser.XXXXXX";
    int fd;
    FILE* fp;
    xdebug_hash* files = NULL;
    xdebug_hash* main_lines = NULL;
    void* value = NULL;
    char* user_code_root = NULL;
    char error[512];

    fd = mkstemp(path_template);
    assert_true(fd >= 0, "failed to create temp fixture");
    if (fd < 0) {
        return 1;
    }

    fp = fdopen(fd, "w");
    assert_true(fp != NULL, "failed to open temp fixture");
    if (!fp) {
        close(fd);
        unlink(path_template);
        return 1;
    }

    write_fixture_file(fp);
    fclose(fp);

    assert_true(
        phuck_off_parse_funcs_file(path_template, &files, &user_code_root, error, sizeof(error)),
        error
    );

    if (files && user_code_root) {
        assert_true(strcmp(user_code_root, "/tmp/user/code") == 0, "unexpected user_code_root");
        assert_true(files->size == 2049, "unexpected outer hash size");
        assert_true(files->slots == 1025, "outer hash did not resize as expected");

        assert_true(
            xdebug_hash_find(files, "/tmp/user/code/main.php", sizeof("/tmp/user/code/main.php") - 1, &value),
            "main.php missing from outer hash"
        );
        main_lines = (xdebug_hash*) value;
        assert_true(main_lines != NULL, "main.php should not be ignored");
        if (main_lines) {
            assert_true(main_lines->size == 17, "unexpected main.php inner hash size");
            assert_true(main_lines->slots == 9, "main.php inner hash did not resize as expected");

            assert_true(
                xdebug_hash_index_find(main_lines, 10, &value) && (unsigned long) (uintptr_t) value == 1,
                "main.php:10 has the wrong input line number"
            );
            assert_true(
                xdebug_hash_index_find(main_lines, 26, &value) && (unsigned long) (uintptr_t) value == 17,
                "main.php:26 has the wrong input line number"
            );
        }

        assert_true(
            xdebug_hash_find(files, "/tmp/user/code/ignored_later.php", sizeof("/tmp/user/code/ignored_later.php") - 1, &value),
            "ignored_later.php missing from outer hash"
        );
        assert_true(value == NULL, "ignored_later.php should map to NULL");

        assert_true(
            xdebug_hash_find(files, "/tmp/user/code/vendor/file_0000.php", sizeof("/tmp/user/code/vendor/file_0000.php") - 1, &value),
            "ignored vendor file missing from outer hash"
        );
        assert_true(value == NULL, "ignored vendor file should map to NULL");
    }

    free(user_code_root);
    if (files) {
        xdebug_hash_destroy(files);
    }
    unlink(path_template);

    if (failures) {
        return 1;
    }

    printf("ok\n");
    return 0;
}
