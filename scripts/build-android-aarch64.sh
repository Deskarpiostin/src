#!/bin/sh
wget -q --retry-connrefused --tries=3 --timeout=30 \
  -O android-ndk-r10e.zip \
  https://dl.google.com/android/repository/android-ndk-r10e-linux-x86_64.zip

wget -q --retry-connrefused --tries=3 --timeout=30 \
  -O clang+llvm-11.1.0.tar.xz \
  https://github.com/llvm/llvm-project/releases/download/llvmorg-11.1.0/clang+llvm-11.1.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz

if [ ! -d "android-ndk-r10e" ]; then
    unzip -q android-ndk-r10e.zip
fi

if [ ! -d "clang+llvm-11.1.0-x86_64-linux-gnu-ubuntu-16.04" ]; then
    tar -xf clang+llvm-11.1.0.tar.xz
fi

sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install -y aptitude
sudo aptitude install -y zlib1g:i386 libc6:i386 libncurses5:i386 libstdc++6:i386 libopenal-dev:i386 g++-multilib gcc-multilib libpng-dev:i386 libjpeg-dev:i386 libfreetype6-dev:i386 libfontconfig1-dev:i386 libcurl4-gnutls-dev:i386 libsdl2-dev:i386 zlib1g-dev:i386 libbz2-dev:i386 libedit-dev:i386
sudo apt-get install zlib1g:i386 zlib1g

export ANDROID_BUILD_TOOLS_VERSION=29.0.3
export ANDROID_SDK_ROOT="/usr/local/lib/android/sdk"
export ANDROID_NDK_HOME="$PWD/android-ndk-r10e"
export PATH="$ANDROID_NDK_HOME:$PATH"
export PATH="$PWD/clang+llvm-11.1.0-x86_64-linux-gnu-ubuntu-16.04/bin:$PATH"
export PATH="$ANDROID_SDK_ROOT/build-tools/29.0.3:$PATH"

./waf configure -T release --prefix=srceng-mod-launcher/android --togles --android=aarch64,host,21 --target=../aarch64 -8 --disable-warns &&
./waf install --target=client,server,GameUI,engine,studiorender,matsys_controls -j$(nproc)
