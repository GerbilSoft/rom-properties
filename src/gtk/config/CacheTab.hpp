/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CacheTab.hpp: Thumbnail Cache tab for rp-config.                        *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_CACHE_TAB (rp_cache_tab_get_type())

#if GTK_CHECK_VERSION(3,0,0)
#  define _RpCacheTab_super	GtkBox
#  define _RpCacheTab_superClass	GtkBoxClass
#else /* !GTK_CHECK_VERSION(3,0,0) */
#  define _RpCacheTab_super	GtkVBox
#  define _RpCacheTab_superClass	GtkVBoxClass
#endif /* GTK_CHECK_VERSION(3,0,0) */

G_DECLARE_FINAL_TYPE(RpCacheTab, rp_cache_tab, RP, CACHE_TAB, _RpCacheTab_super)

GtkWidget	*rp_cache_tab_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS
