#!/bin/sh
rm -rf release
mkdir release

rm -rf build
scons --force-mingw32
mv build/tapcfg.dll release/tapcfg32.dll
i586-mingw32msvc-strip release/tapcfg32.dll
rm -rf build
scons --force-mingw64
mv build/tapcfg.dll release/tapcfg64.dll
amd64-mingw32msvc-strip release/tapcfg64.dll

rm -rf build
scons --force-32bit
mv build/libtapcfg.so release/libtapcfg32.so
strip release/libtapcfg32.so
rm -rf build
scons --force-64bit
mv build/libtapcfg.so release/libtapcfg64.so
strip release/libtapcfg64.so

rm -rf build
scons
mv build/TAPNet.dll release/TAPNet.dll
