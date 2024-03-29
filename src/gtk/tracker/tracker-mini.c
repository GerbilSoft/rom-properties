/***************************************************************************
 * ROM Properties Page shell extension. (GNOME Tracker)                    *
 * tracker-mini.c: Tracker function declarations and pointers              *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Tracker packages on most systems, including Ubuntu and Gentoo,
// do not install headers for libtracker-extract.

#include "tracker-mini.h"

// C includes
#include <dlfcn.h>
#include <errno.h>

// Tracker API version in use.
// Currently, only Tracker 1.x is supported.
int rp_tracker_api = 0;

// Module handles
static void *libtracker_extract_so = NULL;
static void *libtracker_sparql_so = NULL;

// Function pointers
tracker_sparql_pfns_u tracker_sparql_pfns;
tracker_extract_pfns_u tracker_extract_pfns;

/**
 * Initialize Tracker function pointers.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_tracker_init_pfn(void)
{
	// Attempt to open Tracker libraries.
	// TODO: Use a config.h file for the library paths.
	// Hard-coding 32-bit Ubuntu paths for now.
	if (rp_tracker_api > 0) {
		// Already initialized.
		return 0;
	}

	// Check for Tracker 1.0.
	libtracker_sparql_so = dlopen("libtracker-sparql-1.0.so.0", RTLD_NOW | RTLD_LOCAL);
	libtracker_extract_so = dlopen("libtracker-extract.so.0", RTLD_NOW | RTLD_LOCAL);
	if (!libtracker_sparql_so || !libtracker_extract_so) {
		// One or more libraries were not found.
		if (libtracker_sparql_so) {
			dlclose(libtracker_sparql_so);
			libtracker_sparql_so = NULL;
		}
		if (libtracker_extract_so) {
			dlclose(libtracker_extract_so);
			libtracker_extract_so = NULL;
		}
		return -ENOENT;
	}

	// Load symbols.
	// TODO: Check for NULL symbols?
#define DLSYM(so, u, v, substruct, prefix, sym) \
	u.v.substruct.sym = dlsym(so, #prefix "_" #sym)

	/** TrackerSparqlBuilder **/

	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, get_type);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, state_get_type);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, subject_variable);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_variable);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, subject_iri);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, subject);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, predicate_iri);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, predicate);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_iri);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_string);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_unvalidated);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_boolean);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_int64);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_date);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_double);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_blank_open);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_blank_close);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, prepend);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, append);

	/** TrackerExtractInfo **/

	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_type);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, ref);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, unref);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_file);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_mimetype);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_preupdate_builder);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_postupdate_builder);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_metadata_builder);

	rp_tracker_api = 1;
	return 0;
}

/**
 * Free Tracker function pointers.
 */
void rp_tracker_free_pfn(void)
{
	if (rp_tracker_api == 0) {
		// Not loaded.
		return;
	}

	// NOTE: Only dlclose()'ing libraries. Not zeroing function pointers.
	if (libtracker_sparql_so) {
		dlclose(libtracker_sparql_so);
		libtracker_sparql_so = NULL;
	}
	if (libtracker_extract_so) {
		dlclose(libtracker_extract_so);
		libtracker_extract_so = NULL;
	}

	rp_tracker_api = 0;
	return;
}

G_END_DECLS
