/***************************************************************************
 * ROM Properties Page shell extension. (D-Bus Thumbnailer)                *
 * rp-thumbnailer-dbus.h: D-Bus thumbnailer service.                       *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "glib-compat.h"
#include <gio/gio.h>

G_BEGIN_DECLS

/**
 * rp_create_thumbnail2() function pointer.
 * @param source_file Source file (UTF-8)
 * @param output_file Output file (UTF-8)
 * @param maximum_size Maximum size
 * @param flags Flags (see RpCreateThumbnailFlags)
 * @return 0 on success; non-zero on error.
 */
typedef int (*PFN_RP_CREATE_THUMBNAIL2)(
	const char *source_file, const char *output_file, int maximum_size, unsigned int flags);

#define RP_TYPE_THUMBNAILER (rp_thumbnailer_get_type())
G_DECLARE_FINAL_TYPE(RpThumbnailer, rp_thumbnailer, RP, THUMBNAILER, GObject)

RpThumbnailer	*rp_thumbnailer_new			(GDBusConnection *connection,
							 const gchar *cache_dir,
							 PFN_RP_CREATE_THUMBNAIL2 pfn_rp_create_thumbnail2)
							G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

gboolean	rp_thumbnailer_is_exported		(RpThumbnailer *thumbnailer);

G_END_DECLS
