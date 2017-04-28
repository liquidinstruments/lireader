#!/bin/bash

platforms="linux win64 osx64"

declare -A ccmap

ccmap[linux]=x86_64-linux-gnu-gcc
ccmap[win64]=x86_64-w64-mingw32-gcc
ccmap[osx64]=x86_64-apple-darwin15-gcc

for p in $platforms; do
	make clean

	CC=${ccmap[$p]}

	# Statically link zlib on Windows so we don't have to ship a zlib.dll
	# Don't statically link on other platforms as Mac doesn't have static
	# libz (and linux could go either way)
	if [[ "$CC" == *mingw* ]]; then
		CFLAGS="-l:libz.a -lm"
	else
		CFLAGS="-lz -lm"
	fi

	if [[ "$p" == win* ]]; then
		SUFFIX=".exe"
	else
		SUFFIX=""
	fi

	CC=$CC CFLAGS=$CFLAGS make
	mv liconvert liconvert-$p$SUFFIX
done
