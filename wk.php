<?php
// xdebug-test.php
// PHP 5.5 compatible

function start()
{
    logMessage("Starting");
    stepOne();
}

function stepOne()
{
    logMessage("Step one");
    stepTwo();
}

function stepTwo()
{
    logMessage("Step two");
    $x = array("wk" => "po");
    stepThree();
}

function stepThree()
{
    logMessage("Step three");
    stepFour();
}

function stepFour()
{
    logMessage("Final step");
}

function logMessage($msg)
{
    echo $msg . PHP_EOL;
}

// function main() {
//   start();
//   start();
// }
//
// main();
