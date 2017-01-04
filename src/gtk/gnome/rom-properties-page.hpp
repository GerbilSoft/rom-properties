/***************************************************************************
 * ROM Properties Page shell extension. (GNOME)                            *
 * rom-properties-page.hpp: Nautilus Properties Page.                      *
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

#ifndef __ROMPROPERTIES_GNOME_ROM_PROPERTIES_PAGE_HPP__
#define __ROMPROPERTIES_GNOME_ROM_PROPERTIES_PAGE_HPP__

#include <libnautilus-extension/nautilus-property-page-provider.h>

G_BEGIN_DECLS;

typedef struct _RomPropertiesPageClass	RomPropertiesPageClass;
typedef struct _RomPropertiesPage	RomPropertiesPage;

#define TYPE_ROM_PROPERTIES_PAGE            (rom_properties_page_get_type ())
#define ROM_PROPERTIES_PAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_ROM_PROPERTIES_PAGE, RomPropertiesPage))
#define ROM_PROPERTIES_PAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_ROM_PROPERTIES_PAGE, RomPropertiesPageClass))
#define IS_ROM_PROPERTIES_PAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_ROM_PROPERTIES_PAGE))
#define IS_ROM_PROPERTIES_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_ROM_PROPERTIES_PAGE))
#define ROM_PROPERTIES_PAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_ROM_PROPERTIES_PAGE, RomPropertiesPageClass))

/* these two functions are implemented automatically by the G_DEFINE_DYNAMIC_TYPE macro */
GType			rom_properties_page_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
/* NOTE: G_DEFINE_DYNAMIC_TYPE() declares the actual function as static. */
void			rom_properties_page_register_type_ext	(GTypeModule *module) G_GNUC_INTERNAL;

RomPropertiesPage	*rom_properties_page_new		(void) G_GNUC_CONST G_GNUC_INTERNAL G_GNUC_MALLOC;

NautilusFileInfo	*rom_properties_page_get_file		(RomPropertiesPage	*page) G_GNUC_INTERNAL;
void			rom_properties_page_set_file		(RomPropertiesPage	*page,
								 NautilusFileInfo	*file) G_GNUC_INTERNAL;

G_END_DECLS;

#endif /* __ROMPROPERTIES_GNOME_ROM_PROPERTIES_PAGE_HPP__ */
