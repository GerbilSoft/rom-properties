/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CacheCleaner.hpp: Cache cleaner object for CacheCleaner.                *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "CacheCleaner.hpp"
#include "rp-gtk-enums.h"

#include "common.h"

// Other rom-properties libraries
#include "libi18n/i18n.hpp"
#include "librpfile/FileSystem.hpp"
#include "librpfile/RecursiveScan.hpp"
#include "libunixcommon/userdirs.hpp"
using namespace LibRpFile;

// d_type compatibility values
#include "d_type.h"

// C includes (C++ namespace)
#include <cassert>

// C++ STL classes
using std::pair;
using std::string;

// libfmt
#include "rp-libfmt.h"

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
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

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
			rp_cache_cleaner_set_cache_dir(widget, static_cast<RpCacheDir>(g_value_get_enum(value)));
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

	// TODO: Verify that cache_dir is in range.
	if (cleaner->cache_dir != cache_dir) {
		cleaner->cache_dir = cache_dir;
		g_object_notify_by_pspec(G_OBJECT(cleaner), props[PROP_CACHE_DIR]);
	}
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
	const char *s_cache_type = nullptr;
	const char *s_err = nullptr;
	switch (cleaner->cache_dir) {
		default:
			assert(!"Invalid cache directory specified.");
			// NOT TRANSLATABLE: This error shouldn't happen!
			s_err = "Invalid cache directory specified.";
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

			s_cache_type = C_("CacheCleaner", "System thumbnail cache");
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

			s_cache_type = C_("CacheCleaner", "rom-properties cache");
			break;
	}

	if (s_err != nullptr) {
		// An error occurred trying to get the directory.
		g_signal_emit(cleaner, signals[SIGNAL_PROGRESS], 0, 1, 1, TRUE);
		g_signal_emit(cleaner, signals[SIGNAL_ERROR], 0, s_err);
		g_signal_emit(cleaner, signals[SIGNAL_FINISHED], 0);
		return;
	}

	// Recursively scan the cache directory to get an initial maximum progress value.
	RecursiveScanState_t state;
	int ret = recursiveScan(state, cacheDir.c_str(), RecursiveScanMode::ScanOnly, nullptr, nullptr);
	if (ret != 0) {
		// Recursive scan failed.
		string stds_err;
		switch (ret) {
			case RSE_FOUND_UNEXPECTED_FILE_TYPE:
				fmt::format_to(std::back_inserter(stds_err),
					FRUN(C_("CacheCleaner", "{:s} has non-regular files. Not clearing it.")), s_cache_type);
				break;
			case RSE_FOUND_SYMBOLIC_LINK:
				fmt::format_to(std::back_inserter(stds_err),
					FRUN(C_("CacheCleaner", "{:s} has a symbolic link. Not clearing it.")), s_cache_type);
				break;
			case RSE_FOUND_UNEXPECTED_FILE_EXT:
				fmt::format_to(std::back_inserter(stds_err),
					FRUN(C_("CacheCleaner", "{:s} has unexpected files. Not clearing it.")), s_cache_type);
				break;
			default:
				if (ret < 0) {
					fmt::format_to(std::back_inserter(stds_err), FRUN(C_("CacheCleaner", "Initial scan failed: {:s}")), strerror(-ret));
				} else {
					fmt::format_to(std::back_inserter(stds_err), FRUN(C_("CacheCleaner", "Initial scan failed: error code {:d}")), ret);
				}
				break;
		}
		g_signal_emit(cleaner, signals[SIGNAL_PROGRESS], 0, 1, 1, TRUE);
		g_signal_emit(cleaner, signals[SIGNAL_ERROR], 0, stds_err.c_str());
		g_signal_emit(cleaner, signals[SIGNAL_FINISHED], 0);
		return;
	} else if (state.isEmpty()) {
		// Cache directory is empty.
		g_signal_emit(cleaner, signals[SIGNAL_PROGRESS], 0, 1, 1, FALSE);
		g_signal_emit(cleaner, signals[SIGNAL_CACHE_IS_EMPTY], 0, cleaner->cache_dir);
		g_signal_emit(cleaner, signals[SIGNAL_FINISHED], 0);
		return;
	}

	struct CallbackData_t {
		size_t cache_entries_total;
		RpCacheCleaner *obj;
	};
	CallbackData_t callbackData;

	// Get the number of files and directories.
	// NOTE: This is an estimate; it might change due to TOCTOU.
	callbackData.cache_entries_total = state.totalCount();
	callbackData.obj = cleaner;

	// Delete all of the files and subdirectories.
	g_signal_emit(cleaner, signals[SIGNAL_PROGRESS], 0, 0, static_cast<int>(callbackData.cache_entries_total), FALSE);
	state.reset();

	ret = recursiveScan(state, cacheDir.c_str(), RecursiveScanMode::ScanAndDelete, &callbackData,
		[](const RecursiveScanState_t &state, void *user_data) -> void {
			// TODO: Restrict update frequency to X number of files/directories?
			const CallbackData_t *const callbackData = static_cast<const CallbackData_t*>(user_data);
			g_signal_emit(callbackData->obj, signals[SIGNAL_PROGRESS], 0,
				static_cast<int>(callbackData->cache_entries_total),
				state.hasErrors());
		});
	if (ret != 0) {
		// Recursive scan failed.
		string stds_err;
		switch (ret) {
			case RSE_FOUND_UNEXPECTED_FILE_TYPE:
				fmt::format_to(std::back_inserter(stds_err),
					FRUN(C_("CacheCleaner", "{:s} has non-regular files. Cache clearing aborted.")), s_cache_type);
				break;
			case RSE_FOUND_SYMBOLIC_LINK:
				fmt::format_to(std::back_inserter(stds_err),
					FRUN(C_("CacheCleaner", "{:s} has a symbolic link. Cache clearing aborted.")), s_cache_type);
				break;
			case RSE_FOUND_UNEXPECTED_FILE_EXT:
				fmt::format_to(std::back_inserter(stds_err),
					FRUN(C_("CacheCleaner", "{:s} has unexpected files. Cache clearing aborted.")), s_cache_type);
				break;
			default:
				if (ret < 0) {
					fmt::format_to(std::back_inserter(stds_err), FRUN(C_("CacheCleaner", "Secondary scan failed: {:s}")), strerror(-ret));
				} else {
					fmt::format_to(std::back_inserter(stds_err), FRUN(C_("CacheCleaner", "Secondary scan failed: error code {:d}")), ret);
				}
				break;
		}
		g_signal_emit(cleaner, signals[SIGNAL_PROGRESS], 0, 1, 1, TRUE);
		g_signal_emit(cleaner, signals[SIGNAL_ERROR], 0, stds_err.c_str());
		g_signal_emit(cleaner, signals[SIGNAL_FINISHED], 0);
	} else {
		// Directory processed.
		g_signal_emit(cleaner, signals[SIGNAL_PROGRESS], 0, 1, 1, FALSE);
		g_signal_emit(cleaner, signals[SIGNAL_CACHE_CLEARED], 0, cleaner->cache_dir, state.dirErrs, state.fileErrs);
		g_signal_emit(cleaner, signals[SIGNAL_FINISHED], 0);
	}
}
