@echo off
waf configure -T debug --prefix=../game --disable-warns --build-games=hl2sbpp && ^
waf build install -p -vv --no-msvc-lazy -j4

pause