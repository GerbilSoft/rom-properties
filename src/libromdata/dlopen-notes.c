/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * dlopen-notes.c: dlopen() notes for dlopen()'d libraries.                *
 *                                                                         *
 * Copyright (c) 2024-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "dlopen-notes.h"

ELF_NOTE_DLOPEN2( \
	romdata_dlopen, \
	"lz4", "LZ4 decompression (for PSP CISOv2 and ZISO images)", "recommended", "liblz4.so.1", \
	"lzo", "LZO decompression (for PSP JISO images)", "recommended", "liblzo2.so.2"
);
