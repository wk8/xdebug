<?php

$runtimeRoot = '/app/phuck_off_tests/fixtures';
$logPath = isset($argv[1]) ? $argv[1] : '/tmp/phuck-off.log';
$log = @file_get_contents($logPath);

if ($log === false) {
    fwrite(STDERR, "failed to read phuck-off log\n");
    exit(1);
}

$expectedLines = array(
    'Frame calling user function trait_step at ' . $runtimeRoot . '/complex_runtime_lib.php:14',
    'Frame calling user function helper_beta at ' . $runtimeRoot . '/complex_runtime_main.php:9',
    'Frame calling user function static_bridge at ' . $runtimeRoot . '/complex_runtime_main.php:34',
    'Frame calling user function boot at ' . $runtimeRoot . '/complex_runtime_lib.php:22',
    'Frame calling user function hidden at ' . $runtimeRoot . '/complex_runtime_lib.php:26',
    'Frame calling user function main_entry at ' . $runtimeRoot . '/complex_runtime_main.php:39',
    'Frame calling user function run at ' . $runtimeRoot . '/complex_runtime_main.php:14',
    'Frame calling user function helper_alpha at ' . $runtimeRoot . '/complex_runtime_main.php:5',
    'Frame calling user function {closure} at ' . $runtimeRoot . '/complex_runtime_main.php:16',
    'Frame calling user function call_private at ' . $runtimeRoot . '/complex_runtime_main.php:26',
    'Frame calling user function lib_wrap at ' . $runtimeRoot . '/complex_runtime_main.php:30',
    'Frame calling user function lib_free at ' . $runtimeRoot . '/complex_runtime_lib.php:3',
    'Frame calling user function invokeClosure at ' . $runtimeRoot . '/complex_runtime_lib.php:30',
    'Frame calling user function make_lib_closure at ' . $runtimeRoot . '/complex_runtime_lib.php:7',
    'Frame calling user function {closure} at ' . $runtimeRoot . '/complex_runtime_lib.php:8',
    'Frame calling user function lib_free at ' . $runtimeRoot . '/complex_runtime_lib.php:3',
    'Frame calling user function __invoke at ' . $runtimeRoot . '/complex_runtime_lib.php:37',
);

$unexpectedLines = array(
    'Frame calling user function ' . $runtimeRoot . '/complex_runtime_autoload.php:',
    'Frame calling user function ' . $runtimeRoot . '/complex_runtime_config.php:',
    'Frame calling user function ' . $runtimeRoot . '/complex_runtime_messages_en.php:',
    ' at ' . $runtimeRoot . '/complex_runtime_autoload.php:',
    ' at ' . $runtimeRoot . '/complex_runtime_config.php:',
    ' at ' . $runtimeRoot . '/complex_runtime_messages_en.php:',
    'No function map entry for path "' . $runtimeRoot . '/complex_runtime_autoload.php":',
    'No function map entry for path "' . $runtimeRoot . '/complex_runtime_config.php":',
    'No function map entry for path "' . $runtimeRoot . '/complex_runtime_messages_en.php":',
    'No function map entry for path "' . $runtimeRoot . '/complex_runtime_main.php":',
    'No function map entry for path "' . $runtimeRoot . '/complex_runtime_lib.php":',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_lib.php:14',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_main.php:9',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_main.php:34',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_lib.php:22',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_lib.php:26',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_main.php:39',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_main.php:14',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_main.php:5',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_main.php:16',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_main.php:26',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_main.php:30',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_lib.php:3',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_lib.php:30',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_lib.php:7',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_lib.php:8',
    'No function id entry for "' . $runtimeRoot . '/complex_runtime_lib.php:37',
    'Cache error!!',
);

$offset = 0;
foreach ($expectedLines as $expectedLine) {
    $position = strpos($log, $expectedLine, $offset);

    if ($position === false) {
        fwrite(STDERR, "missing expected log line:\n" . $expectedLine . "\n");
        exit(1);
    }

    $offset = $position + strlen($expectedLine);
}

foreach ($unexpectedLines as $unexpectedLine) {
    if (strpos($log, $unexpectedLine) !== false) {
        fwrite(STDERR, "unexpected log line present:\n" . $unexpectedLine . "\n");
        exit(1);
    }
}

echo "ok\n";
