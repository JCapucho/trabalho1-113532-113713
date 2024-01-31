#!/usr/bin/env bash

mkdir -p "$1"

echo "8x8"
./imageTool test/stripes8.pgm tic blur 1,1 toc save "$1/8x8-1x1.pgm"
./imageTool test/stripes8.pgm tic blur 3,3 toc save "$1/8x8-3x3.pgm"

echo "100x100"
./imageTool test/crop.pgm tic blur 5,5 toc save "$1/100x100-5x5.pgm"
./imageTool test/crop.pgm tic blur 10,10 toc save "$1/100x100-10x10.pgm"

echo "300x300"
./imageTool test/original.pgm tic blur 7,7 toc save "$1/300x300-7x7.pgm"
./imageTool test/original.pgm tic blur 15,15 toc save "$1/300x300-15x15.pgm"
