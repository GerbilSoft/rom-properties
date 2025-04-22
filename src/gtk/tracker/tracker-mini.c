/***************************************************************************
 * ROM Properties Page shell extension. (GNOME Tracker)                    *
 * tracker-mini.c: Tracker function declarations and pointers              *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
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
 * Initialize libtracker_extract v1 function pointers.
 * Common to both v1 and v2.
 */
static void init_tracker_extract_v1(void)
{
	/** TrackerExtractInfo **/

	//DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_type);
	//DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, ref);
	//DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, unref);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_file);
	//DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_mimetype);
	//DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_preupdate_builder);
	//DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_postupdate_builder);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_metadata_builder);
}

/**
 * Initialize tracker-1.0 function pointers.
 */
static void init_tracker_v1(void)
{
	/** TrackerSparqlBuilder **/

	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, get_type);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, state_get_type);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, subject_variable);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_variable);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, subject_iri);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, subject);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, predicate_iri);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, predicate);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_iri);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_string);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_unvalidated);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_boolean);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_int64);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_date);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_double);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_blank_open);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, object_blank_close);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, prepend);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v1, builder, tracker_sparql_builder, append);

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
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_boolean);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_double);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_int);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_int64);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_relation);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_take_relation);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_string);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, set_uri);

	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_gvalue);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_boolean);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_double);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_int);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_int64);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_relation);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_take_relation);
	//DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_string);
	DLSYM(libtracker_sparql_so, tracker_sparql_pfns, v2, resource, tracker_resource, add_uri);
// 
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
	// NOTE: The libraries are already loaded in-process,
	// so this should "just work" without having to specify
	// the full paths.
	if (rp_tracker_api > 0) {
		// Already initialized.
		return 0;
	}

	typedef struct _TrackerApiLibs_t {
		const char *sparql_so;	// libtracker-sparql filename
		const char *extract_so;	// libtracker-extract.so filename
		int api_version;	// Tracker API version (1, 2, 3; 3L isn't used here)
	} TrackerApiLibs_t;
	static const TrackerApiLibs_t trackerApiLibs[] = {
		// LocalSearch 3.0 (aka tracker-3.8)
		{"libtinysparql-3.0.so.0", "libtracker-extract.so", 3},

		// Tracker 3.0
		{"libtracker-sparql-3.0.so.0", "libtracker-extract.so", 3},

		// Tracker 2.0
		{"libtracker-sparql-2.0.so.0", "libtracker-extract.so.0", 2},

		// Tracker 1.0
		{"libtracker-sparql-1.0.so.0", "libtracker-extract.so.0", 1},
	};
	static const TrackerApiLibs_t *const pEnd = &trackerApiLibs[sizeof(trackerApiLibs)/sizeof(trackerApiLibs[0])];

	for (const TrackerApiLibs_t *p = trackerApiLibs; p < pEnd; p++) {
		libtracker_sparql_so = dlopen(p->sparql_so, RTLD_NOW | RTLD_LOCAL);
		if (!libtracker_sparql_so) {
			// Not found. Try the next one.
			continue;
		}

		libtracker_extract_so = dlopen(p->extract_so, RTLD_NOW | RTLD_LOCAL);
		if (!libtracker_extract_so) {
			// Not found. Try the next one.
			dlclose(libtracker_sparql_so);
			libtracker_sparql_so = NULL;
			continue;
		}

		// Found a Tracker API.
		// NOTE: API v3 is essentially the same as v2.
		if (p->api_version >= 2) {
			init_tracker_v2();
		} else {
			init_tracker_v1();
		}

		rp_tracker_api = p->api_version;
		return 0;
	}

	// One or more libraries were not found.
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
