#!/bin/bash

FS_PATH="${1:-build/specimen-fs.uf2}"

BUILD_DIR=build/yarg-stdlib
mkdir -p "$BUILD_DIR"

for target in cyarg yarg machine gpio irq repl pio-instructions vm ws2812
do
    echo "compiling $target..."
    bin/yarg compile --interpreter bin/cyarg --source "yarg/specimen/$target.ya" --output "$BUILD_DIR/$target.yb"
    bin/yarg cp -fs "$FS_PATH" -src "$BUILD_DIR/$target.yb" -dest "$target.yb"
done

for target in m0plus dma pio timer uart reset apa102 clock
do
    echo "adding $target..."
    bin/yarg cp -fs "$FS_PATH" -src "yarg/specimen/$target.ya" -dest "$target.ya"
done
