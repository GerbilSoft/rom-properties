/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * RpThunarPlugin.h: ThunarX Plugin Definition.                            *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK3_RPTHUNARPLUGIN_H__
#define __ROMPROPERTIES_GTK3_RPTHUNARPLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS;

// Opaque structs.
struct _ThunarxProviderPlugin;
typedef struct _ThunarxProviderPlugin ThunarxProviderPlugin;

struct _ThunarxFileInfoIface;
typedef struct _ThunarxFileInfoIface ThunarxFileInfoIface;

struct _ThunarxFileInfo;
typedef struct _ThunarxFileInfo ThunarxFileInfo;

struct _ThunarxPropertyPageProviderIface;
typedef struct _ThunarxPropertyPageProviderIface ThunarxPropertyPageProviderIface;

struct _ThunarxPropertyPageProvider;
typedef struct _ThunarxPropertyPageProvider ThunarxPropertyPageProvider;

struct _ThunarxPropertyPage;
typedef struct _ThunarxPropertyPage ThunarxPropertyPage;

// Function pointer typedefs.
typedef const gchar* (*PFN_THUNARX_CHECK_VERSION)(guint required_major, guint required_micro, guint required_minor);

typedef GType (*PFN_THUNARX_PROPERTY_PAGE_PROVIDER_GET_TYPE)(void);
typedef GtkWidget* (*PFN_THUNARX_PROPERTY_PAGE_NEW)(const gchar *label);
typedef GType (*PFN_THUNARX_PROPERTY_PAGE_GET_TYPE)(void);

typedef GType (*PFN_THUNARX_FILE_INFO_GET_TYPE)(void);
typedef gchar* (*PFN_THUNARX_FILE_INFO_GET_URI)(ThunarxFileInfo *file);

// Function pointers.
extern PFN_THUNARX_CHECK_VERSION pfn_thunarx_check_version;

extern PFN_THUNARX_PROPERTY_PAGE_PROVIDER_GET_TYPE pfn_thunarx_property_page_provider_get_type;
extern PFN_THUNARX_PROPERTY_PAGE_NEW pfn_thunarx_property_page_new;
extern PFN_THUNARX_PROPERTY_PAGE_GET_TYPE pfn_thunarx_property_page_get_type;

extern PFN_THUNARX_FILE_INFO_GET_TYPE pfn_thunarx_file_info_get_type;
extern PFN_THUNARX_FILE_INFO_GET_URI pfn_thunarx_file_info_get_uri;

// Function pointer macros.
#define thunarx_check_version(required_major, required_minor, required_micro) \
	pfn_thunarx_check_version((required_major), (required_minor), (required_micro))

#define thunarx_property_page_provider_get_type()	(pfn_thunarx_property_page_provider_get_type ())
#define thunarx_property_page_new(label)		(pfn_thunarx_property_page_new(label))
#define thunarx_property_page_get_type()		(pfn_thunarx_property_page_get_type ())

#define thunarx_file_info_get_type()			(pfn_thunarx_file_info_get_type ())
#define thunarx_file_info_get_uri(file)			(pfn_thunarx_file_info_get_uri(file))

// GType macros.
#define THUNARX_TYPE_PROPERTY_PAGE_PROVIDER		(pfn_thunarx_property_page_provider_get_type ())
#define THUNARX_PROPERTY_PAGE_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), THUNARX_TYPE_PROPERTY_PAGE_PROVIDER, ThunarxPropertyPageProvider))
#define THUNARX_IS_PROPERTY_PAGE_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), THUNARX_TYPE_PROPERTY_PAGE_PROVIDER))
#define THUNARX_PROPERTY_PAGE_PROVIDER_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE((obj),  THUNARX_TYPE_PROPERTY_PAGE_PROVIDER, ThunarxPropertyPageProviderIface))

#define THUNARX_TYPE_PROPERTY_PAGE			(pfn_thunarx_property_page_get_type ())
#define THUNARX_PROPERTY_PAGE(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), THUNARX_TYPE_PROPERTY_PAGE, ThunarxPropertyPage))
#define THUNARX_IS_PROPERTY_PAGE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), THUNARX_TYPE_PROPERTY_PAGE))
#define THUNARX_PROPERTY_PAGE_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj),  THUNARX_TYPE_PROPERTY_PAGE, ThunarxPropertyPageIface))

#define THUNARX_TYPE_FILE_INFO				(pfn_thunarx_file_info_get_type ())
#define THUNARX_FILE_INFO(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), THUNARX_TYPE_FILE_INFO, ThunarxFileInfo))
#define THUNARX_IS_FILE_INFO(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), THUNARX_TYPE_FILE_INFO))
#define THUNARX_FILE_INFO_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj),  THUNARX_TYPE_FILE_INFO, ThunarxFileInfoIface))

G_END_DECLS;

#endif /* __ROMPROPERTIES_GTK3_RPTHUNARPLUGIN_H__ */
