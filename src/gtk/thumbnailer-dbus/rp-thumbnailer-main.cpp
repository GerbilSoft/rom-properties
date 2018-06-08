/***************************************************************************
 * ROM Properties Page shell extension. (GNOME)                            *
 * rp-thumbnailer-main.cpp: D-Bus thumbnailerer service: main()            *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "librpbase/common.h"
#include "libunixcommon/userdirs.hpp"
#include "libunixcommon/dll-search.h"
#include "rp-thumbnailer-dbus.hpp"

// C includes. (C++ namespace)
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <string>
using std::string;

// dlopen()
#include <dlfcn.h>

// Shutdown request.
static bool stop_main_loop = false;

// rp_create_thumbnail() function pointer.
static PFN_RP_CREATE_THUMBNAIL pfn_rp_create_thumbnail = nullptr;

// Cache directory.
static string cache_dir;

/**
 * Initialize the cache directory.
 * @return 0 on success; non-zero on error.
 */
static int init_cache_dir(void)
{
	cache_dir = LibUnixCommon::getCacheDirectory();
	if (cache_dir.empty()) {
		g_critical("Unable to determine the XDG cache directory.");
		return -1;
	}
	g_debug("Cache directory: %s", cache_dir.c_str());
	return 0;
}

/**
 * Debug print function for rp_dll_search().
 * @param level Debug level.
 * @param format Format string.
 * @param ... Format arguments.
 * @return vfprintf() return value.
 */
static int ATTR_PRINTF(2, 3) fnDebug(int level, const char *format, ...)
{
	// g_warning() may be using g_log_structured(),
	// and there's no variant of g_log_structured()
	// that takes va_list, so we'll print it to a
	// buffer first.
	char buf[512];

	va_list args;
	va_start(args, format);
	int ret = vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	if (level < LEVEL_ERROR) {
		// G_MESSAGES_DEBUG must be set to rom-properties-xfce
		// in order to print these messages.
		g_debug("%s", buf);
	} else {
		g_warning("%s", buf);
	}
	return ret;
}

/**
 * Shutdown callback.
 * @param thumbnailer RpThumbnail object.
 * @param main_loop GMainLoop.
 */
static void
shutdown_rp_thumbnailer_dbus(RpThumbnailer *thumbnailer, GMainLoop *main_loop)
{
	g_return_if_fail(IS_RP_THUMBNAILER(thumbnailer));
	g_return_if_fail(main_loop != nullptr);

	// Exit the main loop.
	stop_main_loop = true;
	if (g_main_loop_is_running(main_loop)) {
		g_main_loop_quit(main_loop);
	}
}

/**
 * The D-Bus name was either lost or could not be acquired.
 * @param main_loop GMainLoop
 */
static void
on_dbus_name_lost(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	GMainLoop *const main_loop = (GMainLoop*)user_data;
	g_return_if_fail(main_loop != nullptr);

	stop_main_loop = true;
	if (g_main_loop_is_running(main_loop)) {
		g_debug("D-Bus name was lost; exiting.");
		g_main_loop_quit(main_loop);
	}
}

int main(int argc, char *argv[])
{
	RP_UNUSED(argc);
	RP_UNUSED(argv);

#if !GLIB_CHECK_VERSION(2,36,0)
	// g_type_init() is automatic as of glib-2.36.0
	// and is marked deprecated.
	g_type_init();
#endif /* !GLIB_CHECK_VERSION(2,36,0) */
#if !GLIB_CHECK_VERSION(2,32,0)
	// g_thread_init() is automatic as of glib-2.32.0
	// and is marked deprecated.
	if (!g_thread_supported()) {
		g_thread_init(nullptr);
	}
#endif /* !GLIB_CHECK_VERSION(2,32,0) */

	// Initialize the cache directory.
	if (init_cache_dir() != 0) {
		return EXIT_FAILURE;
	}

	// Attempt to open a ROM Properties Page library.
	void *pDll = NULL;
	int ret = rp_dll_search("rp_create_thumbnail", &pDll, (void**)&pfn_rp_create_thumbnail, fnDebug);
	if (ret != 0) {
		return EXIT_FAILURE;
	}

	GError *error = nullptr;
	GDBusConnection *const connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
	if (error) {
		g_critical("Unable to connect to the session bus: %s", error->message);
		g_error_free(error);
		dlclose(pDll);
		return EXIT_FAILURE;
	}

	GMainLoop *main_loop = g_main_loop_new(nullptr, false);

	// Create the RpThumbnail service object.
	RpThumbnailer *const thumbnailer = rp_thumbnailer_new(
		connection, cache_dir.c_str(), pfn_rp_create_thumbnail);

	// Register the D-Bus service.
	g_bus_own_name_on_connection(connection,
		"com.gerbilsoft.rom-properties.SpecializedThumbnailer1",
		G_BUS_NAME_OWNER_FLAGS_NONE, nullptr, on_dbus_name_lost, main_loop, nullptr);

	if (rp_thumbnailer_is_exported(thumbnailer)) {
		// Service object is exported.

		// Make sure we quit after the RpThumbnail server is idle for long enough.
		g_signal_connect(thumbnailer, "shutdown", G_CALLBACK(shutdown_rp_thumbnailer_dbus), main_loop);

		// Run the main loop.
		if (!stop_main_loop) {
			g_debug("Starting the D-Bus service.");
			g_main_loop_run(main_loop);
		}
	}
	dlclose(pDll);
	return 0;
}
