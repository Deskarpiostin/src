#!/bin/sh
./waf configure -T release --use-ccache --enable-opus --64bits --prefix=../game --disable-warns
./waf build install -p -vv -j$(nproc)
