/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * MenuProviderCommon.c: Common functions for Menu Providers               *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "MenuProviderCommon.h"

#include "CreateThumbnail.hpp"
#include "img/TCreateThumbnail.hpp"

#include "common.h"

// C includes
#include <stdlib.h>
#include <string.h>
#include "tcharx.h"	// for DIR_SEP_CHR

// Supported MIME types
// TODO: Consolidate with the KF5 service menu?
#include "mime-types.convert-to-png.h"
#include <stdio.h>
/**
 * Convert the source URI to a PNG image.
 * @param source_uri URI
 * @return 0 on success; non-zero on error.
 */
int rp_menu_provider_convert_to_png(const gchar *source_uri)
{
	// Create the output URI based on the input URI.
	const size_t source_len = strlen(source_uri);
	const size_t output_len = source_len + 8;
	gchar *const output_uri = g_malloc(output_len);
	g_strlcpy(output_uri, source_uri, output_len);

	// Find the current extension and replace it.
	gchar *const dotpos = strrchr(output_uri, '.');
	if (!dotpos) {
		// No file extension. Add it.
		g_strlcat(output_uri, ".png", output_len);
	} else {
		// If the dot is after the last slash, we already have a file extension.
		// Otherwise, we don't have one, and need to add it.
		gchar *const slashpos = strrchr(output_uri, DIR_SEP_CHR);
		if (slashpos < dotpos) {
			// We already have a file extension.
			strcpy(dotpos, ".png");
		} else {
			// No file extension.
			g_strlcat(output_uri, ".png", output_len);
		}
	}

	// Convert the file using rp_create_thumbnail2().
	// TODO: Check for errors?
	int ret = rp_create_thumbnail2(source_uri, output_uri, 0, RPCT_FLAG_NO_XDG_THUMBNAIL_METADATA);
	g_free(output_uri);
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
		mime_type_compar);

	return (type != NULL);
}
