#!/bin/sh
RET=0
mkdir "${TRAVIS_BUILD_DIR}/build"
cd "${TRAVIS_BUILD_DIR}/build"
cmake .. \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DBUILD_TESTING=ON\
	|| exit 1
# Build UTF-8 and UTF-16 libraries for testing purposes.
make romdata8 romdata16 cachemgr8 cachemgr16 || RET=1
# Build the actual plugin(s).
make || RET=1
ctest -V || RET=1
exit "${RET}"
