/***************************************************************************
 * ROM Properties Page shell extension. (GNOME)                            *
 * thumbnail-dbus.cpp: D-Bus thumbnail provider.                           *
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

/**
 * References:
 * - https://github.com/linneman/dbus-example
 */

#include "rp-thumbnail-dbus.hpp"
#include "librpbase/common.h"
#include "rp-stub/dll-search.h"

#include <glib-object.h>
#include "SpecializedThumbnailer1.h"

// C includes. (C++ namespace)
#include <cstdarg>

// C++ includes.
#include <deque>
#include <string>
#include <unordered_map>
using std::deque;
using std::make_pair;
using std::string;
using std::unordered_map;

// dlopen()
#include <dlfcn.h>

/**
 * rp_create_thumbnail() function pointer.
 * @param source_file Source file. (UTF-8)
 * @param output_file Output file. (UTF-8)
 * @param maximum_size Maximum size.
 * @return 0 on success; non-zero on error.
 */
typedef int (*PFN_RP_CREATE_THUMBNAIL)(const char *source_file, const char *output_file, int maximum_size);
static PFN_RP_CREATE_THUMBNAIL pfn_rp_create_thumbnail = nullptr;

// Thumbnail cache directory.
static string cache_dir;

// D-Bus connection.
// TODO: Make this a property of RpThumbnail?
static GDBusConnection *connection = nullptr;

// Shutdown request.
static bool stop_main_loop = false;

