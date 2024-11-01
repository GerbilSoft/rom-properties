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
#include "tracker-file-utils.h"

// C includes
#include <dlfcn.h>
#include <errno.h>

// Tracker API version in use.
// Currently, only Tracker 1.x is supported.
int rp_tracker_api = 0;

// Module handles
static void *libtracker_extract_so;
static void *libtracker_sparql_so;

// Function pointers
tracker_sparql_pfns_u tracker_sparql_pfns;
tracker_extract_pfns_t tracker_extract_pfns;

#define DLSYM(so, u, v, substruct, prefix, sym) \
	u.v.substruct.sym = dlsym(so, #prefix "_" #sym)

/**
 * Close dlopen()'d module pointers.
 */
static void close_modules(void)
{
	// TODO: NULL out the function pointers?
	if (libtracker_sparql_so) {
		dlclose(libtracker_sparql_so);
		libtracker_sparql_so = NULL;
	}
	if (libtracker_extract_so) {
		dlclose(libtracker_extract_so);
		libtracker_extract_so = NULL;
	}

	rp_tracker_api = 0;
}

/**
 * Initialize libtracker_extract v1 function pointers.
 * Common to both v1 and v2.
 */
static void init_tracker_extract_v1(void)
{
	/** TrackerExtractInfo **/

	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_type);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, ref);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, unref);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_file);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_mimetype);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_preupdate_builder);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_postupdate_builder);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_metadata_builder);
}

/**
 * Initialize tracker-1.0 function pointers.
 */
static void init_tracker_v1(void)
{
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
	init_tracker_extract_v1();

	rp_tracker_api = 1;
}

/**
 * Initialize tracker-2.0 function pointers.
 */
static void init_tracker_v2(void)
{
	// TODO: Check for NULL symbols?

	/** TrackerResource (part of libtracker_sparql) **/

	tracker_sparql_pfns.v2.resource._new = dlsym(libtracker_sparql_so, "tracker_resource_new");

	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, get_first_relation);

	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_gvalue);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_boolean);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_double);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_int);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_int64);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_relation);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_take_relation);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_string);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_uri);

	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_gvalue);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_boolean);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_double);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_int);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_int64);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_relation);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_take_relation);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_string);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_uri);

	/** TrackerExtractInfo **/

	init_tracker_extract_v1();
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v2, info, tracker_extract_info, set_resource);

	DLSYM(libtracker_extract_so, tracker_extract_pfns, v2, _new, tracker_extract_new, artist);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v2, _new, tracker_extract_new, music_album_disc);

	rp_tracker_api = 2;
}

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

	// Check for Tracker 3.0.
	libtracker_sparql_so = dlopen("libtracker-sparql-3.0.so.0", RTLD_NOW | RTLD_LOCAL);
	libtracker_extract_so = dlopen("libtracker-extract.so", RTLD_NOW | RTLD_LOCAL);
	if (libtracker_sparql_so && libtracker_extract_so) {
		// Found Tracker 3.0.
		// NOTE: Functions are essentially the same as 2.0.
		init_tracker_v2();
		rp_tracker_api = 3;
		return 0;
	}

	// Clear dlopen() pointers and try again.
	close_modules();

	// Check for Tracker 2.0.
	libtracker_sparql_so = dlopen("libtracker-sparql-2.0.so.0", RTLD_NOW | RTLD_LOCAL);
	libtracker_extract_so = dlopen("libtracker-extract.so.0", RTLD_NOW | RTLD_LOCAL);
	if (libtracker_sparql_so && libtracker_extract_so) {
		// Found Tracker 2.0.
		init_tracker_v2();
		return 0;
	}

	// Clear dlopen() pointers and try again.
	close_modules();

	// Check for Tracker 1.0.
	libtracker_sparql_so = dlopen("libtracker-sparql-1.0.so.0", RTLD_NOW | RTLD_LOCAL);
	libtracker_extract_so = dlopen("libtracker-extract.so.0", RTLD_NOW | RTLD_LOCAL);
	if (libtracker_sparql_so && libtracker_extract_so) {
		// Found Tracker 1.0.
		init_tracker_v1();
		return 0;
	}

	// One or more libraries were not found.
	close_modules();
	return -ENOENT;
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
}

G_END_DECLS
