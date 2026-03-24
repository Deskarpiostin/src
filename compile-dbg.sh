#!/bin/sh
./waf configure -T debug --use-ccache --enable-opus --64bits --prefix=../game --disable-warns #--sanitize=address,undefined
./waf build install -p -vv -j$(nproc)
