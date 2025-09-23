/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * NautilusPlugin.hpp: Nautilus (and forks) Plugin Definition              *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

// Opaque structs
struct _NautilusFileInfoInterface;
typedef struct _NautilusFileInfoInterface NautilusFileInfoInterface;

struct _NautilusFileInfo;
typedef struct _NautilusFileInfo NautilusFileInfo;

struct _NautilusMenuItem;
typedef struct _NautilusMenuProvider NautilusMenuItem;

struct _NautilusMenuProviderInterface;
typedef struct _NautilusMenuProviderInterface NautilusMenuProviderInterface;

struct _NautilusMenuProvider;
typedef struct _NautilusMenuProvider NautilusMenuProvider;

struct _NautilusPropertyPageProviderInterface;
typedef struct _NautilusPropertyPageProviderInterface NautilusPropertyPageProviderInterface;

struct _NautilusPropertyPageProvider;
typedef struct _NautilusPropertyPageProvider NautilusPropertyPageProvider;

struct _NautilusPropertyPage;
typedef struct _NautilusPropertyPage NautilusPropertyPage;

struct _NautilusOperationHandle;
typedef struct _NautilusOperationHandle NautilusOperationHandle;

struct _NautilusInfoProvider;
typedef struct _NautilusInfoProvider NautilusInfoProvider;

struct _NautilusColumn;
typedef struct _NautilusColumn NautilusColumn;

struct _NautilusColumnProvider;
typedef struct _NautilusColumnProvider NautilusColumnProvider;

typedef enum {
	/* Returned if the call succeeded, and the extension is done 
	 * with the request */
	NAUTILUS_OPERATION_COMPLETE,

	/* Returned if the call failed */
	NAUTILUS_OPERATION_FAILED,

	/* Returned if the extension has begun an async operation. 
	 * If this is returned, the extension must set the handle 
	 * parameter and call the callback closure when the 
	 * operation is complete. */
	NAUTILUS_OPERATION_IN_PROGRESS
} NautilusOperationResult;

// Function pointer typedefs
typedef GType (*PFN_NAUTILUS_FILE_INFO_GET_TYPE)(void);
typedef char* (*PFN_NAUTILUS_FILE_INFO_GET_URI)(NautilusFileInfo *file_info);
typedef char* (*PFN_NAUTILUS_FILE_INFO_GET_URI_SCHEME)(NautilusFileInfo *file_info);
typedef gchar* (*PFN_NAUTILUS_FILE_INFO_GET_MIME_TYPE)(NautilusFileInfo *file_info);
typedef void (*PFN_NAUTILUS_FILE_INFO_ADD_EMBLEM)(NautilusFileInfo *file_info, const char *emblem_name);
typedef void (*PFN_NAUTILUS_FILE_INFO_ADD_STIRNG_ATTRIBUTE)(NautilusFileInfo *file_info, const char *attribute_name, const char *value);

typedef GList* (*PFN_NAUTILUS_FILE_INFO_LIST_COPY)(GList *files);
typedef void (*PFN_NAUTILUS_FILE_INFO_LIST_FREE)(GList *files);

typedef GType (*PFN_NAUTILUS_MENU_PROVIDER_GET_TYPE)(void);

typedef GType (*PFN_NAUTILUS_MENU_ITEM_GET_TYPE)(void);
typedef NautilusMenuItem* (*PFN_NAUTILUS_MENU_ITEM_NEW)(const char *name, const char *label, const char *tip, const char *icon);

typedef GType (*PFN_NAUTILUS_PROPERTY_PAGE_PROVIDER_GET_TYPE)(void);
typedef NautilusPropertyPage* (*PFN_NAUTILUS_PROPERTY_PAGE_NEW)(const char *name, GtkWidget *label, GtkWidget *page);

typedef GType (*PFN_NAUTILUS_INFO_PROVIDER_GET_TYPE)(void);
typedef void (*PFN_NAUTILUS_INFO_PROVIDER_UPDATE_COMPLETE_INVOKE)(GClosure *update_complete, NautilusInfoProvider *provider, NautilusOperationHandle *handle, NautilusOperationResult result);

typedef GType (*PFN_NAUTILUS_COLUMN_GET_TYPE)(void);
typedef NautilusColumn* (*PFN_NAUTILUS_COLUMN_NEW)(const char *name, const char *attribute, const char *label, const char *description);

