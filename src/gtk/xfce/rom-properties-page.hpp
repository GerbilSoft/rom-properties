/***************************************************************************
 * ROM Properties Page shell extension. (XFCE)                             *
 * rom-properties-page.hpp: ThunarX Properties Page.                       *
 *                                                                         *
 * Copyright (c) 2017-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_XFCE_ROM_PROPERTIES_PAGE_HPP__
#define __ROMPROPERTIES_XFCE_ROM_PROPERTIES_PAGE_HPP__

#include <glib.h>

// NOTE: thunarx.h doesn't have extern "C" set up properly everywhere.
G_BEGIN_DECLS;

// NOTE: Thunar-1.8.0's thunarx-renamer.h depends on GtkVBox,
// which is deprecated in GTK+ 3.x.
#ifdef GTK_DISABLE_DEPRECATED
# undef GTK_DISABLE_DEPRECATED
#endif
#include <thunarx/thunarx.h>

typedef struct _RomPropertiesPageClass	RomPropertiesPageClass;
typedef struct _RomPropertiesPage	RomPropertiesPage;

#define TYPE_ROM_PROPERTIES_PAGE            (rom_properties_page_get_type())
#define ROM_PROPERTIES_PAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_ROM_PROPERTIES_PAGE, RomPropertiesPage))
#define ROM_PROPERTIES_PAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_ROM_PROPERTIES_PAGE, RomPropertiesPageClass))
#define IS_ROM_PROPERTIES_PAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_ROM_PROPERTIES_PAGE))
#define IS_ROM_PROPERTIES_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_ROM_PROPERTIES_PAGE))
#define ROM_PROPERTIES_PAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_ROM_PROPERTIES_PAGE, RomPropertiesPageClass))

/* these two functions are implemented automatically by the THUNARX_DEFINE_TYPE macro */
GType			rom_properties_page_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void			rom_properties_page_register_type	(ThunarxProviderPlugin *plugin) G_GNUC_INTERNAL;

RomPropertiesPage	*rom_properties_page_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

ThunarxFileInfo		*rom_properties_page_get_file		(RomPropertiesPage	*page) G_GNUC_INTERNAL;
void			rom_properties_page_set_file		(RomPropertiesPage	*page,
								 ThunarxFileInfo	*file) G_GNUC_INTERNAL;

G_END_DECLS;

#endif /* __ROMPROPERTIES_XFCE_ROM_PROPERTIES_PAGE_HPP__ */
