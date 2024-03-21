/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CacheCleaner.hpp: Cache cleaner object for CacheCleaner.                *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "glib-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_CACHE_CLEANER (rp_cache_cleaner_get_type())
G_DECLARE_FINAL_TYPE(RpCacheCleaner, rp_cache_cleaner, RP, CACHE_CLEANER, GObject)

/** RP_CacheDir: Cache directories. **/
typedef enum {
	RP_CD_System,		/*< nick="System thumbnail cache directory" >*/
	RP_CD_RomProperties,	/*< nick="ROM Properties cache directory" >*/
} RpCacheDir;

RpCacheCleaner	*rp_cache_cleaner_new		(RpCacheDir cache_dir) G_GNUC_INTERNAL G_GNUC_MALLOC;

RpCacheDir	rp_cache_cleaner_get_cache_dir	(RpCacheCleaner *cleaner) G_GNUC_INTERNAL;
void		rp_cache_cleaner_set_cache_dir	(RpCacheCleaner *cleaner, RpCacheDir cache_dir) G_GNUC_INTERNAL;

/**
 * Clean the selected cache directory.
 *
 * This function should be called directly from the GUI thread,
 * since cross-thread signals aren't safe for GTK+.
 *
 * Signal handlers should call gtk_main_iteration() to ensure
 * the GUI doesn't hang.
 *
 * @param cleaner CacheCleaner
 */
void		rp_cache_cleaner_run		(RpCacheCleaner *cleaner) G_GNUC_INTERNAL;

G_END_DECLS
