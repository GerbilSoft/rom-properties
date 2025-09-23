/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * ThunarPlugin.hpp: ThunarX Plugin Definition                             *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

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

struct _ThunarxMenuProviderIface;
typedef struct _ThunarxMenuProviderIface ThunarxMenuProviderIface;

struct _ThunarxMenuProvider;
typedef struct _ThunarxMenuProvider ThunarxMenuProvider;

struct _ThunarxMenuitemIface;
typedef struct _ThunarxMenuitemIface ThunarxMenuitemIface;

struct _ThunarxMenuItem;
typedef struct _ThunarxMenuItem ThunarxMenuItem;

struct _ThunarxPropertyPageProviderIface;
typedef struct _ThunarxPropertyPageProviderIface ThunarxPropertyPageProviderIface;

struct _ThunarxPropertyPageProvider;
typedef struct _ThunarxPropertyPageProvider ThunarxPropertyPageProvider;

struct _ThunarxPropertyPage;
typedef struct _ThunarxPropertyPage ThunarxPropertyPage;

// Function pointer typedefs.
typedef const gchar* (*PFN_THUNARX_CHECK_VERSION)(guint required_major, guint required_micro, guint required_minor);

typedef GType (*PFN_THUNARX_FILE_INFO_GET_TYPE)(void);
typedef gchar* (*PFN_THUNARX_FILE_INFO_GET_MIME_TYPE)(ThunarxFileInfo *file_info);
typedef gchar* (*PFN_THUNARX_FILE_INFO_GET_URI)(ThunarxFileInfo *file);
typedef gchar* (*PFN_THUNARX_FILE_INFO_GET_URI_SCHEME)(ThunarxFileInfo *file);
typedef GList* (*PFN_THUNARX_FILE_INFO_LIST_COPY)(GList *file_infos);
typedef void (*PFN_THUNARX_FILE_INFO_LIST_FREE)(GList *file_infos);

#if GTK_CHECK_VERSION(3, 0, 0)
typedef GType (*PFN_THUNARX_MENU_ITEM_GET_TYPE)(void);
typedef ThunarxMenuItem* (*PFN_THUNARX_MENU_ITEM_NEW)(const gchar *name, const gchar *label, const gchar *tooltip, const gchar *icon) G_GNUC_MALLOC;
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

typedef GType (*PFN_THUNARX_MENU_PROVIDER_GET_TYPE)(void);

typedef GType (*PFN_THUNARX_PROPERTY_PAGE_PROVIDER_GET_TYPE)(void);
typedef GtkWidget* (*PFN_THUNARX_PROPERTY_PAGE_NEW)(const gchar *label);

// Function pointers
extern PFN_THUNARX_CHECK_VERSION			pfn_thunarx_check_version;
extern PFN_THUNARX_FILE_INFO_GET_TYPE			pfn_thunarx_file_info_get_type;
extern PFN_THUNARX_FILE_INFO_GET_MIME_TYPE		pfn_thunarx_file_info_get_mime_type;
extern PFN_THUNARX_FILE_INFO_GET_URI			pfn_thunarx_file_info_get_uri;
extern PFN_THUNARX_FILE_INFO_GET_URI_SCHEME		pfn_thunarx_file_info_get_uri_scheme;
extern PFN_THUNARX_FILE_INFO_LIST_COPY			pfn_thunarx_file_info_list_copy;
extern PFN_THUNARX_FILE_INFO_LIST_FREE			pfn_thunarx_file_info_list_free;
#if GTK_CHECK_VERSION(3, 0, 0)
extern PFN_THUNARX_MENU_ITEM_GET_TYPE			pfn_thunarx_menu_item_get_type;
extern PFN_THUNARX_MENU_ITEM_NEW			pfn_thunarx_menu_item_new;
#endif /* GTK_CHECK_VERSION(3, 0, 0) */
extern PFN_THUNARX_MENU_PROVIDER_GET_TYPE		pfn_thunarx_menu_provider_get_type;
extern PFN_THUNARX_PROPERTY_PAGE_PROVIDER_GET_TYPE	pfn_thunarx_property_page_provider_get_type;
extern PFN_THUNARX_PROPERTY_PAGE_NEW			pfn_thunarx_property_page_new;

