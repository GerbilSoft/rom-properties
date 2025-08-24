/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * dlopen-notes.c: dlopen() notes for dlopen()'d libraries.                *
 *                                                                         *
 * Copyright (c) 2024-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "dlopen-notes.h"

ELF_NOTE_DLOPEN( \
	rp_download_dlopen, \
	"libcurl", "libcurl for downloading external images from online databases", "required", "libcurl.so.4"
);
