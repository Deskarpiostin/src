#!/bin/sh
./waf configure -T release --use-ccache --build-games=hl2sbpp --64bits --prefix=../game --disable-warns
./waf build install -p -vv -j$(nproc)
