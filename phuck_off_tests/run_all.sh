#!/bin/sh

set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
CC_BIN="${CC:-cc}"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/xdebug_fork_tests.XXXXXX")"
SHIMS_HEADER="$ROOT/phuck_off_tests/shims.h"

cleanup() {
    rm -rf "$BUILD_DIR"
}

trap cleanup EXIT

run_test() {
    name="$1"
    source="$2"
    binary="$BUILD_DIR/$name"
    shift 2

    echo "running $name"
    "$CC_BIN" "$@" -I"$ROOT" "$source" -o "$binary"
    "$binary"
}

run_test "xdebug_hash_resize" "$ROOT/phuck_off_tests/xdebug_hash_resize.c" \
    "$ROOT/xdebug_hash.c" "$ROOT/xdebug_llist.c"
run_test "phuck_off_parser" "$ROOT/phuck_off_tests/phuck_off_parser.c" \
    "$ROOT/xdebug_hash.c" "$ROOT/xdebug_llist.c" "$ROOT/phuck_off_parser.c"
run_test "phuck_off_logger" "$ROOT/phuck_off_tests/phuck_off_logger.c" \
    -include "$SHIMS_HEADER" "$ROOT/phuck_off_logger.c"
run_test "phuck_off_sanity_check" "$ROOT/phuck_off_tests/phuck_off_sanity_check.c" \
    "$ROOT/phuck_off_sanity_check.c" "$ROOT/phuck_off_logger.c"
run_test "phuck_off_mmap" "$ROOT/phuck_off_tests/phuck_off_mmap.c" \
    "$ROOT/phuck_off_mmap.c" "$ROOT/phuck_off_logger.c"
run_test "phuck_off_function_id" "$ROOT/phuck_off_tests/phuck_off_function_id.c" \
    -include "$SHIMS_HEADER" "$ROOT/xdebug_hash.c" "$ROOT/xdebug_llist.c" "$ROOT/phuck_off_parser.c" "$ROOT/phuck_off_logger.c" "$ROOT/phuck_off_mmap.c" "$ROOT/phuck_off_sanity_check.c"
run_test "phuck_off_process_stackframe" "$ROOT/phuck_off_tests/phuck_off_process_stackframe.c" \
    -include "$SHIMS_HEADER" "$ROOT/xdebug_hash.c" "$ROOT/xdebug_llist.c" "$ROOT/phuck_off_parser.c" "$ROOT/phuck_off_logger.c" "$ROOT/phuck_off_mmap.c" "$ROOT/phuck_off_sanity_check.c"
run_test "phuck_off_parser_lookup" "$ROOT/phuck_off_tests/phuck_off_parser_lookup.c" \
    -include "$SHIMS_HEADER" "$ROOT/xdebug_hash.c" "$ROOT/xdebug_llist.c" "$ROOT/phuck_off_parser.c" "$ROOT/phuck_off_logger.c" "$ROOT/phuck_off_mmap.c" "$ROOT/phuck_off_sanity_check.c"

echo "all fork tests passed"
