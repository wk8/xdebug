<?php

require_once __DIR__ . "/complex_runtime_lib.php";

function helper_alpha($value) {
    return $value + 1;
}

function helper_beta($value) {
    return Runner::static_bridge($value);
}

class Runner {
    public function run() {
        $seed = helper_alpha(10);
        $scoped = function ($value) {
            return $this->call_private($value);
        };

        $worker = new LibWorker();
        $box = new CallableBox();

        return $box($worker->invokeClosure($this->lib_wrap($scoped($seed))));
    }

    private function call_private($value) {
        return $value - 4;
    }

    protected function lib_wrap($value) {
        return lib_free((string) $value);
    }

    public static function static_bridge($value) {
        return LibWorker::boot($value);
    }
}

function main_entry() {
    return (new Runner())->run();
}
