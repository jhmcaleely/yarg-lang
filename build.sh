#!/bin/bash

mkdir bin
cp tools/pico-reset bin/

pushd hostyarg
go build .
popd

cp hostyarg/hostyarg bin/

pushd cyarg
cmake --preset pico .
cmake --build build/pico

cmake --preset host
cmake --build build/host
popd

cp cyarg/build/host/cyarg bin/

mkdir build
cp cyarg/build/pico/cyarg.uf2 build/yarg-lang.uf2

./bin/hostyarg format -fs build/yarg-lang.uf2

pushd yarg/specimen
../../bin/hostyarg addfile -fs ../../build/yarg-lang.uf2 -add machine.lox
../../bin/hostyarg addfile -fs ../../build/yarg-lang.uf2 -add gpio.lox
