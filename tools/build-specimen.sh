#!/bin/bash

pushd yarg/specimen
rm ../../build/specimen.uf2
../../bin/hostproto format -fs ../../build/specimen.uf2
../../bin/hostproto addfile -fs ../../build/specimen.uf2 -add alarm.lox
../../bin/hostproto addfile -fs ../../build/specimen.uf2 -add blinky.lox
../../bin/hostproto addfile -fs ../../build/specimen.uf2 -add broken.lox
../../bin/hostproto addfile -fs ../../build/specimen.uf2 -add closure.lox
../../bin/hostproto addfile -fs ../../build/specimen.uf2 -add coffee.lox
../../bin/hostproto addfile -fs ../../build/specimen.uf2 -add coroutine-flash.lox
../../bin/hostproto addfile -fs ../../build/specimen.uf2 -add coroutine.lox
../../bin/hostproto addfile -fs ../../build/specimen.uf2 -add fibonacci.lox
../../bin/hostproto addfile -fs ../../build/specimen.uf2 -add gpio.lox
../../bin/hostproto addfile -fs ../../build/specimen.uf2 -add machine.lox
../../bin/hostproto addfile -fs ../../build/specimen.uf2 -add multicore-flash.lox
../../bin/hostproto addfile -fs ../../build/specimen.uf2 -add scone.lox
../../bin/hostproto addfile -fs ../../build/specimen.uf2 -add timed-flash.lox
../../bin/hostproto addfile -fs ../../build/specimen.uf2 -add toy.lox
popd

