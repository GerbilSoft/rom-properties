/***************************************************************************
 * ROM Properties Page shell extension. (XFCE)                             *
 * rom-properties-provider.hpp: ThunarX Provider Definition.               *
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

#ifndef __ROMPROPERTIES_XFCE_ROM_PROPERTIES_PROVIDER_HPP__
#define __ROMPROPERTIES_XFCE_ROM_PROPERTIES_PROVIDER_HPP__

#include <glib.h>

G_BEGIN_DECLS;

// NOTE: thunarx.h doesn't have extern "C" set up properly everywhere.
#include <thunarx/thunarx.h>

typedef struct _RomPropertiesProviderClass	RomPropertiesProviderClass;
typedef struct _RomPropertiesProvider		RomPropertiesProvider;

#define TYPE_ROM_PROPERTIES_PROVIDER		(rom_properties_provider_get_type ())
#define ROM_PROPERTIES_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_ROM_PROPERTIES_PROVIDER, RomPropertiesProvider))
#define ROM_PROPERTIES_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_ROM_PROPERTIES_PROVIDER, RomPropertiesProviderClass))
#define IS_ROM_PROPERTIES_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_ROM_PROPERTIES_PROVIDER))
#define IS_ROM_PROPERTIES_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_ROM_PROPERTIES_PROVIDER))
#define ROM_PROPERTIES_PROVIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_ROM_PROPERTIES_PROVIDER, RomPropertiesProviderClass))

GType		rom_properties_provider_get_type	(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		rom_properties_provider_register_type	(ThunarxProviderPlugin *plugin) G_GNUC_INTERNAL;

gboolean	rom_properties_get_file_supported	(ThunarxFileInfo *info) G_GNUC_INTERNAL;

G_END_DECLS;

#endif /* !__ROMPROPERTIES_XFCE_ROM_PROPERTIES_PROVIDER_HPP__ */
