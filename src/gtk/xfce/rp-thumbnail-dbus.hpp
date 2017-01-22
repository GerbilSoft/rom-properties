/***************************************************************************
 * ROM Properties Page shell extension. (XFCE)                             *
 * rp-thumbnail-dbus.hpp: D-Bus thumbnail provider.                        *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_XFCE_RP_THUMBNAIL_DBUS_HPP__
#define __ROMPROPERTIES_XFCE_RP_THUMBNAIL_DBUS_HPP__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS;

typedef struct _RpThumbnailClass	RpThumbnailClass;
typedef struct _RpThumbnail		RpThumbnail;

#define TYPE_RP_THUMBNAIL		(rp_thumbnail_get_type())
#define RP_THUMBNAIL(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_RP_THUMBNAIL, RpThumbnail))
#define RP_THUMBNAIL_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_RP_THUMBNAIL, RpThumbnailClass))
#define IS_RP_THUMBNAIL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_RP_THUMBNAIL))
#define IS_RP_THUMBNAIL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_RP_THUMBNAIL))
#define RP_THUMBNAIL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_RP_THUMBNAIL, RpThumbnailClass))

/* these two functions are implemented automatically by the G_DEFINE_DYNAMIC_TYPE macro */
GType		rp_thumbnail_provider_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
/* NOTE: G_DEFINE_DYNAMIC_TYPE() declares the actual function as static. */
void		rp_thumbnail_provider_register_type_ext	(GTypeModule *module) G_GNUC_INTERNAL;

guint rp_thumbnail_queue(RpThumbnail *thumbnailer,
	const gchar *uri, const gchar *mime_type,
	const char *flavor, bool urgent) G_GNUC_INTERNAL;

gboolean rp_thumbnail_dequeue(RpThumbnail *thumbnailer,
	unsigned int handle, GError **error);

G_END_DECLS;

#endif /* !__ROMPROPERTIES_XFCE_RP_THUMBNAIL_DBUS_HPP__ */
