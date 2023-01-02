/*****************************************************************************
 * ROM Properties Page shell extension. (GTK4)                               *
 * NautilusPropertiesModelProvider.hpp: Nautilus properties model provider   *
 *                                                                           *
 * NOTE: Nautilus 43 only accepts key/value pairs for properties, instead of *
 * arbitrary GtkWidgets. As such, the properties returned will be more       *
 * limited than in previous versions.                                        *
 *                                                                           *
 * Copyright (c) 2017-2023 by David Korth.                                   *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 *****************************************************************************/

#pragma once

#include "glib-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_NAUTILUS_PROPERTIES_MODEL_PROVIDER (rp_nautilus_properties_model_provider_get_type())
G_DECLARE_FINAL_TYPE(RpNautilusPropertiesModelProvider, rp_nautilus_properties_model_provider, RP, NAUTILUS_PROPERTIES_MODEL_PROVIDER, GObject)

/* this function is implemented automatically by the G_DEFINE_TYPE macro */
/* NOTE: G_DEFINE_DYNAMIC_TYPE() declares the actual function as static. */
void rp_nautilus_properties_model_provider_register_type_ext(GTypeModule *module) G_GNUC_INTERNAL;

G_END_DECLS
