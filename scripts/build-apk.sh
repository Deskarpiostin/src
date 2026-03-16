#!/bin/bash

sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install -y aptitude
sudo aptitude install -y zlib1g:i386 libc6:i386 libncurses5:i386 libstdc++6:i386 libopenal-dev:i386 g++-multilib gcc-multilib libpng-dev:i386 libjpeg-dev:i386 libfreetype6-dev:i386 libfontconfig1-dev:i386 libcurl4-gnutls-dev:i386 libsdl2-dev:i386 zlib1g-dev:i386 libbz2-dev:i386 libedit-dev:i386
sudo apt-get install zlib1g:i386 zlib1g

cd srceng-mod-launcher

export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
export PATH=$JAVA_HOME/bin:$PATH

git clone https://gitlab.com/LostGamer/android-sdk
export ANDROID_SDK_HOME=$PWD/android-sdk
export MODNAME=hl2sbpp
export MODNAMESTRING="Half-Life 2: Sandbox++"
git pull
# sudo apt install -y imagemagick
# ./mod.sh
# wget https://raw.githubusercontent.com/ItzVladik/extras/main/mi_logo.png
# mv mi_logo.png android/
# ./android/scripts/conv.sh android/mi_logo.png
# cp -r res android/
if [ -d build ]; then
    ./waf clean
fi
./waf configure -T release &&
./waf build
