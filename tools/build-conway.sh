#!/bin/bash

HOSTYARG="../../../bin/hostyarg"
TARGETUF2="../../../build/conway.uf2"


pushd yarg/specimen/todo > /dev/null
if [ -e $TARGETUF2 ]
then
    rm $TARGETUF2
fi

cp ../../../build/yarg-lang.uf2 $TARGETUF2
$HOSTYARG format -fs $TARGETUF2
$HOSTYARG addfile -fs $TARGETUF2 -add conway.ya
$HOSTYARG addfile -fs $TARGETUF2 -add cube_bit.ya
$HOSTYARG addfile -fs $TARGETUF2 -add cube_life.ya
popd > /dev/null

pushd yarg/specimen
../../bin/hostyarg addfile -fs ../../build/conway.uf2 -add gpio.ya
../../bin/hostyarg addfile -fs ../../build/conway.uf2 -add machine.ya
popd > /dev/null
