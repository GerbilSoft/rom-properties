/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * NautilusMenuProvider.h: Nautilus (and forks) Menu Provider Definition   *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "glib-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_NAUTILUS_MENU_PROVIDER (rp_nautilus_menu_provider_get_type())
G_DECLARE_FINAL_TYPE(RpNautilusMenuProvider, rp_nautilus_menu_provider, RP, NAUTILUS_MENU_PROVIDER, GObject)

/* this function is implemented automatically by the G_DEFINE_TYPE macro */
/* NOTE: G_DEFINE_DYNAMIC_TYPE() declares the actual function as static. */
void rp_nautilus_menu_provider_register_type_ext(GTypeModule *module) G_GNUC_INTERNAL;

G_END_DECLS
