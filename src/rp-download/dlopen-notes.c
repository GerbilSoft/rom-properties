/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * dlopen-notes.c: dlopen() notes for dlopen()'d libraries.                *
 *                                                                         *
 * Copyright (c) 2024-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "dlopen-notes.h"
#include "dll-macros.h"	// for RP_LIBRARY_SO_VERSIONED()

ELF_NOTE_DLOPEN( \
	rp_download_dlopen, \
	"libcurl", "libcurl for downloading external images from online databases", "required", RP_LIBRARY_SO_VERSIONED("libcurl.so", ".4")
);