// from tumbler-utils.h
#define g_dbus_async_return_val_if_fail(expr, invocation, val) \
G_STMT_START { \
	if (G_UNLIKELY(!(expr))) { \
		GError *dbus_async_return_if_fail_error = nullptr; \
		g_set_error(&dbus_async_return_if_fail_error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, \
			"Assertion \"%s\" failed", #expr); \
		g_dbus_method_invocation_return_gerror(invocation, dbus_async_return_if_fail_error); \
		g_clear_error(&dbus_async_return_if_fail_error); \
		return (val); \
	} \
} G_STMT_END

/* Signal identifiers */
enum RpThumbnailSignals {
	SIGNAL_0,

	// RpThumbnail has been idle for long enough and should exit.
	SIGNAL_SHUTDOWN,

	SIGNAL_LAST
};

// Internal functions.
static void	rp_thumbnail_dispose		(GObject	*object);
static void	rp_thumbnail_finalize		(GObject	*object);
static gboolean	rp_thumbnail_timeout		(RpThumbnail	*thumbnailer);
static gboolean	rp_thumbnail_process		(RpThumbnail	*thumbnailer);

// D-Bus methods.
static gboolean	rp_thumbnail_queue		(OrgFreedesktopThumbnailsSpecializedThumbnailer1 *skeleton,
						 GDBusMethodInvocation *invocation,
						 const gchar	*uri,
						 const gchar	*mime_type,
						 const char	*flavor,
						 bool		 urgent,
						 RpThumbnail	*thumbnailer);
static gboolean	rp_thumbnail_dequeue		(OrgFreedesktopThumbnailsSpecializedThumbnailer1 *skeleton,
						 GDBusMethodInvocation *invocation,
						 guint		 handle,
						 RpThumbnail	*thumbnailer);

struct _RpThumbnailClass {
	GObjectClass __parent__;

	// TODO: Store signal_ids outside of the class?
	guint signal_ids[SIGNAL_LAST];
};

#define SHUTDOWN_TIMEOUT_SECONDS 30

// Thumbnail request information.
struct request_info {
	string uri;
	bool large;	// False for 'normal' (128x128); true for 'large' (256x256)
	bool urgent;	// 'urgent' value
};

struct _RpThumbnail {
	GObject __parent__;
	OrgFreedesktopThumbnailsSpecializedThumbnailer1 *skeleton;

	// Is the D-Bus object exported?
	bool exported;

	// Has the shutdown signal been emitted?
	bool shutdown_emitted;

	// Shutdown timeout.
	guint timeout_id;

	// Idle function for processing.
	guint idle_process;

	// Last handle value.
	guint last_handle;

	// URI queue.
	// Note that queued thumbnail requests are
	// referenced by handle, so we store the
	// handles in a deque and the URIs in a map.
	deque<guint> *handle_queue;
	unordered_map<guint, request_info> *uri_map;
};

/** Type information. **/
// NOTE: We can't use G_DEFINE_DYNAMIC_TYPE() here because
// that requires a GTypeModule, which we don't have.

static void     rp_thumbnail_init              (RpThumbnail      *thumbnailer);
static void     rp_thumbnail_class_init        (RpThumbnailClass *klass);

static void     rp_thumbnail_constructed       (GObject *object);

static gpointer rp_thumbnail_parent_class = nullptr;

static void
rp_thumbnail_class_intern_init(gpointer klass)
{
	  rp_thumbnail_parent_class = g_type_class_peek_parent(klass);
	  rp_thumbnail_class_init(static_cast<RpThumbnailClass*>(klass));
}

GType
rp_thumbnail_get_type(void)
{
	static GType rp_thumbnail_type_id = 0;
	if (!rp_thumbnail_type_id) {
		static const GTypeInfo g_define_type_info = {
			sizeof(RpThumbnailClass),	// class_size
			nullptr,			// base_init
			nullptr,			// base_finalize
			(GClassInitFunc)rp_thumbnail_class_intern_init,
			nullptr,			// class_finalize
			nullptr,			// class_data
			sizeof(RpThumbnail),		// instance_size
			0,				// n_preallocs
			(GInstanceInitFunc)rp_thumbnail_init,
			nullptr				// value_table
		};
		rp_thumbnail_type_id = g_type_register_static(G_TYPE_OBJECT,
							"RpThumbnail",
							&g_define_type_info,
							(GTypeFlags) 0);
	}
	return rp_thumbnail_type_id;
}

/** End type information. **/

static void
rp_thumbnail_class_init(RpThumbnailClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = rp_thumbnail_dispose;
	gobject_class->finalize = rp_thumbnail_finalize;
	gobject_class->constructed = rp_thumbnail_constructed;

	// Register signals.
	klass->signal_ids[SIGNAL_0] = 0;

	// RpThumbnail has been idle for long enough and should exit.
	klass->signal_ids[SIGNAL_SHUTDOWN] = g_signal_new("shutdown",
		TYPE_RP_THUMBNAIL, G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 0);
}

static void
rp_thumbnail_init(RpThumbnail *thumbnailer)
{
	thumbnailer->skeleton = nullptr;
	thumbnailer->exported = false;
	thumbnailer->shutdown_emitted = false;
	thumbnailer->timeout_id = 0;
	thumbnailer->idle_process = 0;
	thumbnailer->last_handle = 0;
	thumbnailer->handle_queue = new deque<guint>();
	thumbnailer->uri_map = new unordered_map<guint, request_info>();
	thumbnailer->uri_map->reserve(8);
}

static void
rp_thumbnail_constructed(GObject *object)
{
	g_return_if_fail(IS_RP_THUMBNAIL(object));
	RpThumbnail *const thumbnailer = RP_THUMBNAIL(object);

	GError *error = nullptr;
	thumbnailer->skeleton = org_freedesktop_thumbnails_specialized_thumbnailer1_skeleton_new();
	g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(thumbnailer->skeleton),
		connection, "/com/gerbilsoft/rom_properties/SpecializedThumbnailer1", &error);
	if (error) {
		g_critical("Error exporting RpThumbnail on session bus: %s", error->message);
		g_error_free(error);
		thumbnailer->exported = false;
	} else {
		// Connect signals to the relevant functions.
		g_signal_connect(thumbnailer->skeleton, "handle-queue",
			G_CALLBACK(rp_thumbnail_queue), thumbnailer);
		g_signal_connect(thumbnailer->skeleton, "handle-dequeue",
			G_CALLBACK(rp_thumbnail_dequeue), thumbnailer);
		
		// Make sure we shut down after inactivity.
		thumbnailer->timeout_id = g_timeout_add_seconds(SHUTDOWN_TIMEOUT_SECONDS,
			(GSourceFunc)rp_thumbnail_timeout, thumbnailer);

		// Object is exported.
		thumbnailer->exported = true;
	}
}

