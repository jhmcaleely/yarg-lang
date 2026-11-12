#!/bin/sh

CYARG=./bin/cyarg

for file in \
conway-life-display/main \
conway-life-display/display \
conway-life-display/conway \
conway-life-display/cube_bit \
ws2812
do
    echo "Processing $file"
    $CYARG --disassemble "yarg/specimen/$file.ya" > yarg/specimen/$file.yd
done
