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

#ifndef __ROMPROPERTIES_GTK4_RPNAUTILUSPROPERTIESMODEL_HPP__
#define __ROMPROPERTIES_GTK4_RPNAUTILUSPROPERTIESMODEL_HPP__

#include <glib.h>
#include <glib-object.h>

#include "RpNautilusPlugin.hpp"

#ifdef __cplusplus

#include "librpbase/RomData.hpp"

NautilusPropertiesModel *rp_nautilus_properties_model_new(const LibRpBase::RomData *romData);

#endif /* __cplusplus */

#endif /* !__ROMPROPERTIES_GTK4_RPNAUTILUSPROPERTIESMODEL_HPP__ */
