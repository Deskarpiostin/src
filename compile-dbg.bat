@echo off
waf configure -T debug --prefix=../game --disable-warns && ^
waf build install -p -vv --no-msvc-lazy -j4

pause