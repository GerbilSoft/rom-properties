/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * NautilusInfoProvider.hpp: Nautilus (and forks) Info Provider Definition *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "glib-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_NAUTILUS_INFO_PROVIDER (rp_nautilus_info_provider_get_type())
G_DECLARE_FINAL_TYPE(RpNautilusInfoProvider, rp_nautilus_info_provider, RP, NAUTILUS_INFO_PROVIDER, GObject)

/* this function is implemented automatically by the G_DEFINE_TYPE macro */
/* NOTE: G_DEFINE_DYNAMIC_TYPE() declares the actual function as static. */
void rp_nautilus_info_provider_register_type_ext(GTypeModule *module) G_GNUC_INTERNAL;

G_END_DECLS
