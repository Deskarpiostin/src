#!/bin/sh
git pull
git submodule init && git submodule update
sudo apt-get update
sudo apt-get install -f -y libopenal-dev g++-multilib gcc-multilib libpng-dev libjpeg-dev libfreetype6-dev libfontconfig1-dev libcurl4-gnutls-dev libsdl2-dev zlib1g-dev libbz2-dev libedit-dev automake autoconf libtool

git clone --recursive --depth 1 https://github.com/xiph/opus
cd opus
./autogen.sh && ./configure --enable-custom-modes && make -j$(nproc) && sudo make install
cd ..

./waf configure -T release --prefix=./to-upload --enable-opus --64bits --disable-warns $* &&
./waf install -j$(nproc)
