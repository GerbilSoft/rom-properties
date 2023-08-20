/*****************************************************************************
 * ROM Properties Page shell extension. (GTK4)                               *
 * NautilusPropertiesModel.hpp: Nautilus properties model                    *
 *                                                                           *
 * NOTE: Nautilus 43 only accepts key/value pairs for properties, instead of *
 * arbitrary GtkWidgets. As such, the properties returned will be more       *
 * limited than in previous versions.                                        *
 *                                                                           *
 * Copyright (c) 2017-2023 by David Korth.                                   *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 *****************************************************************************/

#pragma once

#include <glib.h>
#include <glib-object.h>

#include "NautilusPlugin.hpp"

#ifdef __cplusplus

#include "librpbase/RomData.hpp"

NautilusPropertiesModel *rp_nautilus_properties_model_new(const LibRpBase::RomDataPtr &romData);

#endif /* __cplusplus */
