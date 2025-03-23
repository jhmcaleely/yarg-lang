#!/bin/bash

mkdir bin
cp tools/pico-reset bin/

pushd hostproto
go build .
popd

cp hostproto/hostproto bin/

pushd cyarg
cmake --preset pico .
cmake --build build/pico

cmake --preset host
cmake --build build/host
popd

cp cyarg/build/host/cyarg bin/

mkdir build
cp cyarg/build/pico/cyarg.uf2 build/proto-lang.uf2

./bin/hostproto format -fs build/proto-lang.uf2

pushd yarg/specimen
../../bin/hostproto addfile -fs ../../build/proto-lang.uf2 -add machine.lox
../../bin/hostproto addfile -fs ../../build/proto-lang.uf2 -add gpio.lox
