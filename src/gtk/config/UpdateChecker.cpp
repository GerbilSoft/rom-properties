/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * UpdateChecker.cpp: Update checker object for AboutTab.                  *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "UpdateChecker.hpp"
#include "ProxyForUrl.hpp"

// librpbase
#include "librpbase/config/AboutTabText.hpp"
using namespace LibRpBase;

// libromdata
#include "libromdata/img/CacheManager.hpp"
using LibRomData::CacheManager;

// C++ STL classes
using std::string;

/* Signal identifiers */
typedef enum {
	SIGNAL_ERROR,		// An error occurred
	SIGNAL_RETRIEVED,	// Update version retrieved
	SIGNAL_FINISHED,	// Finished checking (always called when finished)

	SIGNAL_LAST
} UpdateCheckerSignalID;

static guint signals[SIGNAL_LAST];

// UpdateChecker class
struct _RpUpdateCheckerClass {
	GObjectClass __parent__;
};

// UpdateChecker instance
struct _RpUpdateChecker {
	GObject __parent__;

	GThread *thread;
};

static void	rp_update_checker_dispose		(GObject	*object);

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpUpdateChecker, rp_update_checker,
	G_TYPE_OBJECT, static_cast<GTypeFlags>(0), { });

static void
rp_update_checker_class_init(RpUpdateCheckerClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = rp_update_checker_dispose;

	/** Signals **/

	/**
	 * An error occurred while trying to retrieve the update version.
	 * TODO: Error code?
	 * @param error Error message
	 */
	signals[SIGNAL_ERROR] = g_signal_new("error",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 1, G_TYPE_STRING);

	/**
	 * Update version retrieved.
	 * @param updateVersion Update version (64-bit format)
	 */
	signals[SIGNAL_RETRIEVED] = g_signal_new("retrieved",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 1, G_TYPE_UINT64);

	/**
	 * Update version task has completed.
	 * This is called when run() exits, regardless of status.
	 */
	signals[SIGNAL_FINISHED] = g_signal_new("finished",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 0);
}

static void
rp_update_checker_init(RpUpdateChecker *tab)
{
	// Nothing to initialize here.
	RP_UNUSED(tab);
}

RpUpdateChecker*
rp_update_checker_new(void)
{
	return static_cast<RpUpdateChecker*>(g_object_new(RP_TYPE_UPDATE_CHECKER, nullptr));
}

static void
rp_update_checker_dispose(GObject *object)
{
	RpUpdateChecker *const updChecker = RP_UPDATE_CHECKER(object);

	// Make sure the thread has exited.
	// TODO: Set a timeout and terminate the thread if the timeout expires?
	// NOTE: g_thread_join() releases the GThread's resources.
	if (updChecker->thread) {
		g_thread_join(updChecker->thread);
		updChecker->thread = nullptr;
	}

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_update_checker_parent_class)->dispose(object);
}

/** Methods **/

/**
 * Update checker internal function.
 * @param updChecker Update checker
 */
