/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CacheTab.hpp: Thumbnail Cache tab for rp-config.                        *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CONFIG_CACHETAB_HPP__
#define __ROMPROPERTIES_GTK_CONFIG_CACHETAB_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _CacheTabClass		CacheTabClass;
typedef struct _CacheTab		CacheTab;

#define TYPE_CACHE_TAB            (cache_tab_get_type())
#define CACHE_TAB(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_CACHE_TAB, CacheTab))
#define CACHE_TAB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_CACHE_TAB, CacheTabClass))
#define IS_CACHE_TAB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_CACHE_TAB))
#define IS_CACHE_TAB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_CACHE_TAB))
#define CACHE_TAB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_CACHE_TAB, CacheTabClass))

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		cache_tab_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		cache_tab_register_type		(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*cache_tab_new		(void) G_GNUC_INTERNAL G_GNUC_MALLOC;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_CONFIG_CACHETAB_HPP__ */
