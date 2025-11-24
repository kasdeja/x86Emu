#!/bin/sh
mkdir -p _build-win-x64
cd _build-win-x64
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-win-x64.cmake ..
make -j 12
