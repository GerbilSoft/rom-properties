/***************************************************************************
 * ROM Properties Page shell extension. (GNOME Tracker)                    *
 * tracker-mini.h: Tracker function declarations and pointers              *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Tracker packages on most systems, including Ubuntu and Gentoo,
// do not install headers for libtracker-extract.
#pragma once

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "tracker-mini-1.0.h"
#include "tracker-mini-2.0.h"

G_BEGIN_DECLS

// Tracker API version in use.
// Currently, only Tracker 1.x is supported.
extern int rp_tracker_api;

/** Function pointer structs **/

typedef union _tracker_sparql_pfns_u {
	// NOTE: v2 *replaces* v1.
	tracker_sparql_1_0_pfns_t v1;
	tracker_sparql_2_0_pfns_t v2;
} tracker_sparql_pfns_u;
extern tracker_sparql_pfns_u tracker_sparql_pfns;

typedef struct _tracker_extract_pfns_t {
	// NOTE: v2 is an *extension* of v1.
	tracker_extract_1_0_pfns_t v1;
	tracker_extract_2_0_pfns_t v2;
} tracker_extract_pfns_t;
extern tracker_extract_pfns_t tracker_extract_pfns;

/** Management functions **/

/**
 * Initialize Tracker function pointers.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_tracker_init_pfn(void);

/**
 * Free Tracker function pointers.
 */
void rp_tracker_free_pfn(void);

G_END_DECLS
