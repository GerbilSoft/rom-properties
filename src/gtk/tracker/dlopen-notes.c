/***************************************************************************
 * ROM Properties Page shell extension. (GNOME Tracker)                    *
 * dlopen-notes.c: dlopen() notes for dlopen()'d libraries.                *
 *                                                                         *
 * Copyright (c) 2024 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "dlopen-notes.h"

ELF_NOTE_DLOPEN8( \
	tracker_dlopen, \
	"localsearch-3.0", "Support for the GNOME LocalSearch 3.0 API", "recommended", "libtinysparql-3.0.so.0", \
	"localsearch-3.0", "Support for the GNOME LocalSearch 3.0 API", "recommended", "libtracker-extract.so", \
	"tracker-3.0", "Support for the GNOME Tracker 3.0 API", "recommended", "libtracker-sparql-3.0.so.0", \
	"tracker-3.0", "Support for the GNOME Tracker 3.0 API", "recommended", "libtracker-extract.so", \
	"tracker-2.0", "Support for the GNOME Tracker 2.0 API", "recommended", "libtracker-sparql-2.0.so.0", \
	"tracker-2.0", "Support for the GNOME Tracker 2.0 API", "recommended", "libtracker-extract.so.0", \
	"tracker-1.0", "Support for the GNOME Tracker 1.0 API", "recommended", "libtracker-sparql-1.0.so.0", \
	"tracker-1.0", "Support for the GNOME Tracker 1.0 API", "recommended", "libtracker-extract.so.0" \
);
