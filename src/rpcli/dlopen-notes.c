/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * dlopen-notes.c: dlopen() notes for dlopen()'d libraries.                *
 *                                                                         *
 * Copyright (c) 2024-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "dlopen-notes.h"
#include "dll-macros.h"	// for RP_LIBRARY_SO_VERSIONED()

ELF_NOTE_DLOPEN( \
	rpcli_dlopen, \
	"libsixel", "libsixel for rendering graphics in Sixel-capable terminals", "recommended", RP_LIBRARY_SO_VERSIONED("libsixel.so", ".1")
);
