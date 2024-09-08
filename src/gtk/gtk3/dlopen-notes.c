/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * dlopen-notes.c: dlopen() notes for dlopen()'d libraries.                *
 *                                                                         *
 * Copyright (c) 2024 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "dlopen-notes.h"

ELF_NOTE_DLOPEN4( \
	gtk3_dlopen, \
	"nautilus-extension", "Nautilus file browser integration", "recommended", "libnautilus-extension.so.1", \
	"caja-extension", "Caja file browser integration", "recommended", "libcaja-extension.so.1", \
	"nemo-extension", "Nemo file browser integration", "recommended", "libnemo-extension.so.1", \
	"ThunarX", "Thunar file browser integration", "recommended", "libthunarx-3.so.0" \
);
