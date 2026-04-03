#!/bin/sh

set -eu

XDEBUG_PATH="$(php-config --extension-dir)/xdebug.so"

rm -f /tmp/phuck-off.log
rm -f /tmp/phuck_off_map_*

if ! PHUCK_OFF_ENABLED=1 \
    PHUCK_OFF_LOG_LEVEL=trace \
    PHUCK_OFF_SANITY_CHECK_SAMPLING=0 \
    php -n -dzend_extension="$XDEBUG_PATH" /app/phuck_off_tests/e2e/complex_runtime_driver.php
then
    if [ -f /tmp/phuck-off.log ]; then
        cat /tmp/phuck-off.log >&2
    fi
    exit 1
fi

if ! php -n /app/phuck_off_tests/e2e/validate_complex_runtime_log.php /tmp/phuck-off.log
then
    cat /tmp/phuck-off.log >&2
    exit 1
fi

echo "ok"
