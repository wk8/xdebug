<?php

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

echo "ok\n";
