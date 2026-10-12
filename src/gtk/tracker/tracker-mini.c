/***************************************************************************
 * ROM Properties Page shell extension. (GNOME Tracker)                    *
 * tracker-mini.c: Tracker function declarations and pointers              *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Tracker packages on most systems, including Ubuntu and Gentoo,
// do not install headers for libtracker-extract.

#include "tracker-mini.h"
#include "tracker-file-utils.h"
#include "dll-macros.h"	// for RP_LIBRARY_SO_VERSIONED()

// C includes
#include <dlfcn.h>
#include <errno.h>

// Tracker API version in use
int rp_tracker_api = 0;

// Is this localsearch? (v3.9 or later)
bool rp_tracker_is_3L = false;

// Module handles
static void *libtracker_extract_so;
static void *libtracker_sparql_so;

// Function pointers
tracker_sparql_pfns_u tracker_sparql_pfns;
tracker_extract_pfns_t tracker_extract_pfns;

#define DLSYM(so, u, v, substruct, prefix, sym) \
	u.v.substruct.sym = dlsym(so, #prefix "_" #sym)

// TrackerExtractInfo struct for localsearch v3.9, v3.10, and v3.11
typedef struct _TrackerExtractInfo_v39
{
	TrackerResource *resource;

	GFile *file;
	gchar *content_id;
	gchar *mimetype;
	gchar *graph;

	gint max_text;

	gint ref_count;
} TrackerExtractInfo_v39;

/**
 * Internal replacement for tracker_extract_info_get_file()
 * when using localsearch v3.9, v3.10, and v3.11.
 * @param info
 * @return
 */
static GFile *rp_tracker_extract_info_get_file_v39(TrackerExtractInfo_v39 *info)
{
	g_return_val_if_fail(info != NULL, NULL);
	return info->file;
}

/**
 * Internal replacement for tracker_extract_info_set_resource()
 * when using localsearch v3.9, v3.10, and v3.11.
 * @param info
 * @param resource
 */
static void rp_tracker_extract_info_set_resource_v39(TrackerExtractInfo_v39 *info, TrackerResource *resource)
{
	g_object_ref(resource);
	info->resource = resource;
}

/**
 * Initialize libtracker_extract v1 function pointers.
 * Common to both v1 and v2.
 * @param isV1 If true, using ABI v1; get_metadata_builder is required. (Not used for v2 or later.)
 * @param is3L If true, use some special shenanigans for localsearch v3.9, v3.10, and v3.11.
 * @return 0 on success; non-zero on error.
 */
static int init_tracker_extract_v1(bool isV1, bool is3L)
{
	/** TrackerExtractInfo **/

	//DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_type);
	//DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, ref);
	//DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, unref);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_file);
	//DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_mimetype);
	//DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_preupdate_builder);
	//DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_postupdate_builder);

	// localsearch 3.9, 3.10, and 3.11 moved tracker_extract_info_get_file()
	// from libtracker-extract.so to localsearch-extractor-3.
	// localsearch 3.12 moves it back.
	if (!tracker_extract_pfns.v1.info.get_file && is3L) {
		// NOTE: dlsym(RTLD_DEFAULT) isn't working.
		// We'll use our own temporary replacement version for 3L that accesses the struct directly.
		//tracker_extract_pfns.v1.info.get_file = dlsym(RTLD_DEFAULT, "tracker_extract_info_get_file");
		tracker_extract_pfns.v1.info.get_file = (__typeof__(tracker_extract_info_get_file)*)rp_tracker_extract_info_get_file_v39;
	}

	if (!tracker_extract_pfns.v1.info.get_file) {
		return -ENOENT;
	}

	// v1 only
	if (isV1) {
		DLSYM(libtracker_extract_so, tracker_extract_pfns, v1, info, tracker_extract_info, get_metadata_builder);
		if (!tracker_extract_pfns.v1.info.get_metadata_builder) {
			return -ENOENT;
		}
	}

	return 0;
}

/**
 * Initialize tracker-1.0 function pointers.
 * @return 0 on success; non-zero on error.
 */
static int init_tracker_v1(void)
{
	/** TrackerExtractInfo **/
	int ret = init_tracker_extract_v1(true, false);
	if (ret != 0) {
		return ret;
	}

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

	if (!tracker_sparql_pfns.v1.builder.predicate ||
	    !tracker_sparql_pfns.v1.builder.object ||
	    !tracker_sparql_pfns.v1.builder.object_string ||
	    !tracker_sparql_pfns.v1.builder.object_int64 ||
	    !tracker_sparql_pfns.v1.builder.object_date)
	{
		return -ENOENT;
	}

	return 0;
}

