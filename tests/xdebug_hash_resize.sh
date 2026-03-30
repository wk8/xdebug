#!/bin/sh

set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
CC_BIN="${CC:-cc}"
SOURCE="$ROOT/tests/xdebug_hash_resize.c"
BINARY="$(mktemp "${TMPDIR:-/tmp}/xdebug_hash_resize.XXXXXX")"

cleanup() {
	rm -f "$BINARY"
}

trap cleanup EXIT

"$CC_BIN" -I"$ROOT" "$SOURCE" "$ROOT/xdebug_hash.c" "$ROOT/xdebug_llist.c" -o "$BINARY"
"$BINARY"