typedef GType (*PFN_NAUTILUS_COLUMN_PROVIDER_GET_TYPE)(void);
typedef GList* (*PFN_NAUTILUS_COLUMN_PROVIDER_GET_COLUMNS)(NautilusColumnProvider *provider);

// Function pointers
extern PFN_NAUTILUS_FILE_INFO_GET_TYPE				pfn_nautilus_file_info_get_type;
extern PFN_NAUTILUS_FILE_INFO_GET_URI				pfn_nautilus_file_info_get_uri;
extern PFN_NAUTILUS_FILE_INFO_GET_URI_SCHEME			pfn_nautilus_file_info_get_uri_scheme;
extern PFN_NAUTILUS_FILE_INFO_GET_MIME_TYPE			pfn_nautilus_file_info_get_mime_type;
extern PFN_NAUTILUS_FILE_INFO_ADD_EMBLEM			pfn_nautilus_file_info_add_emblem;
extern PFN_NAUTILUS_FILE_INFO_ADD_STIRNG_ATTRIBUTE		pfn_nautilus_file_info_add_string_attribute;
extern PFN_NAUTILUS_FILE_INFO_LIST_COPY				pfn_nautilus_file_info_list_copy;
extern PFN_NAUTILUS_FILE_INFO_LIST_FREE				pfn_nautilus_file_info_list_free;
extern PFN_NAUTILUS_MENU_ITEM_GET_TYPE				pfn_nautilus_menu_item_get_type;
extern PFN_NAUTILUS_MENU_ITEM_NEW				pfn_nautilus_menu_item_new;
extern PFN_NAUTILUS_MENU_PROVIDER_GET_TYPE			pfn_nautilus_menu_provider_get_type;
extern PFN_NAUTILUS_PROPERTY_PAGE_PROVIDER_GET_TYPE		pfn_nautilus_property_page_provider_get_type;
extern PFN_NAUTILUS_PROPERTY_PAGE_NEW				pfn_nautilus_property_page_new;
extern PFN_NAUTILUS_INFO_PROVIDER_GET_TYPE			pfn_nautilus_info_provider_get_type;
extern PFN_NAUTILUS_INFO_PROVIDER_UPDATE_COMPLETE_INVOKE	pfn_nautilus_info_provider_update_complete_invoke;
extern PFN_NAUTILUS_COLUMN_GET_TYPE				pfn_nautilus_column_get_type;
extern PFN_NAUTILUS_COLUMN_NEW					pfn_nautilus_column_new;
extern PFN_NAUTILUS_COLUMN_PROVIDER_GET_TYPE			pfn_nautilus_column_provider_get_type;
extern PFN_NAUTILUS_COLUMN_PROVIDER_GET_COLUMNS			pfn_nautilus_column_provider_get_columns;

// Function pointer macros
#define nautilus_file_info_get_type()				(pfn_nautilus_file_info_get_type ())
#define nautilus_file_info_get_uri(file_info)			(pfn_nautilus_file_info_get_uri(file_info))
#define nautilus_file_info_get_uri_scheme(file_info)		(pfn_nautilus_file_info_get_uri_scheme(file_info))
#define nautilus_file_info_get_mime_type(file_info)		(pfn_nautilus_file_info_get_mime_type(file_info))
#define nautilus_file_info_add_emblem(file_info, emblem_name)	(pfn_nautilus_file_info_add_emblem((file_info), (emblem_name)))
#define nautilus_file_info_add_string_attribute(file_info, attribute_name, value) \
	(pfn_nautilus_file_info_add_string_attribute((file_info), (attribute_name), (value)))
#define nautilus_file_info_list_copy(files)			(pfn_nautilus_file_info_list_copy(files))
#define nautilus_file_info_list_free(files)			(pfn_nautilus_file_info_list_free(files))
#define nautilus_menu_item_get_type()				(pfn_nautilus_menu_item_get_type ())
#define nautilus_menu_item_new(name, label, tip, icon)		(pfn_nautilus_menu_item_new((name), (label), (tip), (icon)))
#define nautilus_menu_provider_get_type()			(pfn_nautilus_menu_provider_get_type ())
#define nautilus_property_page_provider_get_type()		(pfn_nautilus_property_page_provider_get_type ())
#define nautilus_property_page_new(name, label, page)		(pfn_nautilus_property_page_new((name), (label), (page)))
#define nautilus_info_provider_get_type()			(pfn_nautilus_info_provider_get_type ())
#define nautilus_info_provider_update_complete_invoke(update_complete, provider, handle, result) \
	(pfn_nautilus_info_provider_update_complete_invoke((update_complete), (provider), (handle), (result)))
