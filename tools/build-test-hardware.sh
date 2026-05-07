#!/bin/bash

HOSTYARG="../../bin/yarg"
TARGETUF2="../../build/test-hardware.uf2"


pushd test/hardware > /dev/null
if [ -e $TARGETUF2 ]
then
    rm $TARGETUF2
fi

cp ../../build/yarg-lang.uf2 $TARGETUF2
for test in cheese.ya scone.ya interrupt.ya alarm.ya blinky.ya main.ya coroutine-flash.ya multicore-flash.ya timed-flash.ya
do
    $HOSTYARG compile --interpreter ../../bin/cyarg --source $test
    $HOSTYARG cp -fs $TARGETUF2 -src $test -dest $test
done

$HOSTYARG compile --interpreter ../../bin/cyarg --source ../../yarg/specimen/hello_led.ya
$HOSTYARG cp -fs $TARGETUF2 -src ../../yarg/specimen/hello_led.ya -dest hello_led.ya

$HOSTYARG compile --interpreter ../../bin/cyarg --source ../../yarg/specimen/hello_button.ya
$HOSTYARG cp -fs $TARGETUF2 -src ../../yarg/specimen/hello_button.ya -dest hello_button.ya
popd > /dev/null
