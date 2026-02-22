#!/bin/sh
./waf configure -T debug --use-ccache --build-games=hl2sbpp --64bits --prefix=../game --disable-warns #--sanitize=address,undefined
./waf build install -p -vv -j$(nproc)
