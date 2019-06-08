/***************************************************************************
 * ROM Properties Page shell extension. (GNOME)                            *
 * rom-properties-provider.hpp: Nautilus Provider Definition.              *
 *                                                                         *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GNOME_ROM_PROPERTIES_PROVIDER_HPP__
#define __ROMPROPERTIES_GNOME_ROM_PROPERTIES_PROVIDER_HPP__

#if defined(RP_UI_GTK3_GNOME)
// GNOME 3 desktop
# include "config.gnome.h"
# ifdef HAVE_NAUTILUS_EXTENSION_H
// introduced with Nautilus 3.30
#  include <nautilus-extension.h>
# else /* !HAVE_NAUTILUS_EXTENSION_H */
#  include <libnautilus-extension/nautilus-property-page-provider.h>
# endif /* HAVE_NAUTILUS_EXTENSION_H */
typedef NautilusFileInfo RpGtk3FileInfo;
#elif defined(RP_UI_GTK3_MATE)
// MATE desktop (v1.18.0+; GTK+ 3.x)
# include <libcaja-extension/caja-property-page-provider.h>
typedef CajaFileInfo RpGtk3FileInfo;
#else
# error GTK3 desktop environment not set and/or supported.
#endif

G_BEGIN_DECLS;

typedef struct _RomPropertiesProviderClass	RomPropertiesProviderClass;
typedef struct _RomPropertiesProvider		RomPropertiesProvider;

#define TYPE_ROM_PROPERTIES_PROVIDER		(rom_properties_provider_get_type())
#define ROM_PROPERTIES_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_ROM_PROPERTIES_PROVIDER, RomPropertiesProvider))
#define ROM_PROPERTIES_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_ROM_PROPERTIES_PROVIDER, RomPropertiesProviderClass))
#define IS_ROM_PROPERTIES_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_ROM_PROPERTIES_PROVIDER))
#define IS_ROM_PROPERTIES_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_ROM_PROPERTIES_PROVIDER))
#define ROM_PROPERTIES_PROVIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_ROM_PROPERTIES_PROVIDER, RomPropertiesProviderClass))

/* these two functions are implemented automatically by the G_DEFINE_DYNAMIC_TYPE macro */
GType		rom_properties_provider_get_type	(void) G_GNUC_CONST G_GNUC_INTERNAL;
/* NOTE: G_DEFINE_DYNAMIC_TYPE() declares the actual function as static. */
void		rom_properties_provider_register_type_ext(GTypeModule *module) G_GNUC_INTERNAL;

gboolean	rom_properties_get_file_supported	(RpGtk3FileInfo *info) G_GNUC_INTERNAL;

G_END_DECLS;

#endif /* !__ROMPROPERTIES_GNOME_ROM_PROPERTIES_PROVIDER_HPP__ */
