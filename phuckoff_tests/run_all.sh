#!/bin/sh

set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
CC_BIN="${CC:-cc}"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/xdebug_fork_tests.XXXXXX")"

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

run_test "xdebug_hash_resize" "$ROOT/phuckoff_tests/xdebug_hash_resize.c" \
    "$ROOT/xdebug_hash.c" "$ROOT/xdebug_llist.c"
run_test "phuck_off_parser" "$ROOT/phuckoff_tests/phuck_off_parser.c" \
    "$ROOT/xdebug_hash.c" "$ROOT/xdebug_llist.c" "$ROOT/phuck_off_parser.c"
run_test "phuck_off_logger" "$ROOT/phuckoff_tests/phuck_off_logger.c" \
    -DPHUCK_OFF_STANDALONE_TEST "$ROOT/phuck_off_logger.c"

echo "all fork tests passed"
