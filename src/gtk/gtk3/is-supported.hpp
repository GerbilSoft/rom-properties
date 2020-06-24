/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * is-supported.hpp: Check if a URI is supported.                          *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK3_IS_SUPPORTED_H__
#define __ROMPROPERTIES_GTK3_IS_SUPPORTED_H__

#include <glib.h>

G_BEGIN_DECLS

/**
 * Check if the specified URI is supported.
 * @param uri URI rom e.g. nautilus_file_info_get_uri().
 * @return True if supported; false if not.
 */
gboolean rp_gtk3_is_uri_supported(const gchar *uri);

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK3_IS_SUPPORTED_H__ */
