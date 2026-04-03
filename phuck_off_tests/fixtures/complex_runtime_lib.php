<?php

function lib_free($value) {
    return strtoupper($value);
}

function make_lib_closure($suffix) {
    return function ($value) use ($suffix) {
        return lib_free($value) . $suffix;
    };
}

trait LibTrait {
    public function trait_step($value) {
        return $value + 2;
    }
}

class LibWorker {
    use LibTrait;

    public static function boot($value) {
        return self::hidden($value) + 1;
    }

    private static function hidden($value) {
        return $value * 3;
    }

    public function invokeClosure($value) {
        $formatter = make_lib_closure("!");
        return $formatter($value);
    }
}

class CallableBox {
    public function __invoke($value) {
        return $value . "?";
    }
}
