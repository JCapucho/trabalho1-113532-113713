#!/usr/bin/env bash

cmp2() {
	cmp first/$1 second/$1
}

cmp2 8x8-1x1.pgm
cmp2 8x8-3x3.pgm

cmp2 100x100-5x5.pgm
cmp2 100x100-10x10.pgm

cmp2 300x300-7x7.pgm
cmp2 300x300-15x15.pgm
