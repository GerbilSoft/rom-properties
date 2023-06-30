/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CacheCleaner.hpp: Cache cleaner object for CacheCleaner.                *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "CacheCleaner.hpp"
#include "rp-gtk-enums.h"

// libunixcommon
#include "libunixcommon/userdirs.hpp"

// librpfile
#include "librpfile/FileSystem.hpp"
#include "librpfile/RecursiveScan.hpp"
using namespace LibRpFile;

// d_type compatibility values
#include "d_type.h"

// C++ STL classes
using std::forward_list;
using std::pair;
using std::string;

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_CACHE_DIR,

	PROP_LAST
} CacheCleanerPropID;

/* Signal identifiers */
typedef enum {
	SIGNAL_PROGRESS,	// Progress update
	SIGNAL_ERROR,		// An error occurred
	SIGNAL_CACHE_IS_EMPTY,	// Cache directory is empty
	SIGNAL_CACHE_CLEARED,	// Cache directory has been cleared
	SIGNAL_FINISHED,	// Cleaning has finished

	SIGNAL_LAST
} CacheCleanerSignalID;

static void	rp_cache_cleaner_get_property	(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);
static void	rp_cache_cleaner_set_property	(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);

static GParamSpec *props[PROP_LAST];
static guint signals[SIGNAL_LAST];

// RpCacheCleaner class
struct _RpCacheCleanerClass {
	GObjectClass __parent__;
};

// RpCacheCleaner instance
struct _RpCacheCleaner {
	GObject __parent__;

	// Cache directory to clean
	RpCacheDir cache_dir;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpCacheCleaner, rp_cache_cleaner,
	G_TYPE_OBJECT, static_cast<GTypeFlags>(0), { });

static void
rp_cache_cleaner_class_init(RpCacheCleanerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_cache_cleaner_set_property;
	gobject_class->get_property = rp_cache_cleaner_get_property;

	/** Properties **/

	props[PROP_CACHE_DIR] = g_param_spec_enum(
		"cache-dir", "cache-dir", "Cache directory to clean.",
		RP_TYPE_CACHE_DIR, RP_CD_System,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);

	/** Signals **/

	/**
	 * Cache cleaning task progress update.
	 * @param pg_cur Current progress.
	 * @param pg_max Maximum progress.
	 * @param hasError If true, errors have occurred.
	 */
	signals[SIGNAL_PROGRESS] = g_signal_new("progress",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 3, G_TYPE_INT, G_TYPE_INT, G_TYPE_BOOLEAN);

	/**
	 * An error occurred while clearing the cache.
	 * @param error Error description.
	 */
	signals[SIGNAL_ERROR] = g_signal_new("error",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 1, G_TYPE_STRING);

	/**
	 * Cache directory is empty.
	 * @param cache_dir Which cache directory was checked.
	 */
	signals[SIGNAL_CACHE_IS_EMPTY] = g_signal_new("cache-is-empty",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 1, RP_TYPE_CACHE_DIR);

	/**
	 * Cache has been cleared.
	 * @param cache_dir Which cache dir was cleared.
	 * @param dirErrs Number of directories that could not be deleted.
	 * @param fileErrs Number of files that could not be deleted.
	 */
	signals[SIGNAL_CACHE_CLEARED] = g_signal_new("cache-cleared",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 3, RP_TYPE_CACHE_DIR, G_TYPE_UINT, G_TYPE_UINT);

	/**
	 * Cache cleaning task has completed.
	 * This is called when run() exits, regardless of status.
	 */
	signals[SIGNAL_FINISHED] = g_signal_new("finished",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 0);
}

static void
rp_cache_cleaner_init(RpCacheCleaner *tab)
{
	// Nothing to initialize here.
	RP_UNUSED(tab);
}

RpCacheCleaner*
rp_cache_cleaner_new(RpCacheDir cache_dir)
{
	return static_cast<RpCacheCleaner*>(g_object_new(RP_TYPE_CACHE_CLEANER, "cache-dir", cache_dir, nullptr));
}

/** Properties **/

