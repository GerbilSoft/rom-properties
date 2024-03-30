/***************************************************************************
 * ROM Properties Page shell extension. (GNOME Tracker)                    *
 * tracker-mini-1.0.h: tracker-1.0 function declarations and pointers      *
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

G_BEGIN_DECLS

/** TrackerSparqlBuilder **/

typedef struct _TrackerSparqlBuilder TrackerSparqlBuilder;
typedef struct _TrackerSparqlBuilderPrivate TrackerSparqlBuilderPrivate;

struct _TrackerSparqlBuilder {
	GObject parent_instance;
	TrackerSparqlBuilderPrivate * priv;
};

GType tracker_sparql_builder_get_type (void) G_GNUC_CONST;
GType tracker_sparql_builder_state_get_type (void) G_GNUC_CONST;
void tracker_sparql_builder_subject_variable (TrackerSparqlBuilder* self, const gchar* var_name);
void tracker_sparql_builder_object_variable (TrackerSparqlBuilder* self, const gchar* var_name);
void tracker_sparql_builder_subject_iri (TrackerSparqlBuilder* self, const gchar* iri);
void tracker_sparql_builder_subject (TrackerSparqlBuilder* self, const gchar* s);
void tracker_sparql_builder_predicate_iri (TrackerSparqlBuilder* self, const gchar* iri);
void tracker_sparql_builder_predicate (TrackerSparqlBuilder* self, const gchar* s);
void tracker_sparql_builder_object_iri (TrackerSparqlBuilder* self, const gchar* iri);
void tracker_sparql_builder_object (TrackerSparqlBuilder* self, const gchar* s);
void tracker_sparql_builder_object_string (TrackerSparqlBuilder* self, const gchar* literal);
void tracker_sparql_builder_object_unvalidated (TrackerSparqlBuilder* self, const gchar* value);
void tracker_sparql_builder_object_boolean (TrackerSparqlBuilder* self, gboolean literal);
void tracker_sparql_builder_object_int64 (TrackerSparqlBuilder* self, gint64 literal);
void tracker_sparql_builder_object_date (TrackerSparqlBuilder* self, time_t* literal);
void tracker_sparql_builder_object_double (TrackerSparqlBuilder* self, gdouble literal);
void tracker_sparql_builder_object_blank_open (TrackerSparqlBuilder* self);
void tracker_sparql_builder_object_blank_close (TrackerSparqlBuilder* self);
void tracker_sparql_builder_prepend (TrackerSparqlBuilder* self, const gchar* raw);
void tracker_sparql_builder_append (TrackerSparqlBuilder* self, const gchar* raw);

typedef struct _tracker_sparql_1_0_pfns_t {
	struct {
		__typeof__(tracker_sparql_builder_get_type)		*get_type;
		__typeof__(tracker_sparql_builder_state_get_type)	*state_get_type;
		__typeof__(tracker_sparql_builder_subject_variable)	*subject_variable;
		__typeof__(tracker_sparql_builder_object_variable)	*object_variable;
		__typeof__(tracker_sparql_builder_subject_iri)		*subject_iri;
		__typeof__(tracker_sparql_builder_subject)		*subject;
		__typeof__(tracker_sparql_builder_predicate_iri)	*predicate_iri;
		__typeof__(tracker_sparql_builder_predicate)		*predicate;
		__typeof__(tracker_sparql_builder_object_iri)		*object_iri;
		__typeof__(tracker_sparql_builder_object)		*object;
		__typeof__(tracker_sparql_builder_object_string)	*object_string;
		__typeof__(tracker_sparql_builder_object_unvalidated)	*object_unvalidated;
		__typeof__(tracker_sparql_builder_object_boolean)	*object_boolean;
		__typeof__(tracker_sparql_builder_object_int64)		*object_int64;
		__typeof__(tracker_sparql_builder_object_date)		*object_date;
		__typeof__(tracker_sparql_builder_object_double)	*object_double;
		__typeof__(tracker_sparql_builder_object_blank_open)	*object_blank_open;
		__typeof__(tracker_sparql_builder_object_blank_close)	*object_blank_close;
		__typeof__(tracker_sparql_builder_prepend)		*prepend;
		__typeof__(tracker_sparql_builder_append)		*append;
	} builder;
} tracker_sparql_1_0_pfns_t;

/** TrackerExtractInfo **/

typedef struct _TrackerExtractInfo TrackerExtractInfo;

GType                 tracker_extract_info_get_type               (void) G_GNUC_CONST;

TrackerExtractInfo *  tracker_extract_info_ref                    (TrackerExtractInfo *info);
void                  tracker_extract_info_unref                  (TrackerExtractInfo *info);
GFile *               tracker_extract_info_get_file               (TrackerExtractInfo *info);
const gchar *         tracker_extract_info_get_mimetype           (TrackerExtractInfo *info);
TrackerSparqlBuilder *tracker_extract_info_get_preupdate_builder  (TrackerExtractInfo *info);
TrackerSparqlBuilder *tracker_extract_info_get_postupdate_builder (TrackerExtractInfo *info);
TrackerSparqlBuilder *tracker_extract_info_get_metadata_builder   (TrackerExtractInfo *info);

typedef struct _tracker_extract_1_0_pfns_t {
	struct {
		__typeof__(tracker_extract_info_get_type)		*get_type;
		__typeof__(tracker_extract_info_ref)			*ref;
		__typeof__(tracker_extract_info_unref)			*unref;
		__typeof__(tracker_extract_info_get_file)		*get_file;
		__typeof__(tracker_extract_info_get_mimetype)		*get_mimetype;
		__typeof__(tracker_extract_info_get_preupdate_builder)	*get_preupdate_builder;
		__typeof__(tracker_extract_info_get_postupdate_builder)	*get_postupdate_builder;
		__typeof__(tracker_extract_info_get_metadata_builder)	*get_metadata_builder;
	} info;
} tracker_extract_1_0_pfns_t;

G_END_DECLS
