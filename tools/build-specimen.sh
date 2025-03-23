#!/bin/bash

pushd yarg/specimen
rm ../../build/specimen.uf2
../../bin/hostyarg format -fs ../../build/specimen.uf2
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add alarm.ya
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add blinky.ya
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add broken.ya
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add closure.ya
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add coffee.ya
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add coroutine-flash.ya
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add coroutine.ya
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add fibonacci.ya
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add gpio.ya
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add machine.ya
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add multicore-flash.ya
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add scone.ya
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add timed-flash.ya
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add toy.ya
popd

