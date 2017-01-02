/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataView.hpp: RomData viewer widget.                                 *
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

#ifndef __ROMPROPERTIES_GTK_ROMDATA_VIEW_HPP__
#define __ROMPROPERTIES_GTK_ROMDATA_VIEW_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS;

typedef struct _RomDataViewClass	RomDataViewClass;
typedef struct _RomDataView		RomDataView;

#define TYPE_ROM_DATA_VIEW            (rom_data_view_get_type ())
#define ROM_DATA_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_ROM_DATA_VIEW, RomDataView))
#define ROM_DATA_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_ROM_DATA_VIEW, RomDataViewClass))
#define IS_ROM_DATA_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_ROM_DATA_VIEW))
#define IS_ROM_DATA_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_ROM_DATA_VIEW))
#define ROM_DATA_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_ROM_DATA_VIEW, RomDataViewClass))

GType		rom_data_view_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
GtkWidget	*rom_data_view_new		(void) G_GNUC_CONST G_GNUC_INTERNAL G_GNUC_MALLOC;

const gchar	*rom_data_view_get_filename	(RomDataView	*page) G_GNUC_INTERNAL;
void		rom_data_view_set_filename	(RomDataView	*page,
						 const gchar	*file) G_GNUC_INTERNAL;

G_END_DECLS;

#endif /* __ROMPROPERTIES_GTK_ROMDATA_VIEW_HPP__ */
