/***************************************************************************
 * ROM Properties Page shell extension. (XFCE)                             *
 * rom-properties-provider.hpp: ThunarX Provider Definition.               *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_XFCE_ROM_PROPERTIES_PROVIDER_HPP__
#define __ROMPROPERTIES_XFCE_ROM_PROPERTIES_PROVIDER_HPP__

#include <glib.h>

// NOTE: thunarx.h doesn't have extern "C" set up properly everywhere.
G_BEGIN_DECLS

// NOTE: Thunar-1.8.0's thunarx-renamer.h depends on GtkVBox,
// which is deprecated in GTK+ 3.x.
#ifdef GTK_DISABLE_DEPRECATED
# undef GTK_DISABLE_DEPRECATED
#endif
#include <thunarx/thunarx.h>

typedef struct _RomPropertiesProviderClass	RomPropertiesProviderClass;
typedef struct _RomPropertiesProvider		RomPropertiesProvider;

#define TYPE_ROM_PROPERTIES_PROVIDER		(rom_properties_provider_get_type())
#define ROM_PROPERTIES_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_ROM_PROPERTIES_PROVIDER, RomPropertiesProvider))
#define ROM_PROPERTIES_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_ROM_PROPERTIES_PROVIDER, RomPropertiesProviderClass))
#define IS_ROM_PROPERTIES_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_ROM_PROPERTIES_PROVIDER))
#define IS_ROM_PROPERTIES_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_ROM_PROPERTIES_PROVIDER))
#define ROM_PROPERTIES_PROVIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_ROM_PROPERTIES_PROVIDER, RomPropertiesProviderClass))

GType		rom_properties_provider_get_type	(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		rom_properties_provider_register_type	(ThunarxProviderPlugin *plugin) G_GNUC_INTERNAL;

gboolean	rom_properties_get_file_supported	(ThunarxFileInfo *info) G_GNUC_INTERNAL;

G_END_DECLS

#endif /* !__ROMPROPERTIES_XFCE_ROM_PROPERTIES_PROVIDER_HPP__ */
