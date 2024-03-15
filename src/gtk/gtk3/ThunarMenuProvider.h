/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * ThunarMenuProvider.h: ThunarX Menu Provider Definition                  *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "glib-compat.h"
#include "ThunarPlugin.hpp"

G_BEGIN_DECLS

#define RP_TYPE_THUNAR_MENU_PROVIDER (rp_thunar_menu_provider_get_type())
G_DECLARE_FINAL_TYPE(RpThunarMenuProvider, rp_thunar_menu_provider, RP, THUNAR_MENU_PROVIDER, GObject)

/* this function is implemented automatically by the G_DEFINE_TYPE macro */
/* NOTE: G_DEFINE_DYNAMIC_TYPE() declares the actual function as static. */
void rp_thunar_menu_provider_register_type_ext(ThunarxProviderPlugin *plugin) G_GNUC_INTERNAL;

G_END_DECLS