static void
rp_thumbnail_dispose(GObject *object)
{
	RpThumbnail *const thumbnailer = RP_THUMBNAIL(object);

	// Stop the inactivity timeout.
	if (G_LIKELY(thumbnailer->timeout_id != 0)) {
		g_source_remove(thumbnailer->timeout_id);
		thumbnailer->timeout_id = 0;
	}

	// Unregister idle_process.
	if (G_UNLIKELY(thumbnailer->idle_process != 0)) {
		g_source_remove(thumbnailer->idle_process);
		thumbnailer->idle_process = 0;
	}
}

static void
rp_thumbnail_finalize(GObject *object)
{
	RpThumbnail *const thumbnailer = RP_THUMBNAIL(object);

	delete thumbnailer->handle_queue;
	delete thumbnailer->uri_map;
}

/**
 * Queue a ROM image for thumbnailing.
 * @param skeleton	[in] GDBusObjectSkeleton
 * @param invocation	[in/out] GDBusMethodInvocation
 * @param uri		[in] URI to thumbnail.
 * @param mime_type	[in] MIME type of the URI.
 * @param flavor	[in] The flavor that should be made, e.g. "normal".
 * @param urgent	[in] Is this thumbnail "urgent"?
 * @param thumbnailer	[in] RpThumbnailer object.
 * @return True if the signal was handled; false if not.
 */
static gboolean
rp_thumbnail_queue(OrgFreedesktopThumbnailsSpecializedThumbnailer1 *skeleton,
	GDBusMethodInvocation *invocation,
	const gchar *uri, const gchar *mime_type,
	const char *flavor, bool urgent,
	RpThumbnail *thumbnailer)
{
	g_dbus_async_return_val_if_fail(IS_RP_THUMBNAIL(thumbnailer), invocation, false);
	g_dbus_async_return_val_if_fail(uri != nullptr, invocation, false);

	if (G_UNLIKELY(thumbnailer->shutdown_emitted)) {
		// The shutdown signal was emitted.
		// Can't queue anything else.
		g_dbus_method_invocation_return_error(invocation,
			G_DBUS_ERROR, G_DBUS_ERROR_NO_SERVER, "Service is shutting down.");
		return true;
	}

	// Stop the inactivity timeout.
	if (G_LIKELY(thumbnailer->timeout_id != 0)) {
		g_source_remove(thumbnailer->timeout_id);
		thumbnailer->timeout_id = 0;
	}

	// Queue the URI for processing.
	// TODO: Handle 'flavor', 'urgent', etc.
	guint handle = ++thumbnailer->last_handle;
	if (G_UNLIKELY(handle == 0)) {
		// Overflow. Increment again so we
		// don't return a handle of 0.
		handle = ++thumbnailer->last_handle;
	}

	// Add the URI to the queue.
	// NOTE: Currently handling all flavors that aren't "large" as "normal".
	request_info req;
	req.uri = uri;
	req.large = (g_ascii_strcasecmp(flavor, "large") == 0);
	req.urgent = urgent;
	thumbnailer->uri_map->insert(make_pair(handle, req));
	thumbnailer->handle_queue->push_back(handle);

	// Make sure the idle process is started.
	// TODO: Make it multi-threaded?
	// FIXME: Atomic compare and/or mutex if multi-threaded.
	if (thumbnailer->idle_process == 0) {
		thumbnailer->idle_process = g_idle_add((GSourceFunc)rp_thumbnail_process, thumbnailer);
	}

	org_freedesktop_thumbnails_specialized_thumbnailer1_complete_queue(skeleton, invocation, handle);
	return true;
}

/**
 * Dequeue a ROM image that was previously queued for thumbnailing.
 * @param skeleton	[in] GDBusObjectSkeleton
 * @param invocation	[in] GDBusMethodInvocation
 * @param handle	[in] Handle previously returned by queue().
 * @param thumbnailer	[in] RpThumbnailer object.
 * @return True if the signal was handled; false if not.
 */
static gboolean
rp_thumbnail_dequeue(OrgFreedesktopThumbnailsSpecializedThumbnailer1 *skeleton,
	GDBusMethodInvocation *invocation,
	guint handle,
	RpThumbnail *thumbnailer)
{
	g_dbus_async_return_val_if_fail(IS_RP_THUMBNAIL(thumbnailer), invocation, false);
	g_dbus_async_return_val_if_fail(handle != 0, invocation, false);

	// TODO
	org_freedesktop_thumbnails_specialized_thumbnailer1_complete_dequeue(skeleton, invocation);
	return true;
}

