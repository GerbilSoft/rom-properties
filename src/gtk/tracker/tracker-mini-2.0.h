/***************************************************************************
 * ROM Properties Page shell extension. (GNOME Tracker)                    *
 * tracker-mini-2.0.h: tracker-2.0 function declarations and pointers      *
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

/** TrackerResource **/

typedef struct _TrackerResource TrackerResource;

TrackerResource *tracker_resource_new (const char *identifier);

TrackerResource *tracker_resource_get_first_relation (TrackerResource *self, const char *property_uri);

void tracker_resource_set_gvalue (TrackerResource *self, const char *property_uri, const GValue *value);
void tracker_resource_set_boolean (TrackerResource *self, const char *property_uri, gboolean value);
void tracker_resource_set_double (TrackerResource *self, const char *property_uri, double value);
void tracker_resource_set_int (TrackerResource *self, const char *property_uri, int value);
void tracker_resource_set_int64 (TrackerResource *self, const char *property_uri, gint64 value);
void tracker_resource_set_relation (TrackerResource *self, const char *property_uri, TrackerResource *resource);
void tracker_resource_set_take_relation (TrackerResource *self, const char *property_uri, TrackerResource *resource);
void tracker_resource_set_string (TrackerResource *self, const char *property_uri, const char *value);
void tracker_resource_set_uri (TrackerResource *self, const char *property_uri, const char *value);

void tracker_resource_add_gvalue (TrackerResource *self, const char *property_uri, const GValue *value);
void tracker_resource_add_boolean (TrackerResource *self, const char *property_uri, gboolean value);
void tracker_resource_add_double (TrackerResource *self, const char *property_uri, double value);
void tracker_resource_add_int (TrackerResource *self, const char *property_uri, int value);
void tracker_resource_add_int64 (TrackerResource *self, const char *property_uri, gint64 value);
void tracker_resource_add_relation (TrackerResource *self, const char *property_uri, TrackerResource *resource);
void tracker_resource_add_take_relation (TrackerResource *self, const char *property_uri, TrackerResource *resource);
void tracker_resource_add_string (TrackerResource *self, const char *property_uri, const char *value);
void tracker_resource_add_uri (TrackerResource *self, const char *property_uri, const char *value);

typedef struct _tracker_sparql_2_0_pfns_t {
	struct {
		__typeof__(tracker_resource_new)		*_new;

		__typeof__(tracker_resource_get_first_relation)	*get_first_relation;

		__typeof__(tracker_resource_set_gvalue)		*set_gvalue;
		__typeof__(tracker_resource_set_boolean)	*set_boolean;
		__typeof__(tracker_resource_set_double)		*set_double;
		__typeof__(tracker_resource_set_int)		*set_int;
		__typeof__(tracker_resource_set_int64)		*set_int64;
		__typeof__(tracker_resource_set_relation)	*set_relation;
		__typeof__(tracker_resource_set_take_relation)	*set_take_relation;
		__typeof__(tracker_resource_set_string)		*set_string;
		__typeof__(tracker_resource_set_uri)		*set_uri;

		__typeof__(tracker_resource_add_gvalue)		*add_gvalue;
		__typeof__(tracker_resource_add_boolean)	*add_boolean;
		__typeof__(tracker_resource_add_double)		*add_double;
		__typeof__(tracker_resource_add_int)		*add_int;
		__typeof__(tracker_resource_add_int64)		*add_int64;
		__typeof__(tracker_resource_add_relation)	*add_relation;
		__typeof__(tracker_resource_add_take_relation)	*add_take_relation;
		__typeof__(tracker_resource_add_string)		*add_string;
		__typeof__(tracker_resource_add_uri)		*add_uri;
	} resource;
} tracker_sparql_2_0_pfns_t;

/** TrackerExtractInfo **/

// NOTE: Extension of v1.

typedef struct _TrackerExtractInfo TrackerExtractInfo;

void tracker_extract_info_set_resource (TrackerExtractInfo *info, TrackerResource *resource);

TrackerResource *tracker_extract_new_artist (const char *name);
TrackerResource *tracker_extract_new_music_album_disc (const char *album_title, TrackerResource *album_artist, int disc_number, const char *data);

typedef struct _tracker_extract_2_0_pfns_t {
	struct {
		__typeof__(tracker_extract_info_set_resource)		*set_resource;
	} info;
	struct {
		__typeof__(tracker_extract_new_artist)			*artist;
		__typeof__(tracker_extract_new_music_album_disc)	*music_album_disc;
	} _new;
} tracker_extract_2_0_pfns_t;

G_END_DECLS
