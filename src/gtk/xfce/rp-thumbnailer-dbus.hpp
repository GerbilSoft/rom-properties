/***************************************************************************
 * ROM Properties Page shell extension. (GNOME)                            *
 * rp-thumbnailer-dbus.hpp: D-Bus thumbnailer service.                     *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_XFCE_RP_THUMBNAILER_DBUS_HPP__
#define __ROMPROPERTIES_GTK_XFCE_RP_THUMBNAILER_DBUS_HPP__

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS;

/**
 * rp_create_thumbnail() function pointer.
 * @param source_file Source file. (UTF-8)
 * @param output_file Output file. (UTF-8)
 * @param maximum_size Maximum size.
 * @return 0 on success; non-zero on error.
 */
typedef int (*PFN_RP_CREATE_THUMBNAIL)(const char *source_file, const char *output_file, int maximum_size);

typedef struct _RpThumbnailerClass	RpThumbnailerClass;
typedef struct _RpThumbnailer		RpThumbnailer;

#define TYPE_RP_THUMBNAILER		(rp_thumbnailer_get_type())
#define RP_THUMBNAILER(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_RP_THUMBNAILER, RpThumbnailer))
#define RP_THUMBNAILER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_RP_THUMBNAILER, RpThumbnailerClass))
#define IS_RP_THUMBNAILER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_RP_THUMBNAILER))
#define IS_RP_THUMBNAILER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_RP_THUMBNAILER))
#define RP_THUMBNAILER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_RP_THUMBNAILER, RpThumbnailerClass))

GType		rp_thumbnailer_get_type			(void) G_GNUC_CONST G_GNUC_INTERNAL;

RpThumbnailer	*rp_thumbnailer_new			(GDBusConnection *connection,
							 const gchar *cache_dir,
							 PFN_RP_CREATE_THUMBNAIL pfn_rp_create_thumbnail)
							G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

gboolean	rp_thumbnailer_is_exported		(RpThumbnailer *thumbnailer);

G_END_DECLS;

#endif /* __ROMPROPERTIES_GTK_XFCE_RP_THUMBNAILER_DBUS_HPP__ */