// Function pointer macros
#define thunarx_check_version(required_major, required_minor, required_micro) \
	pfn_thunarx_check_version((required_major), (required_minor), (required_micro))
#define thunarx_file_info_get_type()				(pfn_thunarx_file_info_get_type ())
#define thunarx_file_info_get_mime_type(file_info)		(pfn_thunarx_file_info_get_mime_type(file_info))
#define thunarx_file_info_get_uri(file)				(pfn_thunarx_file_info_get_uri(file))
#define thunarx_file_info_get_uri_scheme(file)			(pfn_thunarx_file_info_get_uri_scheme(file))
#define thunarx_file_info_list_copy(file_infos)			(pfn_thunarx_file_info_list_copy(file_infos))
#define thunarx_file_info_list_free(file_infos)			(pfn_thunarx_file_info_list_free(file_infos))
#if GTK_CHECK_VERSION(3, 0, 0)
#define thunarx_menu_item_get_type()				(pfn_thunarx_menu_item_get_type ())
#define thunarx_menu_item_new(name, label, tooltip, icon)	(pfn_thunarx_menu_item_new((name), (label), (tooltip), (icon)))
#endif /* GTK_CHECK_VERSION(3, 0, 0) */
#define thunarx_menu_provider_get_type()			(pfn_thunarx_menu_provider_get_type ())
#define thunarx_property_page_provider_get_type()		(pfn_thunarx_property_page_provider_get_type ())
#define thunarx_property_page_new(label)			(pfn_thunarx_property_page_new(label))

// GType macros.
#define THUNARX_TYPE_FILE_INFO				(pfn_thunarx_file_info_get_type ())
#define THUNARX_FILE_INFO(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), THUNARX_TYPE_FILE_INFO, ThunarxFileInfo))
#define THUNARX_IS_FILE_INFO(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), THUNARX_TYPE_FILE_INFO))
#define THUNARX_FILE_INFO_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj),  THUNARX_TYPE_FILE_INFO, ThunarxFileInfoIface))

#define THUNARX_TYPE_MENU_PROVIDER			(pfn_thunarx_menu_provider_get_type ())
#define THUNARX_MENU_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), THUNARX_TYPE_MENU_PROVIDER, ThunarxMenuProvider))
#define THUNARX_IS_MENU_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), THUNARX_TYPE_MENU_PROVIDER))
#define THUNARX_MENU_PROVIDER_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj),  THUNARX_TYPE_MENU_PROVIDER, ThunarxMenuProviderIface))

#if GTK_CHECK_VERSION(3, 0, 0)
#define THUNARX_TYPE_MENU_ITEM				(pfn_thunarx_menu_item_get_type ())
#define THUNARX_MENU_ITEM(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), THUNARX_TYPE_MENU_ITEM, ThunarxMenuItem))
#define THUNARX_IS_MENU_ITEM(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), THUNARX_TYPE_MENU_ITEM))
#define THUNARX_MENU_ITEM_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj),  THUNARX_TYPE_MENU_ITEM, ThunarxMenuItemIface))
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

#define THUNARX_TYPE_PROPERTY_PAGE_PROVIDER		(pfn_thunarx_property_page_provider_get_type ())
#define THUNARX_PROPERTY_PAGE_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), THUNARX_TYPE_PROPERTY_PAGE_PROVIDER, ThunarxPropertyPageProvider))
#define THUNARX_IS_PROPERTY_PAGE_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), THUNARX_TYPE_PROPERTY_PAGE_PROVIDER))
#define THUNARX_PROPERTY_PAGE_PROVIDER_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE((obj),  THUNARX_TYPE_PROPERTY_PAGE_PROVIDER, ThunarxPropertyPageProviderIface))

G_END_DECLS;