/**
 * Inactivity timeout has elapsed.
 * @param thumbnailer RpThumbnail object.
 */
static gboolean
rp_thumbnail_timeout(RpThumbnail *thumbnailer)
{
	g_return_val_if_fail(IS_RP_THUMBNAIL(thumbnailer), false);
	if (!thumbnailer->handle_queue->empty()) {
		// Still processing stuff.
		return true;
	}

	// Stop the timeout and shut down the thumbnailer.
	RpThumbnailClass *const klass = RP_THUMBNAIL_GET_CLASS(thumbnailer);
	thumbnailer->timeout_id = 0;
	thumbnailer->shutdown_emitted = true;
	g_signal_emit(thumbnailer, klass->signal_ids[SIGNAL_SHUTDOWN], 0);
	g_debug("Shutting down due to %u seconds of inactivity.", SHUTDOWN_TIMEOUT_SECONDS);
	return false;
}

/**
 * Process a thumbnail.
 * @param thumbnailer RpThumbnail object.
 */
static gboolean
rp_thumbnail_process(RpThumbnail *thumbnailer)
{
	g_return_val_if_fail(IS_RP_THUMBNAIL(thumbnailer), FALSE);

	guint handle;
	gchar *filename;
	GChecksum *md5 = nullptr;
	const gchar *md5_string;	// owned by md5 object
	string cache_filename;
	int ret;

	// Process one thumbnail.
	handle = thumbnailer->handle_queue->front();
	thumbnailer->handle_queue->pop_front();
	auto iter = thumbnailer->uri_map->find(handle);
	const request_info *const req = (iter != thumbnailer->uri_map->end() ? &(iter->second) : nullptr);
	if (!req) {
		// URI not found.
		org_freedesktop_thumbnails_specialized_thumbnailer1_emit_error(
			thumbnailer->skeleton, handle, "",
			0, "Handle has no associated URI.");
		goto cleanup;
	}

	// Verify that the specified URI is local.
	// TODO: Support GVFS.
	filename = g_filename_from_uri(req->uri.c_str(), nullptr, nullptr);
	if (!filename) {
		// URI is not describing a local file.
		org_freedesktop_thumbnails_specialized_thumbnailer1_emit_error(
			thumbnailer->skeleton, handle, req->uri.c_str(),
			0, "URI is not describing a local file.");
		goto cleanup;
	}

	// TODO: Make sure the URI to thumbnail is not in the cache directory.

	// Make sure the thumbnail directory exists.
	cache_filename = cache_dir + "/thumbnails/";
	cache_filename += (req->large ? "large" : "normal");
	if (g_mkdir_with_parents(cache_filename.c_str(), 0777) != 0) {
		org_freedesktop_thumbnails_specialized_thumbnailer1_emit_error(
			thumbnailer->skeleton, handle, req->uri.c_str(),
			0, "Cannot mkdir() the thumbnail cache directory.");
		goto cleanup;
	}

	// Reference: https://specifications.freedesktop.org/thumbnail-spec/thumbnail-spec-latest.html
	// NOTE: glib-2.34 has g_compute_checksum_for_bytes().
	md5 = g_checksum_new(G_CHECKSUM_MD5);
	if (!md5) {
		// Cannot allocate an MD5...
		// TODO: Test for this early.
		org_freedesktop_thumbnails_specialized_thumbnailer1_emit_error(
			thumbnailer->skeleton, handle, req->uri.c_str(),
			0, "g_checksum_new() does not support MD5.");
		goto cleanup;
	}
	g_checksum_update(md5, reinterpret_cast<const guchar*>(req->uri.c_str()), req->uri.size());
	md5_string = g_checksum_get_string(md5);
	cache_filename += '/';
	cache_filename += md5_string;
	cache_filename += ".png";

	// Thumbnail the image.
	ret = pfn_rp_create_thumbnail(filename, cache_filename.c_str(),
		req->large ? 256 : 128);
	if (ret == 0) {
		// Image thumbnailed successfully.
		g_debug("rom-properties thumbnail: %s -> %s [OK]", filename, cache_filename.c_str());
		org_freedesktop_thumbnails_specialized_thumbnailer1_emit_ready(
			thumbnailer->skeleton, handle, req->uri.c_str());
	} else {
		// Error thumbnailing the image...
		g_debug("rom-properties thumbnail: %s -> %s [ERR=%d]", filename, cache_filename.c_str(), ret);
		org_freedesktop_thumbnails_specialized_thumbnailer1_emit_error(
			thumbnailer->skeleton, handle, req->uri.c_str(),
			2, "Image thumbnailing failed... (TODO: return code)");
	}

cleanup:
	// Request is finished. Emit the finished signal.
	org_freedesktop_thumbnails_specialized_thumbnailer1_emit_finished(thumbnailer->skeleton, handle);
	if (iter != thumbnailer->uri_map->end()) {
		// Remove the URI from the map.
		thumbnailer->uri_map->erase(iter);
	}

	// Free the checksum objects.
	g_checksum_free(md5);

	// Return TRUE if we still have more thumbnails queued.
	const bool isEmpty = thumbnailer->handle_queue->empty();
	if (isEmpty) {
		// Clear the idle process.
		// FIXME: Atomic compare and/or mutex? (assuming multi-threaded...)
		thumbnailer->idle_process = 0;

		// Restart the inactivity timeout.
		if (G_LIKELY(thumbnailer->timeout_id == 0)) {
			thumbnailer->timeout_id = g_timeout_add_seconds(SHUTDOWN_TIMEOUT_SECONDS,
				(GSourceFunc)rp_thumbnail_timeout, thumbnailer);
		}
	}
	return !isEmpty;
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
	if (level < LEVEL_ERROR)
		return 0;

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
		g_debug(buf);
	} else {
		g_warning(buf);
	}
	return ret;
}

