/***************************************************************************
 * ROM Properties Page shell extension. (GNOME)                            *
 * rp-thumbnailer-main.cpp: D-Bus thumbnailerer service: main()            *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "common.h"
#include "check-uid.h"
#include "libunixcommon/dll-search.h"
#include "rp-thumbnailer-dbus.h"

// OS-specific security options.
#include "rptsecure.h"

// C includes
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// stdbool
#include "stdboolx.h"

// dlopen()
#include <dlfcn.h>

// Shutdown request
static bool stop_main_loop = false;

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
	g_return_if_fail(RP_IS_THUMBNAILER(thumbnailer));
	g_return_if_fail(main_loop != NULL);

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
	RP_UNUSED(connection);
	RP_UNUSED(name);
	GMainLoop *const main_loop = (GMainLoop*)user_data;
	g_return_if_fail(main_loop != NULL);

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

	// Enable security options.
	CHECK_UID_RET(EXIT_FAILURE);
	rpt_do_security_options();

#if !GLIB_CHECK_VERSION(2, 35, 1)
	// g_type_init() is automatic as of glib-2.35.1
	// and is marked deprecated.
	g_type_init();
#endif /* !GLIB_CHECK_VERSION(2, 35, 1) */
#if !GLIB_CHECK_VERSION(2, 32, 0)
	// g_thread_init() is automatic as of glib-2.32.0
	// and is marked deprecated.
	if (!g_thread_supported()) {
		g_thread_init(NULL);
	}
#endif /* !GLIB_CHECK_VERSION(2, 32, 0) */

	// Get the XDG cache directory.
	const gchar *const cache_dir = g_get_user_cache_dir();
	if (!cache_dir) {
		return EXIT_FAILURE;
	}
	g_debug("Cache directory: %s", cache_dir);

	// Attempt to open a ROM Properties Page library.
	void *pDll = NULL;
	PFN_RP_CREATE_THUMBNAIL2 pfn_rp_create_thumbnail2 = NULL;
	int ret = rp_dll_search("rp_create_thumbnail2", &pDll, (void**)&pfn_rp_create_thumbnail2, fnDebug);
	if (ret != 0) {
		return EXIT_FAILURE;
	}

	GError *error = NULL;
	GDBusConnection *const connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	if (error) {
		g_critical("Unable to connect to the session bus: %s", error->message);
		g_error_free(error);
		dlclose(pDll);
		return EXIT_FAILURE;
	}

	GMainLoop *main_loop = g_main_loop_new(NULL, false);

	// Create the RpThumbnail service object.
	RpThumbnailer *const thumbnailer = rp_thumbnailer_new(
		connection, cache_dir, pfn_rp_create_thumbnail2);

	// Register the D-Bus service.
	g_bus_own_name_on_connection(connection,
		"com.gerbilsoft.rom-properties.SpecializedThumbnailer1",
		G_BUS_NAME_OWNER_FLAGS_NONE, NULL, on_dbus_name_lost, main_loop, NULL);

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
