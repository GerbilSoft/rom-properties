/***************************************************************************
 * ROM Properties Page shell extension. (GNOME Tracker)                    *
 * tracker-mini.h: Tracker function declarations and pointers              *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Tracker packages on most systems, including Ubuntu and Gentoo,
// do not install headers for libtracker-extract.

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

// Tracker API version in use.
// Currently, only Tracker 1.x is supported.
extern int rp_tracker_api;

/** TrackerSparqlBuilder **/

#define TRACKER_SPARQL_TYPE_BUILDER (tracker_sparql_builder_get_type ())
#define TRACKER_SPARQL_BUILDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TRACKER_SPARQL_TYPE_BUILDER, TrackerSparqlBuilder))
#define TRACKER_SPARQL_BUILDER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TRACKER_SPARQL_TYPE_BUILDER, TrackerSparqlBuilderClass))
#define TRACKER_SPARQL_IS_BUILDER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TRACKER_SPARQL_TYPE_BUILDER))
#define TRACKER_SPARQL_IS_BUILDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TRACKER_SPARQL_TYPE_BUILDER))
#define TRACKER_SPARQL_BUILDER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TRACKER_SPARQL_TYPE_BUILDER, TrackerSparqlBuilderClass))

typedef struct _TrackerSparqlBuilder TrackerSparqlBuilder;
typedef struct _TrackerSparqlBuilderClass TrackerSparqlBuilderClass;
typedef struct _TrackerSparqlBuilder TrackerSparqlBuilderPrivate;

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

extern __typeof__(tracker_sparql_builder_get_type)		*pfn_tracker_sparql_builder_get_type;
extern __typeof__(tracker_sparql_builder_state_get_type)	*pfn_tracker_sparql_builder_state_get_type;
extern __typeof__(tracker_sparql_builder_subject_variable)	*pfn_tracker_sparql_builder_subject_variable;
extern __typeof__(tracker_sparql_builder_object_variable)	*pfn_tracker_sparql_builder_object_variable;
extern __typeof__(tracker_sparql_builder_subject_iri)		*pfn_tracker_sparql_builder_subject_iri;
extern __typeof__(tracker_sparql_builder_subject)		*pfn_tracker_sparql_builder_subject;
extern __typeof__(tracker_sparql_builder_predicate_iri)		*pfn_tracker_sparql_builder_predicate_iri;
extern __typeof__(tracker_sparql_builder_predicate)		*pfn_tracker_sparql_builder_predicate;
extern __typeof__(tracker_sparql_builder_object_iri)		*pfn_tracker_sparql_builder_object_iri;
extern __typeof__(tracker_sparql_builder_object)		*pfn_tracker_sparql_builder_object;
extern __typeof__(tracker_sparql_builder_object_string)		*pfn_tracker_sparql_builder_object_string;
extern __typeof__(tracker_sparql_builder_object_unvalidated)	*pfn_tracker_sparql_builder_object_unvalidated;
extern __typeof__(tracker_sparql_builder_object_boolean)	*pfn_tracker_sparql_builder_object_boolean;
extern __typeof__(tracker_sparql_builder_object_int64)		*pfn_tracker_sparql_builder_object_int64;
extern __typeof__(tracker_sparql_builder_object_date)		*pfn_tracker_sparql_builder_object_date;
extern __typeof__(tracker_sparql_builder_object_double)		*pfn_tracker_sparql_builder_object_double;
extern __typeof__(tracker_sparql_builder_object_blank_open)	*pfn_tracker_sparql_builder_object_blank_open;
extern __typeof__(tracker_sparql_builder_object_blank_close)	*pfn_tracker_sparql_builder_object_blank_close;
extern __typeof__(tracker_sparql_builder_prepend)		*pfn_tracker_sparql_builder_prepend;
extern __typeof__(tracker_sparql_builder_append)		*pfn_tracker_sparql_builder_append;

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

extern __typeof__(tracker_extract_info_get_type)		*pfn_tracker_extract_info_get_type;
extern __typeof__(tracker_extract_info_ref)			*pfn_tracker_extract_info_ref;
extern __typeof__(tracker_extract_info_unref)			*pfn_tracker_extract_info_unref;
extern __typeof__(tracker_extract_info_get_file)		*pfn_tracker_extract_info_get_file;
extern __typeof__(tracker_extract_info_get_mimetype)		*pfn_tracker_extract_info_get_mimetype;
extern __typeof__(tracker_extract_info_get_preupdate_builder)	*pfn_tracker_extract_info_get_preupdate_builder;
extern __typeof__(tracker_extract_info_get_postupdate_builder)	*pfn_tracker_extract_info_get_postupdate_builder;
extern __typeof__(tracker_extract_info_get_metadata_builder)	*pfn_tracker_extract_info_get_metadata_builder;

/** rom-properties **/

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