/**
 * Shutdown callback.
 * @param thumbnailer RpThumbnail object.
 * @param main_loop GMainLoop.
 */
static void
shutdown_rp_thumbnail_dbus(RpThumbnail *thumbnailer, GMainLoop *main_loop)
{
	g_return_if_fail(IS_RP_THUMBNAIL(thumbnailer));
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

	// Get the cache directory.
	// TODO: Use $XDG_CACHE_HOME and/or getpwuid_r().
	const char *const home_env = getenv("HOME");
	if (home_env) {
		cache_dir = home_env;
	}
	// Remove trailing slashes.
	// NOTE: If $HOME is "/", this will result in an empty directory,
	// which will cause the program to exit. Not a big deal, since
	// that shouldn't happen...
	while (!cache_dir.empty() && cache_dir[cache_dir.size()-1] == '/') {
		cache_dir.resize(cache_dir.size()-1);
	}
	if (cache_dir.empty()) {
		g_warning("$HOME is not set.");
		return EXIT_FAILURE;
	}
	// Append "/.cache".
	cache_dir += "/.cache";

	// Attempt to open a ROM Properties Page library.
	void *pDll = NULL;
	int ret = rp_dll_search("rp_create_thumbnail", &pDll, (void**)&pfn_rp_create_thumbnail, fnDebug);
	if (ret != 0) {
		return EXIT_FAILURE;
	}

	GError *error = nullptr;
	connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
	if (error) {
		g_critical("Unable to connect to the session bus: %s", error->message);
		g_error_free(error);
		dlclose(pDll);
		return EXIT_FAILURE;
	}

	GMainLoop *main_loop = g_main_loop_new(nullptr, false);

	// Create the RpThumbnail service object.
	RpThumbnail *const thumbnailer = RP_THUMBNAIL(g_object_new(TYPE_RP_THUMBNAIL, nullptr));

	// Register the D-Bus service.
	g_bus_own_name_on_connection(connection,
		"com.gerbilsoft.rom-properties.SpecializedThumbnailer1",
		G_BUS_NAME_OWNER_FLAGS_NONE, nullptr, on_dbus_name_lost, main_loop, nullptr);

	if (thumbnailer->exported) {
		// Service object is exported.

		// Make sure we quit after the RpThumbnail server is idle for long enough.
		g_signal_connect(thumbnailer, "shutdown", G_CALLBACK(shutdown_rp_thumbnail_dbus), main_loop);

		// Run the main loop.
		if (!stop_main_loop) {
			g_debug("Starting the D-Bus service.");
			g_main_loop_run(main_loop);
		}
	}
	dlclose(pDll);
	return 0;
}
