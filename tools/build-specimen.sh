#!/bin/bash

pushd yarg/specimen
rm ../../build/specimen.uf2
../../bin/hostyarg format -fs ../../build/specimen.uf2
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add alarm.lox
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add blinky.lox
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add broken.lox
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add closure.lox
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add coffee.lox
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add coroutine-flash.lox
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add coroutine.lox
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add fibonacci.lox
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add gpio.lox
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add machine.lox
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add multicore-flash.lox
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add scone.lox
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add timed-flash.lox
../../bin/hostyarg addfile -fs ../../build/specimen.uf2 -add toy.lox
popd

