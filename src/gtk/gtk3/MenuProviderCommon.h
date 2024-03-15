/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * MenuProviderCommon.h: Common functions for Menu Providers               *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "glib-compat.h"

G_BEGIN_DECLS

/**
 * Is a URI using the file:// scheme?
 * @param uri URI
 * @return TRUE if using file://; FALSE if not.
 */
gboolean rp_is_file_uri(const gchar *uri);

/**
 * Convert the source URI to a PNG image.
 * @param source_uri URI
 * @return 0 on success; non-zero on error.
 */
int rp_menu_provider_convert_to_png(const gchar *source_uri);

/**
 * Is a MIME type supported for "Convert to PNG"?
 * @param mime MIME type
 * @return True if supported; false if not.
 */
gboolean rp_menu_provider_is_mime_type_supported(const gchar *mime_type);

G_END_DECLS
