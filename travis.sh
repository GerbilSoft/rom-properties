#!/bin/sh
RET=0
mkdir "${TRAVIS_BUILD_DIR}/build"
cd "${TRAVIS_BUILD_DIR}/build"
cmake --version

# Initial build with optional components disabled.
case "$OSTYPE" in
	darwin*)
		# Mac OS X. Disable gettext for now.
		# Also disable split debug due to lack of `objcopy`.
		cmake .. \
			-DCMAKE_INSTALL_PREFIX=/usr \
			-DSPLIT_DEBUG=OFF \
			-DENABLE_LTO=OFF \
			-DENABLE_PCH=ON \
			-DBUILD_TESTING=ON \
			-DENABLE_NLS=OFF \
			\
			-DENABLE_EXTRA_SECURITY=OFF \
			-DENABLE_JPEG=OFF \
			-DENABLE_XML=OFF \
			-DENABLE_DECRYPTION=OFF \
			-DENABLE_UNICE68=OFF \
			-DENABLE_LIBMSPACK=OFF \
			-DENABLE_PVRTC=OFF \
			-DENABLE_ZSTD=OFF \
			-DENABLE_LZ4=OFF \
			-DENABLE_LZO=OFF \
			|| exit 1
		;;
	*)
		# Linux. Enable all UI frontends.
		# NOTE: KF5 is not available on Ubuntu 14.04,
		# so we can't build the KF5 plugin.
		cmake .. \
			-DCMAKE_INSTALL_PREFIX=/usr \
			-DENABLE_LTO=OFF \
			-DENABLE_PCH=ON \
			-DBUILD_TESTING=ON \
			-DENABLE_NLS=ON \
			-DBUILD_KDE4=ON \
			-DBUILD_KDE5=OFF \
			-DBUILD_XFCE=ON \
			-DBUILD_GTK3=ON \
			\
			-DENABLE_EXTRA_SECURITY=OFF \
			-DENABLE_JPEG=OFF \
			-DENABLE_XML=OFF \
			-DENABLE_DECRYPTION=OFF \
			-DENABLE_UNICE68=OFF \
			-DENABLE_LIBMSPACK=OFF \
			-DENABLE_PVRTC=OFF \
			-DENABLE_ZSTD=OFF \
			-DENABLE_LZ4=OFF \
			-DENABLE_LZO=OFF \
			|| exit 1
esac

# Build everything.
make -k || RET=1
# Test with en_US.UTF8.
LC_ALL="en_US.UTF8" ctest -V || RET=1
# Test with fr_FR.UTF8 to find i18n issues.
LC_ALL="fr_FR.UTF8" ctest -V || RET=1

# Second build with optional components enabled.
cmake .. \
	-DENABLE_EXTRA_SECURITY=ON \
	-DENABLE_JPEG=ON \
	-DENABLE_XML=ON \
	-DENABLE_DECRYPTION=ON \
	-DENABLE_UNICE68=ON \
	-DENABLE_LIBMSPACK=ON \
	-DENABLE_PVRTC=ON \
	-DENABLE_ZSTD=ON \
	-DENABLE_LZ4=ON \
	-DENABLE_LZO=ON \
	|| exit 1

# Build everything.
make -k || RET=1
# Test with en_US.UTF8.
LC_ALL="en_US.UTF8" ctest -V || RET=1
# Test with fr_FR.UTF8 to find i18n issues.
LC_ALL="fr_FR.UTF8" ctest -V || RET=1

# All done!
exit "${RET}"
