/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * MenuProviderCommon.c: Common functions for Menu Providers               *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MenuProviderCommon.h"

#include "CreateThumbnail.hpp"
#include "img/TCreateThumbnail.hpp"

#include "tcharx.h"	// for DIR_SEP_CHR

// Supported MIME types
// TODO: Consolidate with the KF5 service menu?
#include "mime-types.convert-to-png.h"

/**
 * Is a URI using the file:// scheme?
 * @param uri URI
 * @return TRUE if using file://; FALSE if not.
 */
static gboolean rp_is_file_uri(const gchar *uri)
{
	g_return_val_if_fail(uri != NULL, FALSE);

	gboolean ret = FALSE;
	gchar *const scheme = g_uri_parse_scheme(uri);
	if (!g_ascii_strcasecmp(scheme, "file")) {
		// It's file:// protocol!
		ret = TRUE;
	}

	g_free(scheme);
	return ret;
}

/**
 * Convert the source URI to a PNG image.
 * @param source_uri URI
 * @return 0 on success; non-zero on error.
 */
int rp_menu_provider_convert_to_png(const gchar *source_uri)
{
	// FIXME: We don't support writing to non-local files right now.
	// Only allow file:// protocol.
	if (!rp_is_file_uri(source_uri)) {
		// Not file:// protocol.
		return -1;
	}

	// Create the output filename based on the input filename.
	const size_t source_len = strlen(source_uri);
	if (source_len < 8) {
		// Doesn't have "file://".
		return -2;
	}
	// Skip the "file://" portion.
	// NOTE: Needs to be urldecoded.
	const size_t output_len_esc = source_len - 7 + 16;
	gchar *const output_file_esc = g_malloc(output_len_esc);
	g_strlcpy(output_file_esc, &source_uri[7], output_len_esc);

	// Find the current extension and replace it.
	gchar *const dotpos = strrchr(output_file_esc, '.');
	if (!dotpos) {
		// No file extension. Add it.
		g_strlcat(output_file_esc, ".png", output_len_esc);
	} else {
		// If the dot is after the last slash, we already have a file extension.
		// Otherwise, we don't have one, and need to add it.
		gchar *const slashpos = strrchr(output_file_esc, DIR_SEP_CHR);
		if (slashpos < dotpos) {
			// We already have a file extension.
			strcpy(dotpos, ".png");
		} else {
			// No file extension.
			g_strlcat(output_file_esc, ".png", output_len_esc);
		}
	}

	// Unescape the URI.
	gchar *const output_file = g_uri_unescape_string(output_file_esc, NULL);

	// Convert the file using rp_create_thumbnail2().
	// TODO: Check for errors?
	int ret = rp_create_thumbnail2(source_uri, output_file, 0, RPCT_FLAG_NO_XDG_THUMBNAIL_METADATA);
	g_free(output_file_esc);
	g_free(output_file);
	return ret;
}

/**
 * Comparator function for rp_menu_provider_is_mime_type_supported().
 *
 * bsearch() gives us a pointer to the const char*, not the const char*,
 * so we can't simply use strcmp().
 *
 * @param a
 * @param b
 * @return strcmp() result
 */
static int mime_type_compar(const void *a, const void *b)
{
	const char *const sa = *((const char**)a);
	const char *const sb = *((const char**)b);
	return strcmp(sa, sb);
}

/**
 * Is a MIME type supported for "Convert to PNG"?
 * @param mime MIME type
 * @return True if supported; false if not.
 */
gboolean rp_menu_provider_is_mime_type_supported(const gchar *mime_type)
{
	// Check the file against all supported MIME types.
	const char *const type = bsearch(&mime_type,
		mime_types_convert_to_png,
		ARRAY_SIZE(mime_types_convert_to_png),
		sizeof(mime_types_convert_to_png[0]),
		(int(*)(const void*, const void*))mime_type_compar);

	return (type != NULL);
}
