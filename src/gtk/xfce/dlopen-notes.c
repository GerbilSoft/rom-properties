/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 2.x)                         *
 * dlopen-notes.c: dlopen() notes for dlopen()'d libraries.                *
 *                                                                         *
 * Copyright (c) 2024 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "dlopen-notes.h"

ELF_NOTE_DLOPEN( \
	xfce_dlopen, \
	"ThunarX", "Thunar file browser integration", "recommended", "libthunarx-2.so.0" \
);
