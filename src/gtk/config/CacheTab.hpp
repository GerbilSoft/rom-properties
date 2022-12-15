/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CacheTab.hpp: Thumbnail Cache tab for rp-config.                        *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CONFIG_CACHETAB_HPP__
#define __ROMPROPERTIES_GTK_CONFIG_CACHETAB_HPP__

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_CACHE_TAB (rp_cache_tab_get_type())
#if GTK_CHECK_VERSION(3,0,0)
G_DECLARE_FINAL_TYPE(RpCacheTab, rp_cache_tab, RP, CACHE_TAB, GtkBox)
#else /* !GTK_CHECK_VERSION(3,0,0) */
G_DECLARE_FINAL_TYPE(RpCacheTab, rp_cache_tab, RP, CACHE_TAB, GtkVBox)
#endif /* GTK_CHECK_VERSION(3,0,0) */

GtkWidget	*rp_cache_tab_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_CONFIG_CACHETAB_HPP__ */
