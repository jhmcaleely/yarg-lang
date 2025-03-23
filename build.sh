#!/bin/bash

mkdir bin
cp tools/pico-reset bin/

pushd hostproto
go build .
popd

cp hostproto/hostproto bin/

pushd cproto
cmake --preset pico .
cmake --build build/pico

cmake --preset host
cmake --build build/host
popd

cp cproto/build/host/clox bin/

mkdir build
cp cproto/build/pico/clox.uf2 build/proto-lang.uf2

./bin/hostproto format -fs build/proto-lang.uf2

pushd yarg/specimen
../../bin/hostproto addfile -fs ../../build/proto-lang.uf2 -add machine.lox
../../bin/hostproto addfile -fs ../../build/proto-lang.uf2 -add gpio.lox
