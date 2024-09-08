/***************************************************************************
 * ROM Properties Page shell extension. (GTK4)                             *
 * dlopen-notes.c: dlopen() notes for dlopen()'d libraries.                *
 *                                                                         *
 * Copyright (c) 2024 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "dlopen-notes.h"

ELF_NOTE_DLOPEN( \
	gtk4_dlopen, \
	"nautilus-extension", "Nautilus file browser integration", "recommended", "libnautilus-extension.so.4" \
);
