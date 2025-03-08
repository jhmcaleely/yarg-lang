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

popd

mkdir build
cp cproto/build/pico/clox.uf2 build/proto-lang.uf2

./bin/hostproto format -fs build/proto-lang.uf2
