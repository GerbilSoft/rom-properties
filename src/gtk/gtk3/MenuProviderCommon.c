/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * MenuProviderCommon.h: Common functions for Menu Providers               *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MenuProviderCommon.h"

#include "CreateThumbnail.hpp"
#include "img/TCreateThumbnail.hpp"

#include "tcharx.h"	// for DIR_SEP_CHR

/**
 * Is a URI using the file:// scheme?
 * @param uri URI
 * @return TRUE if using file://; FALSE if not.
 */
gboolean rp_is_file_uri(const gchar *uri)
{
	gboolean ret = FALSE;
	if (G_UNLIKELY(!uri))
		return ret;

	gchar *const scheme = g_uri_parse_scheme(uri);
	if (!g_ascii_strcasecmp(scheme, "file")) {
		// It's file:// protocol!
		ret = true;
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
