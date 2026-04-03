<?php

require __DIR__ . '/../fixtures/complex_runtime_autoload.php';
require __DIR__ . '/../fixtures/complex_runtime_config.php';
require __DIR__ . '/../fixtures/complex_runtime_messages_en.php';
require __DIR__ . '/../fixtures/complex_runtime_main.php';

$worker = new LibWorker();
$trait = $worker->trait_step(4);
$beta = helper_beta(3);
$result = main_entry();

if ($trait !== 6) {
    fwrite(STDERR, "unexpected trait result\n");
    exit(1);
}

if ($beta !== 10) {
    fwrite(STDERR, "unexpected helper_beta result\n");
    exit(1);
}

if ($result !== "7!?") {
    fwrite(STDERR, "unexpected main_entry result\n");
    exit(1);
}

if (empty($GLOBALS['complex_runtime_autoload_loaded'])) {
    fwrite(STDERR, "autoload fixture did not run\n");
    exit(1);
}

if (!isset($GLOBALS['complex_runtime_config']['env']) || $GLOBALS['complex_runtime_config']['env'] !== 'test') {
    fwrite(STDERR, "config fixture did not run\n");
    exit(1);
}

if (!isset($GLOBALS['complex_runtime_messages']['hello']) || $GLOBALS['complex_runtime_messages']['hello'] !== 'world') {
    fwrite(STDERR, "messages fixture did not run\n");
    exit(1);
}

echo "ok\n";
