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

/** TrackerSparqlBuilder **/

__typeof__(tracker_sparql_builder_get_type)		*pfn_tracker_sparql_builder_get_type;
__typeof__(tracker_sparql_builder_state_get_type)	*pfn_tracker_sparql_builder_state_get_type;
__typeof__(tracker_sparql_builder_subject_variable)	*pfn_tracker_sparql_builder_subject_variable;
__typeof__(tracker_sparql_builder_object_variable)	*pfn_tracker_sparql_builder_object_variable;
__typeof__(tracker_sparql_builder_subject_iri)		*pfn_tracker_sparql_builder_subject_iri;
__typeof__(tracker_sparql_builder_subject)		*pfn_tracker_sparql_builder_subject;
__typeof__(tracker_sparql_builder_predicate_iri)	*pfn_tracker_sparql_builder_predicate_iri;
__typeof__(tracker_sparql_builder_predicate)		*pfn_tracker_sparql_builder_predicate;
__typeof__(tracker_sparql_builder_object_iri)		*pfn_tracker_sparql_builder_object_iri;
__typeof__(tracker_sparql_builder_object)		*pfn_tracker_sparql_builder_object;
__typeof__(tracker_sparql_builder_object_string)	*pfn_tracker_sparql_builder_object_string;
__typeof__(tracker_sparql_builder_object_unvalidated)	*pfn_tracker_sparql_builder_object_unvalidated;
__typeof__(tracker_sparql_builder_object_boolean)	*pfn_tracker_sparql_builder_object_boolean;
__typeof__(tracker_sparql_builder_object_int64)		*pfn_tracker_sparql_builder_object_int64;
__typeof__(tracker_sparql_builder_object_date)		*pfn_tracker_sparql_builder_object_date;
__typeof__(tracker_sparql_builder_object_double)	*pfn_tracker_sparql_builder_object_double;
__typeof__(tracker_sparql_builder_object_blank_open)	*pfn_tracker_sparql_builder_object_blank_open;
__typeof__(tracker_sparql_builder_object_blank_close)	*pfn_tracker_sparql_builder_object_blank_close;
__typeof__(tracker_sparql_builder_prepend)		*pfn_tracker_sparql_builder_prepend;
__typeof__(tracker_sparql_builder_append)		*pfn_tracker_sparql_builder_append;

/** TrackerExtractInfo **/

__typeof__(tracker_extract_info_get_type)		*pfn_tracker_extract_info_get_type;
__typeof__(tracker_extract_info_ref)			*pfn_tracker_extract_info_ref;
__typeof__(tracker_extract_info_unref)			*pfn_tracker_extract_info_unref;
__typeof__(tracker_extract_info_get_file)		*pfn_tracker_extract_info_get_file;
__typeof__(tracker_extract_info_get_mimetype)		*pfn_tracker_extract_info_get_mimetype;
__typeof__(tracker_extract_info_get_preupdate_builder)	*pfn_tracker_extract_info_get_preupdate_builder;
__typeof__(tracker_extract_info_get_postupdate_builder)	*pfn_tracker_extract_info_get_postupdate_builder;
__typeof__(tracker_extract_info_get_metadata_builder)	*pfn_tracker_extract_info_get_metadata_builder;

/**
 * Initialize Tracker function pointers.
 * @return 0 on success; negative POSIX error code on error.
 */
#include <stdio.h>
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
#define DLSYM(so, sym) \
	pfn_##sym = dlsym(so, #sym)

	/** TrackerSparqlBuilder **/

	DLSYM(libtracker_sparql_so, tracker_sparql_builder_get_type);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_state_get_type);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_subject_variable);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_object_variable);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_subject_iri);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_subject);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_predicate_iri);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_predicate);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_object_iri);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_object);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_object_string);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_object_unvalidated);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_object_boolean);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_object_int64);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_object_date);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_object_double);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_object_blank_open);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_object_blank_close);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_prepend);
	DLSYM(libtracker_sparql_so, tracker_sparql_builder_append);

	/** TrackerExtractInfo **/

	DLSYM(libtracker_extract_so, tracker_extract_info_get_type);
	DLSYM(libtracker_extract_so, tracker_extract_info_ref);
	DLSYM(libtracker_extract_so, tracker_extract_info_unref);
	DLSYM(libtracker_extract_so, tracker_extract_info_get_file);
	DLSYM(libtracker_extract_so, tracker_extract_info_get_mimetype);
	DLSYM(libtracker_extract_so, tracker_extract_info_get_preupdate_builder);
	DLSYM(libtracker_extract_so, tracker_extract_info_get_postupdate_builder);
	DLSYM(libtracker_extract_so, tracker_extract_info_get_metadata_builder);

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
