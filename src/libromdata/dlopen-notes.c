/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * dlopen-notes.c: dlopen() notes for dlopen()'d libraries.                *
 *                                                                         *
 * Copyright (c) 2024-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "dlopen-notes.h"

// NOTE: libpng.so isn't listed here, even though it's dlopen()'d,
// because it's also linked normally for regular PNG functionality.
// dlopen() is only used for APNG functionality.

// NOTE: The WebP functions we're using are identical across
// the following SOVERSIONs:
//
// - 5 (libwebp-0.4.4-1, Xubuntu 16.04)
// - 6 (not tested, but should be the same)
// - 7 (1.5.0, Gentoo Linux)
//
// Ubuntu systems don't have the unversioned .so if the -dev package
// isn't installed, so all tested versions are listed.

#define ELF_NOTE_DLOPEN3_THREESO0(var, feature0, description0, priority0, module0a, module0b, module0c, feature1, description1, priority1, module1, feature2, description2, priority2, module2) \
	_ELF_NOTE_DLOPEN("[" _ELF_NOTE_DLOPEN_VA_INT(feature0, description0, priority0, module0a, module0b, module0c) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature1, description1, priority1, module1) "," \
	                     _ELF_NOTE_DLOPEN_INT(feature2, description2, priority2, module2) "]", var)

ELF_NOTE_DLOPEN3_THREESO0( \
	romdata_dlopen, \
	"webp", "WebP image decoding (for Android APK packages)", "recommended", "libwebp.so.7", "libwebp.so.6", "libwebp.so.5", \
	"lz4", "LZ4 decompression (for PSP CISOv2 and ZISO images)", "recommended", "liblz4.so.1", \
	"lzo", "LZO decompression (for PSP JISO images)", "recommended", "liblzo2.so.2"
);
