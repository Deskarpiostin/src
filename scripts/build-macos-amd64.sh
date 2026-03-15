#!/bin/sh

git submodule init && git submodule update

brew install sdl2
NUM_CORES=$(sysctl -n hw.ncpu)

./waf configure -T debug --64bits --disable-warns --prefix=./to-upload $* &&
./waf install -j$NUM_CORES
