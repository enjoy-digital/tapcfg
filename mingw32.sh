
export AR=i586-mingw32msvc-ar
export RANLIB=i586-mingw32msvc-ranlib
export CC=i586-mingw32msvc-gcc
export CPP=i586-mingw32msvc-g++
export CXX=i586-mingw32msvc-g++
export LINK_CC=i586-mingw32msvc-gcc
export LINK_CXX=i586-mingw32msvc-g++
export WINRC=i586-mingw32msvc-windres

./waf clean
./waf configure --nocache --conf-prefix="/usr/i586-mingw32msvc" --prefix=windows/
./waf
./waf install
