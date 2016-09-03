#!/bin/sh
RET=0
mkdir "${TRAVIS_BUILD_DIR}/build"
cd "${TRAVIS_BUILD_DIR}/build"
cmake .. \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DBUILD_TESTING=ON\
	|| exit 1
make || RET=1
CTEST_OUTPUT_ON_FAILURE=1 make test || RET=1
exit "${RET}"