static gpointer
rp_update_checker_thread_run(RpUpdateChecker *updChecker)
{
	// Download sys/version.txt and compare it to our version.
	// NOTE: Ignoring the fourth decimal (development flag).
	const char *const updateVersionUrl =
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::UpdateVersionUrl);
	const char *const updateVersionCacheKey =
		AboutTabText::getProgramInfoString(AboutTabText::ProgramInfoStringID::UpdateVersionCacheKey);

	assert(updateVersionUrl != nullptr);
	assert(updateVersionCacheKey != nullptr);
	if (!updateVersionUrl || !updateVersionCacheKey) {
		// TODO: Show an error message.
		g_signal_emit(updChecker, signals[SIGNAL_FINISHED], 0);
		return GINT_TO_POINTER(1);
	}

	CacheManager cache;
	const string proxy = proxyForUrl(updateVersionUrl);
	if (!proxy.empty()) {
		// Proxy is required.
		cache.setProxyUrl(proxy);
	}

	// Download the version file.
	const string cache_filename = cache.download(updateVersionCacheKey);
	if (cache_filename.empty()) {
		// Unable to download the version file.
		g_signal_emit(updChecker, signals[SIGNAL_ERROR], 0,
			C_("UpdateChecker", "Failed to download version file."));
		g_signal_emit(updChecker, signals[SIGNAL_FINISHED], 0);
		return GINT_TO_POINTER(2);
	}

	// Read the version file.
	FILE *f = fopen(cache_filename.c_str(), "r");
	if (!f) {
		// TODO: Error code?
		g_signal_emit(updChecker, signals[SIGNAL_ERROR], 0,
			C_("UpdateChecker", "Failed to open version file."));
		g_signal_emit(updChecker, signals[SIGNAL_FINISHED], 0);
		return GINT_TO_POINTER(3);
	}

	// Read the first line, which should contain a 4-decimal version number.
	char buf[256];
	char *fgret = fgets(buf, sizeof(buf), f);
	fclose(f);
	if (fgret == buf && ISSPACE(buf[0])) {
		g_signal_emit(updChecker, signals[SIGNAL_ERROR], 0,
			C_("UpdateChecker", "Version file is invalid."));
		g_signal_emit(updChecker, signals[SIGNAL_FINISHED], 0);
		return GINT_TO_POINTER(4);
	}

	// Split into 4 elements.
	gchar **sVersionArray = g_strsplit(buf, ".", 5);
	if (sVersionArray && sVersionArray[0] && sVersionArray[1] &&
	    sVersionArray[2] && sVersionArray[3] && !sVersionArray[4])
	{
		// We have exactly 4 elements.
	} else {
		g_signal_emit(updChecker, signals[SIGNAL_ERROR], 0,
			C_("UpdateChecker", "Version file is invalid."));
		g_signal_emit(updChecker, signals[SIGNAL_FINISHED], 0);
		g_strfreev(sVersionArray);
		return GINT_TO_POINTER(5);
	}

	// Convert to a 64-bit version. (ignoring the development flag)
	uint64_t updateVersion = 0;
	for (unsigned int i = 0; i < 3; i++, updateVersion <<= 16U) {
		gchar *endptr;
		const gint64 x = g_ascii_strtoll(sVersionArray[i], &endptr, 10);
		if (x < 0 || *endptr != '\0') {
			g_signal_emit(updChecker, signals[SIGNAL_ERROR], 0,
				C_("UpdateChecker", "Version file is invalid."));
			g_strfreev(sVersionArray);
			return GINT_TO_POINTER(6);
		}
		updateVersion |= (static_cast<uint64_t>(x) & 0xFFFFU);
	}
	g_strfreev(sVersionArray);

	// Update version retrieved.
	g_signal_emit(updChecker, signals[SIGNAL_RETRIEVED], 0, updateVersion);
	g_signal_emit(updChecker, signals[SIGNAL_FINISHED], 0);
	return GINT_TO_POINTER(0);
}

/**
 * Check for updates.
 * The update check is run asynchronously in a separate thread.
 *
 * Results will be sent as signals:
 * - 'retrieved': Update version retrieved. (guint64 parameter with the version)
 * - 'error': An error occurred. (gchar* parameter with the error message)
 *
 * @param updChecker Update checker
 */
void
rp_update_checker_run(RpUpdateChecker *updChecker)
{
	g_return_if_fail(RP_IS_UPDATE_CHECKER(updChecker));

	// Make sure the thread isn't currently running.
	// TODO: Show an error if it is?
	if (updChecker->thread) {
		g_thread_join(updChecker->thread);
		updChecker->thread = nullptr;
	}

	// Run the update check in a separate thread.
	updChecker->thread = g_thread_new("updChecker", (GThreadFunc)rp_update_checker_thread_run, updChecker);
}
