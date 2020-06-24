/***************************************************************************
 * ROM Properties Page shell extension. (GNOME)                            *
 * RpNautilusPlugin.h: Nautilus (and forks) Plugin Definition.             *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK3_RPNAUTILUSPLUGIN_H__
#define __ROMPROPERTIES_GTK3_RPNAUTILUSPLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS;

// Opaque structs.
struct _NautilusFileInfoIface;
typedef struct _NautilusFileInfoIface NautilusFileInfoIface;

struct _NautilusFileInfo;
typedef struct _NautilusFileInfo NautilusFileInfo;

struct _NautilusPropertyPageProviderIface;
typedef struct _NautilusPropertyPageProviderIface NautilusPropertyPageProviderIface;

struct _NautilusPropertyPageProvider;
typedef struct _NautilusPropertyPageProvider NautilusPropertyPageProvider;

struct _NautilusPropertyPage;
typedef struct _NautilusPropertyPage NautilusPropertyPage;

// Function pointer typedefs.
typedef GType (*PFN_NAUTILUS_FILE_INFO_GET_TYPE)(void);
typedef char* (*PFN_NAUTILUS_FILE_INFO_GET_URI)(NautilusFileInfo *file);

typedef GType (*PFN_NAUTILUS_PROPERTY_PAGE_PROVIDER_GET_TYPE)(void);
typedef NautilusPropertyPage* (*PFN_NAUTILUS_PROPERTY_PAGE_NEW)(const char *name, GtkWidget *label, GtkWidget *page);

// Function pointers.
extern PFN_NAUTILUS_FILE_INFO_GET_TYPE pfn_nautilus_file_info_get_type;
extern PFN_NAUTILUS_FILE_INFO_GET_URI pfn_nautilus_file_info_get_uri;
extern PFN_NAUTILUS_PROPERTY_PAGE_PROVIDER_GET_TYPE pfn_nautilus_property_page_provider_get_type;
extern PFN_NAUTILUS_PROPERTY_PAGE_NEW pfn_nautilus_property_page_new;

// Function pointer macros.
#define nautilus_file_info_get_type()			pfn_nautilus_file_info_get_type()
#define nautilus_file_info_get_uri(file)		pfn_nautilus_file_info_get_uri(file)
#define nautilus_property_page_provider_get_type()	pfn_nautilus_property_page_provider_get_type()
#define nautilus_property_page_new(name, label, page)	pfn_nautilus_property_page_new((name), (label), (page))

// GType macros.
#define NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER		(pfn_nautilus_property_page_provider_get_type ())
#define NAUTILUS_PROPERTY_PAGE_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER, NautilusPropertyPageProvider))
#define NAUTILUS_IS_PROPERTY_PAGE_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER))
#define NAUTILUS_PROPERTY_PAGE_PROVIDER_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE((obj),  NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER, NautilusPropertyPageProviderIface))

#define NAUTILUS_TYPE_FILE_INFO				(pfn_nautilus_file_info_get_type ())
#define NAUTILUS_FILE_INFO(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), NAUTILUS_TYPE_FILE_INFO, NautilusFileInfo))
#define NAUTILUS_IS_FILE_INFO(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), NAUTILUS_TYPE_FILE_INFO))
#define NAUTILUS_FILE_INFO_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj),  NAUTILUS_TYPE_FILE_INFO, NautilusFileInfoIface))

G_END_DECLS;

#endif /* __ROMPROPERTIES_GTK3_RPNAUTILUSPLUGIN_H__ */