#define nautilus_column_get_type()				(pfn_nautilus_column_get_type ())
#define nautilus_column_new(name, attribute, label, description) \
	(pfn_nautilus_column_new((name), (attribute), (label), (description)))
#define nautilus_column_provider_get_type()			(pfn_nautilus_column_provider_get_type ())
#define nautilus_column_provider_get_columns(provider)		(pfn_nautilus_column_provider_get_columns(provider))

// GType macros
#define NAUTILUS_TYPE_FILE_INFO				(pfn_nautilus_file_info_get_type ())
#define NAUTILUS_FILE_INFO(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), NAUTILUS_TYPE_FILE_INFO, NautilusFileInfo))
#define NAUTILUS_IS_FILE_INFO(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), NAUTILUS_TYPE_FILE_INFO))
#define NAUTILUS_FILE_INFO_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj),  NAUTILUS_TYPE_FILE_INFO, NautilusFileInfoInterface))

#define NAUTILUS_TYPE_MENU_ITEM				(pfn_nautilus_menu_item_get_type ())
#define NAUTILUS_MENU_ITEM(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), NAUTILUS_TYPE_MENU_ITEM, NautilusMenuItem))
#define NAUTILUS_IS_MENU_ITEM(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), NAUTILUS_TYPE_MENU_ITEM))
#define NAUTILUS_MENU_ITEM_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj),  NAUTILUS_TYPE_MENU_ITEM, NautilusMenuItemInterface))

#define NAUTILUS_TYPE_MENU_PROVIDER			(pfn_nautilus_menu_provider_get_type ())
#define NAUTILUS_MENU_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), NAUTILUS_TYPE_MENU_PROVIDER, NautilusMenuProvider))
#define NAUTILUS_IS_MENU_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), NAUTILUS_TYPE_MENU_PROVIDER))
#define NAUTILUS_MENU_PROVIDER_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj),  NAUTILUS_TYPE_MENU_PROVIDER, NautilusMenuProviderInterface))

#define NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER		(pfn_nautilus_property_page_provider_get_type ())
#define NAUTILUS_PROPERTY_PAGE_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER, NautilusPropertyPageProvider))
#define NAUTILUS_IS_PROPERTY_PAGE_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER))
#define NAUTILUS_PROPERTY_PAGE_PROVIDER_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE((obj),  NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER, NautilusPropertyPageProviderInterface))

#define NAUTILUS_TYPE_INFO_PROVIDER			(pfn_nautilus_info_provider_get_type ())
#define NAUTILUS_INFO_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), NAUTILUS_TYPE_INFO_PROVIDER, NautilusInfoProvider))
#define NAUTILUS_IS_INFO_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), NAUTILUS_TYPE_INFO_PROVIDER))
#define NAUTILUS_INFO_PROVIDER_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj),  NAUTILUS_TYPE_INFO_PROVIDER, NautilusInfoProviderInterface))

#define NAUTILUS_TYPE_COLUMN				(pfn_nautilus_column_provider_get_type ())
#define NAUTILUS_COLUMN(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), NAUTILUS_TYPE_COLUMN, NautilusColumn))
#define NAUTILUS_IS_COLUMN(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), NAUTILUS_TYPE_COLUMN))
#define NAUTILUS_COLUMN_GET_IFACE(obj)			(G_TYPE_INSTANCE_GET_INTERFACE((obj),  NAUTILUS_TYPE_COLUMN, NautilusColumnInterface))

#define NAUTILUS_TYPE_COLUMN_PROVIDER			(pfn_nautilus_column_provider_get_type ())
#define NAUTILUS_COLUMN_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), NAUTILUS_TYPE_COLUMN_PROVIDER, NautilusColumnProvider))
#define NAUTILUS_IS_COLUMN_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), NAUTILUS_TYPE_COLUMN_PROVIDER))
#define NAUTILUS_COLUMN_PROVIDER_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj),  NAUTILUS_TYPE_COLUMN_PROVIDER, NautilusColumnProviderInterface))

G_END_DECLS