/**
 * Initialize tracker-2.0 function pointers.
 * @param is3L If true, use some special shenanigans for localsearch v3.9, v3.10, and v3.11.
 * @return 0 on success; non-zero on error.
 */
static int init_tracker_v2(bool is3L)
{
	/** TrackerExtractInfo **/

	int ret = init_tracker_extract_v1(false, is3L);
	if (ret != 0) {
		return ret;
	}

	DLSYM(libtracker_extract_so, tracker_extract_pfns, v2, info, tracker_extract_info, set_resource);

	// localsearch 3.9, 3.10, and 3.11 moved tracker_extract_info_set_resource()
	// from libtracker-extract.so to localsearch-extractor-3.
	// localsearch 3.12 moves it back.
	if (!tracker_extract_pfns.v2.info.set_resource && is3L) {
		// NOTE: dlsym(RTLD_DEFAULT) isn't working.
		// We'll use our own temporary replacement version for 3L that accesses the struct directly.
		//tracker_extract_pfns.v2.info.set_resource = dlsym(NULL, "tracker_extract_info_set_resource");
		tracker_extract_pfns.v2.info.set_resource = (__typeof__(tracker_extract_info_set_resource)*)rp_tracker_extract_info_set_resource_v39;
	}

	DLSYM(libtracker_extract_so, tracker_extract_pfns, v2, _new, tracker_extract_new, artist);
	DLSYM(libtracker_extract_so, tracker_extract_pfns, v2, _new, tracker_extract_new, music_album_disc);

	if (!tracker_extract_pfns.v2.info.set_resource ||
	    !tracker_extract_pfns.v2._new.artist ||
	    !tracker_extract_pfns.v2._new.music_album_disc)
	{
		return -ENOENT;
	}

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

	if (!tracker_sparql_pfns.v2.resource._new ||
	    !tracker_sparql_pfns.v2.resource.get_first_relation ||
	    !tracker_sparql_pfns.v2.resource.set_gvalue ||
	    !tracker_sparql_pfns.v2.resource.set_int ||
	    !tracker_sparql_pfns.v2.resource.set_relation ||
	    !tracker_sparql_pfns.v2.resource.set_take_relation ||
	    !tracker_sparql_pfns.v2.resource.set_string ||
	    !tracker_sparql_pfns.v2.resource.add_take_relation ||
	    !tracker_sparql_pfns.v2.resource.add_uri)
	{
		return -ENOENT;
	}

	return 0;
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
		bool is3L;		// Set for localsearch only
	} TrackerApiLibs_t;
	static const TrackerApiLibs_t trackerApiLibs[] = {
		// LocalSearch 3.0 (aka tracker-3.8)
		{RP_LIBRARY_SO_VERSIONED("libtinysparql-3.0.so", ".0"), "libtracker-extract.so", 3, true},

		// Tracker 3.0
		{RP_LIBRARY_SO_VERSIONED("libtracker-sparql-3.0.so", ".0"), "libtracker-extract.so", 3, false},

		// Tracker 2.0
		{RP_LIBRARY_SO_VERSIONED("libtracker-sparql-2.0.so", ".0"), "libtracker-extract.so.0", 2, false},

		// Tracker 1.0
		{RP_LIBRARY_SO_VERSIONED("libtracker-sparql-1.0.so", ".0"), "libtracker-extract.so.0", 1, false},
	};
	static const TrackerApiLibs_t *const pEnd = &trackerApiLibs[sizeof(trackerApiLibs)/sizeof(trackerApiLibs[0])];

	for (const TrackerApiLibs_t *p = trackerApiLibs; p < pEnd; p++) {
		libtracker_sparql_so = dlopen(p->sparql_so, RP_DLOPEN_FLAGS);
		if (!libtracker_sparql_so) {
			// Not found. Try the next one.
			continue;
		}

		libtracker_extract_so = dlopen(p->extract_so, RP_DLOPEN_FLAGS);
		if (!libtracker_extract_so) {
			// Not found. Try the next one.
			dlclose(libtracker_sparql_so);
			libtracker_sparql_so = NULL;
			continue;
		}

		// Found a Tracker API.
		// NOTE: API v3 is essentially the same as v2.
		int ret = 0;
		if (p->api_version >= 2) {
			ret = init_tracker_v2(p->is3L);
		} else {
			ret = init_tracker_v1();
		}

		if (ret == 0) {
			rp_tracker_api = p->api_version;
			rp_tracker_is_3L = p->is3L;
			return 0;
		}

		// Could not initialize the libraries...
		dlclose(libtracker_sparql_so);
		libtracker_sparql_so = NULL;
		dlclose(libtracker_extract_so);
		libtracker_extract_so = NULL;

		rp_tracker_api = 0;
		break;
	}

	// One or more libraries were not found,
	// or required symbols are missing.
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
