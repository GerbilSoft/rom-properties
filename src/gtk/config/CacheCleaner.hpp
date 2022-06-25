/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CacheCleaner.hpp: Cache cleaner object for CacheCleaner.                *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CONFIG_CACHECLEANER_HPP__
#define __ROMPROPERTIES_GTK_CONFIG_CACHECLEANER_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _CacheCleanerClass	CacheCleanerClass;
typedef struct _CacheCleaner		CacheCleaner;

#define TYPE_CACHE_CLEANER            (cache_cleaner_get_type())
#define CACHE_CLEANER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_CACHE_CLEANER, CacheCleaner))
#define CACHE_CLEANER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_CACHE_CLEANER, CacheCleanerClass))
#define IS_CACHE_CLEANER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_CACHE_CLEANER))
#define IS_CACHE_CLEANER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_CACHE_CLEANER))
#define CACHE_CLEANER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_CACHE_CLEANER, CacheCleanerClass))

/** RP_CacheDir: Cache directories. **/
typedef enum {
	RP_CD_System,		/*< nick=System thumbnail cache directory >*/
	RP_CD_RomProperties,	/*< nick=ROM Properties cache directory >*/
} RpCacheDir;

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		cache_cleaner_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		cache_cleaner_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

CacheCleaner	*cache_cleaner_new		(RpCacheDir cache_dir) G_GNUC_INTERNAL G_GNUC_MALLOC;

RpCacheDir	cache_cleaner_get_cache_dir	(CacheCleaner *cleaner) G_GNUC_INTERNAL;
void		cache_cleaner_set_cache_dir	(CacheCleaner *cleaner, RpCacheDir cache_dir) G_GNUC_INTERNAL;

/**
 * Clean the selected cache directory.
 *
 * This function should be called asynchronously from a GThread.
 * Connect to signals for progress information.
 */
void		cache_cleaner_run		(CacheCleaner *cleaner) G_GNUC_INTERNAL;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_CONFIG_CACHECLEANER_HPP__ */
