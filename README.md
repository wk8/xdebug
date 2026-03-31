# xdebug fork

This is our own fork of [xdebug](https://github.com/xdebug/xdebug).

It's based off [version 2.5](https://github.com/xdebug/xdebug/tree/xdebug_2_5), which is the last version that supports PHP 5.6.

## Relevant branches

* `xdebug_2_5` is the historical 2.5 branch
* `main` is our main branch, that adds our fork on top of 2.5

An easy way to see what our fork has added on top of 2.5: https://github.com/wk8/xdebug/compare/xdebug_2_5...wk8:xdebug:main

## Tests:

```bash
./phuck_off_tests/run_all.sh
```