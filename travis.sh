#!/bin/sh
RET=0
mkdir "${TRAVIS_BUILD_DIR}/build"
cd "${TRAVIS_BUILD_DIR}/build"
# NOTE: KF5 is not available on Ubuntu 14.04,
# so we can't build the KDE5 plugin.
cmake --version
cmake .. \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DENABLE_LTO=OFF \
	-DENABLE_JPEG=ON \
	-DBUILD_TESTING=ON \
	-DBUILD_KDE4=ON \
	-DBUILD_KDE5=OFF \
	-DBUILD_XFCE=ON \
	-DBUILD_GNOME=ON \
	|| exit 1
# Build everything.
make -k || RET=1
# Test with en_US.UTF8.
LC_ALL="en_US.UTF8" ctest -V || RET=1
# Test with fr_FR.UTF8 to find i18n issues.
LC_ALL="fr_FR.UTF8" ctest -V || RET=1
exit "${RET}"
