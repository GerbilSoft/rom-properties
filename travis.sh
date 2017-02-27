#!/bin/sh
RET=0
mkdir "${TRAVIS_BUILD_DIR}/build"
cd "${TRAVIS_BUILD_DIR}/build"
# NOTE: KF5 is not available on Ubuntu 14.04,
# so we can't build the KDE5 plugin.
cmake .. \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DENABLE_JPEG=ON \
	-DBUILD_TESTING=ON \
	-DBUILD_KDE4=ON \
	-DBUILD_KDE5=OFF \
	-DBUILD_XFCE=ON \
	-DBUILD_GNOME=ON \
	|| exit 1
# Build UTF-8 and UTF-16 libraries for testing purposes.
make romdata8 romdata16 cachemgr8 cachemgr16 || RET=1
# Build the actual plugin(s).
make || RET=1
ctest -V || RET=1
exit "${RET}"
