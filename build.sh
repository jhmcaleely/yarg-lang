#!/bin/bash

mkdir bin
cp tools/pico-reset bin/

pushd hostproto
go build .
popd

cp hostproto/hostproto bin/

mkdir cproto/build
pushd cproto/build
cmake ..
cmake --build .

popd

mkdir build
cp cproto/build/clox.uf2 build/proto-lang.uf2

./bin/hostproto format -fs build/proto-lang.uf2
