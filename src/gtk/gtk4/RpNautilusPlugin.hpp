/***************************************************************************
 * ROM Properties Page shell extension. (GTK4)                             *
 * RpNautilusPlugin.cpp: Nautilus GTK4 Plugin Definition.                  *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK4_RPNAUTILUSPLUGIN_HPP__
#define __ROMPROPERTIES_GTK4_RPNAUTILUSPLUGIN_HPP__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS;

// Opaque structs.
struct _NautilusFileInfoInterface;
typedef struct _NautilusFileInfoInterface NautilusFileInfoInterface;

struct _NautilusFileInfo;
typedef struct _NautilusFileInfo NautilusFileInfo;

struct _NautilusPropertiesModelProviderInterface;
typedef struct _NautilusPropertiesModelProviderInterface NautilusPropertiesModelProviderInterface;

struct _NautilusPropertiesModelProvider;
typedef struct _NautilusPropertiesModelProvider NautilusPropertiesModelProvider;

struct _NautilusPropertiesModel;
typedef struct _NautilusPropertiesModel NautilusPropertiesModel;

struct _NautilusPropertiesItem;
typedef struct _NautilusPropertiesItem NautilusPropertiesItem;

// Function pointer typedefs
typedef GType (*PFN_NAUTILUS_FILE_INFO_GET_TYPE)(void);
typedef char* (*PFN_NAUTILUS_FILE_INFO_GET_URI)(NautilusFileInfo *file);

typedef GType (*PFN_NAUTILUS_PROPERTIES_MODEL_PROVIDER_GET_TYPE)(void);
typedef GType (*PFN_NAUTILUS_PROPERTIES_MODEL_GET_TYPE)(void);
typedef NautilusPropertiesModel* (*PFN_NAUTILUS_PROPERTIES_MODEL_NEW)(const char *title, GListModel *model);

typedef GType (*PFN_NAUTILUS_PROPERTIES_ITEM_GET_TYPE)(void);
typedef NautilusPropertiesItem* (*PFN_NAUTILUS_PROPERTIES_ITEM_NEW)(const char *name, const char *value);

// Function pointers
extern PFN_NAUTILUS_FILE_INFO_GET_TYPE			pfn_nautilus_file_info_get_type;
extern PFN_NAUTILUS_FILE_INFO_GET_URI			pfn_nautilus_file_info_get_uri;
extern PFN_NAUTILUS_PROPERTIES_MODEL_PROVIDER_GET_TYPE	pfn_nautilus_properties_model_provider_get_type;
extern PFN_NAUTILUS_PROPERTIES_MODEL_GET_TYPE		pfn_nautilus_properties_model_get_type;
extern PFN_NAUTILUS_PROPERTIES_MODEL_NEW		pfn_nautilus_properties_model_new;
extern PFN_NAUTILUS_PROPERTIES_ITEM_GET_TYPE		pfn_nautilus_properties_item_get_type;
extern PFN_NAUTILUS_PROPERTIES_ITEM_NEW			pfn_nautilus_properties_item_new;

// Function pointer macros
#define nautilus_file_info_get_type()			(pfn_nautilus_file_info_get_type ())
#define nautilus_file_info_get_uri(file)		(pfn_nautilus_file_info_get_uri(file))
#define nautilus_properties_model_provider_get_type()	(pfn_nautilus_properties_model_provider_get_type ())
#define nautilus_properties_model_get_type()		(pfn_nautilus_properties_model_get_type ())
#define nautilus_properties_model_new(title, model)	(pfn_nautilus_properties_model_new((title), (model)))
#define nautilus_properties_item_get_type()		(pfn_nautilus_properties_item_get_type ())
#define nautilus_properties_item_new(name, value)	(pfn_nautilus_properties_item_new((name), (value)))

// GType macros
#define NAUTILUS_TYPE_FILE_INFO				(pfn_nautilus_file_info_get_type ())
#define NAUTILUS_FILE_INFO(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), NAUTILUS_TYPE_FILE_INFO, NautilusFileInfo))
#define NAUTILUS_IS_FILE_INFO(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), NAUTILUS_TYPE_FILE_INFO))
#define NAUTILUS_FILE_INFO_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj),  NAUTILUS_TYPE_FILE_INFO, NautilusFileInfoInterface))

#define NAUTILUS_TYPE_PROPERTIES_MODEL_PROVIDER		(pfn_nautilus_properties_model_provider_get_type ())
#define NAUTILUS_PROPERTIES_MODEL_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), NAUTILUS_TYPE_PROPERTIES_MODEL_PROVIDER, NautilusPropertiesModelProvider))
#define NAUTILUS_IS_PROPERTIES_MODEL_PROVIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), NAUTILUS_TYPE_PROPERTIES_MODEL_PROVIDER))
#define NAUTILUS_PROPERTIES_MODEL_PROVIDER_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE((obj),  NAUTILUS_TYPE_PROPERTIES_MODEL_PROVIDER, NautilusPropertiesModelProviderInterface))

#define NAUTILUS_TYPE_PROPERTIES_MODEL			(pfn_nautilus_properties_model_get_type ())
#define NAUTILUS_PROPERTIES_MODEL(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), NAUTILUS_TYPE_PROPERTIES_MODEL, NautilusPropertiesModel))
#define NAUTILUS_IS_PROPERTIES_MODEL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), NAUTILUS_TYPE_PROPERTIES_MODEL))

#define NAUTILUS_TYPE_PROPERTIES_ITEM			(pfn_nautilus_properties_item_get_type ())
#define NAUTILUS_PROPERTIES_ITEM(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), NAUTILUS_TYPE_PROPERTIES_ITEM, NautilusPropertiesItem))
#define NAUTILUS_IS_PROPERTIES_ITEM(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), NAUTILUS_TYPE_PROPERTIES_ITEM))

G_END_DECLS;

#endif /* __ROMPROPERTIES_GTK4_RPNAUTILUSPLUGIN_HPP__ */
