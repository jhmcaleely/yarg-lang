#!/bin/bash

# a specimen script to get a vanilla Ubuntu 25.10 VM ready for building
# You may prefer an alternative approach.
# Note that the C++ in cyarg/test-system requires GCC 14 or later. 15 appears
# to be the default in Ubuntu 25.10.

sudo apt -y install build-essential
sudo apt -y install golang cmake ninja-build

if [ "$1" = "pico" ]; then
    echo "Setting up for Pico development"

    mkdir -p pico-tooling
    pushd pico-tooling
    wget https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz
    tar xf arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz

    export PATH="$(pwd)/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin:$PATH"

    git clone --recurse-submodules https://github.com/raspberrypi/pico-sdk.git \
        --branch 2.2.0 --single-branch --depth 1

    export PICO_SDK_PATH="$(pwd)/pico-sdk"

    git clone https://github.com/raspberrypi/picotool.git \
              --branch 2.2.0-a4 --single-branch --depth 1
    cd picotool
    mkdir build
    cd build
    cmake ..
    cmake --build . --parallel $(nproc)

    sudo cmake --install .
fi