static void
rp_cache_cleaner_get_property(GObject	*object,
			   guint	 prop_id,
			   GValue	*value,
			   GParamSpec	*pspec)
{
	RpCacheCleaner *const widget = RP_CACHE_CLEANER(object);

	switch (prop_id) {
		case PROP_CACHE_DIR:
			g_value_set_enum(value, widget->cache_dir);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rp_cache_cleaner_set_property(GObject	*object,
			   guint	 prop_id,
			   const GValue	*value,
			   GParamSpec	*pspec)
{
	RpCacheCleaner *const widget = RP_CACHE_CLEANER(object);

	switch (prop_id) {
		case PROP_CACHE_DIR:
			// TODO: Verify that it's in range?
			widget->cache_dir = static_cast<RpCacheDir>(g_value_get_enum(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/**
 * Get the specified cache directory.
 * @param cleaner CacheCleaner
 * @return Cache directory
 */
RpCacheDir
rp_cache_cleaner_get_cache_dir(RpCacheCleaner *cleaner)
{
	g_return_val_if_fail(RP_IS_CACHE_CLEANER(cleaner), RP_CD_System);
	return cleaner->cache_dir;
}

/**
 * Set the cache directory.
 * @param cleaner CacheCleaner
 * @param cache_dir Cache directory
 */
void
rp_cache_cleaner_set_cache_dir(RpCacheCleaner *cleaner, RpCacheDir cache_dir)
{
	g_return_if_fail(RP_IS_CACHE_CLEANER(cleaner));
	cleaner->cache_dir = cache_dir;
}

/** Internal functions **/

/** Methods **/

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
void
rp_cache_cleaner_run(RpCacheCleaner *cleaner)
{
	g_return_if_fail(RP_IS_CACHE_CLEANER(cleaner));

	string cacheDir;
	const char *s_err = nullptr;
	switch (cleaner->cache_dir) {
		default:
			assert(!"Invalid cache directory specified.");
			s_err = C_("CacheCleaner", "Invalid cache directory specified.");
			break;

		case RP_CD_System:
			// System thumbnails. (~/.cache/thumbnails)
			cacheDir = LibUnixCommon::getCacheDirectory();
			if (cacheDir.empty()) {
				s_err = C_("CacheCleaner", "Unable to get the XDG cache directory.");
				break;
			}
			// Append "/thumbnails".
			cacheDir += "/thumbnails";
			if (!LibUnixCommon::isWritableDirectory(cacheDir.c_str())) {
				// Thumbnails subdirectory does not exist. (or is not writable)
				// TODO: Check specifically if it's not writable or doesn't exist?
				s_err = C_("CacheCleaner", "Thumbnails cache directory does not exist.");
				break;
			}
			break;

		case RP_CD_RomProperties:
			// rom-properties cache. (~/.cache/rom-properties)
			cacheDir = FileSystem::getCacheDirectory();
			if (cacheDir.empty()) {
				s_err = C_("CacheCleaner", "Unable to get the rom-properties cache directory.");
				break;
			}

			// Does the cache directory exist?
			// If it doesn't, we'll act like it's empty.
			if (FileSystem::access(cacheDir.c_str(), R_OK) != 0) {
				g_signal_emit(cleaner, signals[SIGNAL_CACHE_IS_EMPTY], 0, cleaner->cache_dir);
				g_signal_emit(cleaner, signals[SIGNAL_FINISHED], 0);
				return;
			}

			break;
	}

	if (s_err != nullptr) {
		// An error occurred trying to get the directory.
		g_signal_emit(cleaner, signals[SIGNAL_PROGRESS], 0, 1, 1, TRUE);
		g_signal_emit(cleaner, signals[SIGNAL_ERROR], 0, s_err);
		g_signal_emit(cleaner, signals[SIGNAL_FINISHED], 0);
		return;
	}

	// Recursively scan the cache directory.
	// TODO: Do we really want to store everything in a list? (Wastes memory.)
	// Maybe do a simple counting scan first, then delete.
	forward_list<pair<string, uint8_t> > rlist;
	int ret = recursiveScan(cacheDir.c_str(), rlist);
	if (ret != 0) {
		// Non-image file found.
		const char *s_err;
		switch (cleaner->cache_dir) {
			default:
				assert(!"Invalid cache directory specified.");
				s_err = C_("CacheCleaner", "Invalid cache directory specified.");
				break;
			case RP_CD_System:
				s_err = C_("CacheCleaner", "System thumbnail cache has unexpected files. Not clearing it.");
				break;
			case RP_CD_RomProperties:
				s_err = C_("CacheCleaner", "rom-properties cache has unexpected files. Not clearing it.");
				break;
		}
		g_signal_emit(cleaner, signals[SIGNAL_PROGRESS], 0, 1, 1, TRUE);
		g_signal_emit(cleaner, signals[SIGNAL_ERROR], 0, s_err);
		g_signal_emit(cleaner, signals[SIGNAL_FINISHED], 0);
		return;
	} else if (rlist.empty()) {
		// Cache directory is empty.
		g_signal_emit(cleaner, signals[SIGNAL_CACHE_IS_EMPTY], 0, cleaner->cache_dir);
		g_signal_emit(cleaner, signals[SIGNAL_FINISHED], 0);
		return;
	}

	// NOTE: std::forward_list doesn't have size().
	const size_t rlist_size = std::distance(rlist.cbegin(), rlist.cend());

	// Delete all of the files and subdirectories.
	g_signal_emit(cleaner, signals[SIGNAL_PROGRESS], 0, 0, static_cast<int>(rlist_size), FALSE);
	unsigned int count = 0;
	unsigned int dirErrs = 0, fileErrs = 0;
	bool hasErrors = false;
	for (const auto &p : rlist) {
		if (p.second == DT_DIR) {
			// Remove the directory.
			int ret = rmdir(p.first.c_str());
			if (ret != 0) {
				dirErrs++;
				hasErrors = true;
			}
		} else {
			// Delete the file.
			// TODO: Does the parent directory mode need to be changed to writable?
			int ret = unlink(p.first.c_str());
			if (ret != 0) {
				fileErrs++;
				hasErrors = true;
			}
		}

		// TODO: Restrict update frequency to X number of files/directories?
		count++;
		g_signal_emit(cleaner, signals[SIGNAL_PROGRESS], 0, count, static_cast<int>(rlist_size), hasErrors);
	}

	// Directory processed.
	g_signal_emit(cleaner, signals[SIGNAL_CACHE_CLEARED], 0, cleaner->cache_dir, dirErrs, fileErrs);
	g_signal_emit(cleaner, signals[SIGNAL_FINISHED], 0);
}
