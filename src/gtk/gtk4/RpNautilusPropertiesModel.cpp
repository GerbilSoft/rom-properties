/*****************************************************************************
 * ROM Properties Page shell extension. (GTK4)                               *
 * RpNautilusPropertiesModel.hpp: Nautilus properties model                  *
 *                                                                           *
 * NOTE: Nautilus 43 only accepts key/value pairs for properties, instead of *
 * arbitrary GtkWidgets. As such, the properties returned will be more       *
 * limited than in previous versions.                                        *
 *                                                                           *
 * Copyright (c) 2017-2022 by David Korth.                                   *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 *****************************************************************************/

#include "stdafx.h"
#include "RpNautilusPropertiesModel.hpp"
#include "libi18n/i18n.h"

// librpbase
#include "librpbase/RomData.hpp"
using LibRpBase::RomData;

// Internal data.
// Based on Nautilus 43's image-extension.
// Reference: https://github.com/GNOME/nautilus/blob/43.0/extensions/image-properties/nautilus-image-properties-model.c
typedef struct _RpNautilusPropertiesModel {
	GListStore *listStore;
} RpNautilusPropertiesModel;

static void
rp_nautilus_properties_model_free(RpNautilusPropertiesModel *self)
{
	g_free(self);
}

static void
append_item(RpNautilusPropertiesModel *self,
            const char                *name,
            const char                *value)
{
	NautilusPropertiesItem *const item = nautilus_properties_item_new(name, value);
	g_list_store_append(self->listStore, item);
	g_object_unref(item);
}

static void
rp_nautilus_properties_model_init(RpNautilusPropertiesModel *self)
{
	self->listStore = g_list_store_new(NAUTILUS_TYPE_PROPERTIES_ITEM);
}

static void
rp_nautilus_properties_model_load_from_romData(RpNautilusPropertiesModel *self,
                                               const RomData             *romData)
{
	// NOTE: Not taking a reference to RomData.

	// TODO: Process RomData.
	// TODO: Asynchronous field loading. (separate thread?)
	// If implemented, check Nautilus 43's image-extension for cancellable.
	g_list_store_append(self->listStore, nautilus_properties_item_new("RP Name 2", "RP Value 2"));
}

NautilusPropertiesModel *
rp_nautilus_properties_model_new(const RomData *romData)
{
	RpNautilusPropertiesModel *const self = g_new0 (RpNautilusPropertiesModel, 1);

	rp_nautilus_properties_model_init(self);
	rp_nautilus_properties_model_load_from_romData(self, romData);

	NautilusPropertiesModel *model = nautilus_properties_model_new(
		C_("RomDataView", "ROM Properties"), G_LIST_MODEL(self->listStore));

	g_object_weak_ref(G_OBJECT(model), (GWeakNotify)rp_nautilus_properties_model_free, self);

	return model;
}
