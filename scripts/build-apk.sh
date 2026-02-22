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
