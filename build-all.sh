#!/bin/bash

platforms="linux win64 osx64"

declare -A ccmap

ccmap[linux]=x86_64-linux-gnu-gcc
ccmap[win64]=x86_64-w64-mingw32-gcc
ccmap[osx64]=x86_64-apple-darwin15-clang

EXTRA_CFLAGS="-lm -lz"

for p in $platforms; do
	make clean

	CC=${ccmap[$p]}

	if [[ "$CC" == *gcc ]]; then
		CFLAGS=$EXTRA_CFLAGS
	else
		CFLAGS=''
	fi

	CC=$CC CFLAGS=$CFLAGS EXTRA_CFLAGS=$EXTRA_CFLAGS make
	mv liconvert liconvert-$p
done
