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

#include <glib-object.h>
#include <dbus/dbus-glib-bindings.h>
#include "rp-thumbnail-server-bindings.h"

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

/* Signal identifiers */
enum RpThumbnailSignals {
	SIGNAL_0,
	SIGNAL_READY,
	SIGNAL_STARTED,
	SIGNAL_FINISHED,
	SIGNAL_ERROR,

	SIGNAL_LAST
};

// Internal functions.
static void	rp_thumbnail_dispose		(GObject	*object);
static void	rp_thumbnail_finalize		(GObject	*object);
static gboolean	rp_thumbnail_process		(gpointer	 data);

struct _RpThumbnailClass {
	GObjectClass __parent__;
	DBusGConnection *connection;
	guint signal_ids[SIGNAL_LAST];
};

struct _RpThumbnail {
	GObject __parent__;

	// Is the D-Bus service registered?
	bool registered;

	// Idle function for processing.
	guint process_idle;

	// Last handle value.
	guint last_handle;

	// URI queue.
	// Note that queued thumbnail requests are
	// referenced by handle, so we store the
	// handles in a deque and the URIs in a map.
	deque<guint> *handle_queue;
	unordered_map<guint, string> *uri_map;
};

/** Type information. **/
// NOTE: We can't use G_DEFINE_DYNAMIC_TYPE() here because
// that requires a GTypeModule, which we don't have.

static void     rp_thumbnail_init              (RpThumbnail      *thumbnailer);
static void     rp_thumbnail_class_init        (RpThumbnailClass *klass);

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

	// Register signals.
	klass->signal_ids[SIGNAL_0] = 0;
	klass->signal_ids[SIGNAL_READY] = g_signal_new("ready",
		TYPE_RP_THUMBNAIL, G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING, 0);
	klass->signal_ids[SIGNAL_STARTED] = g_signal_new("started",
		TYPE_RP_THUMBNAIL, G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 1, G_TYPE_UINT, 0);
	klass->signal_ids[SIGNAL_FINISHED] = g_signal_new("finished",
		TYPE_RP_THUMBNAIL, G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 1, G_TYPE_UINT, 0);
	klass->signal_ids[SIGNAL_ERROR] = g_signal_new("error",
		TYPE_RP_THUMBNAIL, G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 4, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, 0);

	// Initialize the D-Bus connection, per-klass.
	GError *error = nullptr;
	klass->connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	if (!klass->connection) {
		g_warning("Unable to connect to D-Bus: %s", error->message);
		g_error_free(error);
		return;
	}

	// Register introspection data.
	dbus_g_object_type_install_info(TYPE_RP_THUMBNAIL, &dbus_glib_rp_thumbnail_object_info);
}

static void
rp_thumbnail_init(RpThumbnail *thumbnailer)
{
	thumbnailer->registered = false;
	thumbnailer->process_idle = 0;
	thumbnailer->last_handle = 0;
	thumbnailer->handle_queue = new deque<guint>();
	thumbnailer->uri_map = new unordered_map<guint, string>();
	thumbnailer->uri_map->reserve(8);

	GError *error = NULL;
	DBusGProxy *driver_proxy;
	RpThumbnailClass *klass = RP_THUMBNAIL_GET_CLASS(thumbnailer);
	guint request_ret;

	// Register D-Bus path.
	dbus_g_connection_register_g_object(klass->connection,
		"/com/gerbilsoft/rom_properties/SpecializedThumbnailer1",
		G_OBJECT(thumbnailer));

	// Register the service name.
	driver_proxy = dbus_g_proxy_new_for_name(klass->connection,
		DBUS_SERVICE_DBUS,
		DBUS_PATH_DBUS,
		DBUS_INTERFACE_DBUS);

	int res = org_freedesktop_DBus_request_name(driver_proxy,
		"com.gerbilsoft.rom-properties.SpecializedThumbnailer1",
		DBUS_NAME_FLAG_DO_NOT_QUEUE, &request_ret, &error);
	if (res == 1) {
		// The D-Bus call succeeded.
		// Check the return value.
		switch (request_ret) {
			case 0:
			default:
				// An unknown error occurred.
				g_warning("Unable to register service: res == %d", request_ret);
				dbus_g_connection_unregister_g_object(klass->connection, G_OBJECT(thumbnailer));
				break;

			case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
			case DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER:
				// D-Bus object registered successfully.
				thumbnailer->registered = true;
				break;

			case DBUS_REQUEST_NAME_REPLY_IN_QUEUE:
			case DBUS_REQUEST_NAME_REPLY_EXISTS:
				// Some other process has already registered this name.
				g_warning("This thumbnailer is already registered; exiting.");
				dbus_g_connection_unregister_g_object(klass->connection, G_OBJECT(thumbnailer));
				break;
		}
	} else {
		// The D-Bus call failed.
		g_warning("Unable to register service: %s", error->message);
		g_error_free(error);
		dbus_g_connection_unregister_g_object(klass->connection, G_OBJECT(thumbnailer));
	}

	g_object_unref(driver_proxy);
}

static void
rp_thumbnail_dispose(GObject *object)
{
	RpThumbnail *const thumbnailer = RP_THUMBNAIL(object);

	// Unregister process_idle.
	if (G_UNLIKELY(thumbnailer->process_idle != 0)) {
		g_source_remove(thumbnailer->process_idle);
		thumbnailer->process_idle = 0;
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
 * @param thumbnailer RpThumbnailer object.
 * @param uri URI to thumbnail.
 * @param mime_type MIME type of the URI.
 * @param flavor The flavor that should be made, e.g. "normal".
 * @return Request handle.
 */
guint rp_thumbnail_queue(RpThumbnail *thumbnailer,
	const gchar *uri, const gchar *mime_type,
	const char *flavor, bool urgent)
{
	// Queue the URI for processing.
	// TODO: Handle 'flavor', 'urgent', etc.
	guint handle = ++thumbnailer->last_handle;
	if (G_UNLIKELY(handle == 0)) {
		// Overflow. Increment again so we
		// don't return a handle of 0.
		handle = ++thumbnailer->last_handle;
	}

	// Add the URI to the queue.
	thumbnailer->uri_map->insert(make_pair(handle, uri));
	thumbnailer->handle_queue->push_back(handle);

	// Make sure the idle process is started.
	// FIXME: Atomic compare and/or mutex? (assuming multi-threaded...)
	if (thumbnailer->process_idle == 0) {
		thumbnailer->process_idle = g_idle_add(rp_thumbnail_process, thumbnailer);
	}

	return handle;
}

gboolean rp_thumbnail_dequeue(RpThumbnail *thumbnailer,
	unsigned int handle, GError **error)
{
	// TODO
	return false;
}

/**
 * Process a thumbnail.
 * @param data RpThumbnail object.
 */
static gboolean
rp_thumbnail_process(gpointer data)
{
	RpThumbnail *const thumbnailer = RP_THUMBNAIL(data);
	g_return_val_if_fail(thumbnailer != nullptr || IS_RP_THUMBNAIL(thumbnailer), FALSE);

	RpThumbnailClass *const klass = RP_THUMBNAIL_GET_CLASS(data);

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
	if (iter == thumbnailer->uri_map->end()) {
		// URI not found.
		g_signal_emit(thumbnailer, klass->signal_ids[SIGNAL_ERROR], 0,
			 handle, "", 0, "Handle has no associated URI.");
		goto cleanup;
	}

	// Verify that the specified URI is local.
	// TODO: Support GVFS.
	filename = g_filename_from_uri(iter->second.c_str(), nullptr, nullptr);
	if (!filename) {
		// URI is not describing a local file.
		g_signal_emit(thumbnailer, klass->signal_ids[SIGNAL_ERROR], 0,
			 handle, iter->second.c_str(), 0, "URI is not describing a local file.");
		goto cleanup;
	}

	// TODO: Hard-coding "flavor" as "normal" right now.
	// TODO: Make sure the URI to thumbnail is not in the cache directory.

	// Make sure the thumbnail directory exists.
	cache_filename = cache_dir + "/thumbnails/normal";
	if (g_mkdir_with_parents(cache_filename.c_str(), 0777) != 0) {
		g_signal_emit(thumbnailer, klass->signal_ids[SIGNAL_ERROR], 0,
			 handle, iter->second.c_str(), 0, "Cannot mkdir() the thumbnail cache directory.");
		goto cleanup;
	}

	// TODO: Handle "flavor", check $XDG_CACHE_HOME.
	// Reference: https://specifications.freedesktop.org/thumbnail-spec/thumbnail-spec-latest.html
	// For now, creating "normal" 256x256 thumbnails.
	// NOTE: glib-2.34 has g_compute_checksum_for_bytes().
	md5 = g_checksum_new(G_CHECKSUM_MD5);
	if (!md5) {
		// Cannot allocate an MD5...
		// TODO: Test for this early.
		// FIXME: failed_uris should be an array of strings.
		g_signal_emit(thumbnailer, klass->signal_ids[SIGNAL_ERROR], 0,
			 handle, iter->second.c_str(), 0, "g_checksum_new() does not support MD5.");
		goto cleanup;
	}
	g_checksum_update(md5, reinterpret_cast<const guchar*>(iter->second.c_str()), iter->second.size());
	md5_string = g_checksum_get_string(md5);
	cache_filename += '/';
	cache_filename += md5_string;
	cache_filename += ".png";

	// Thumbnail the iamge.
	ret = pfn_rp_create_thumbnail(filename, cache_filename.c_str(), 256);
	if (ret == 0) {
		// Image thumbnailed successfully.
		g_debug("rom-properties thumbnail: %s -> %s [OK]", filename, cache_filename.c_str());
		g_signal_emit(thumbnailer, klass->signal_ids[SIGNAL_READY], 0, handle, iter->second.c_str());
	} else {
		// Error thumbnailing the image...
		g_signal_emit(thumbnailer, klass->signal_ids[SIGNAL_ERROR], 0,
			 handle, iter->second.c_str(), 2, "Image thumbnailing failed... (TODO: return code)");
		g_debug("rom-properties thumbnail: %s -> %s [ERR=%d]", filename, cache_filename.c_str(), ret);
	}

cleanup:
	// Request is finished. Emit the finished signal.
	g_signal_emit(thumbnailer, klass->signal_ids[SIGNAL_FINISHED], 0, handle);
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
		thumbnailer->process_idle = 0;
	}
	return !isEmpty;
}

int main(int argc, char *argv[])
{
	RP_UNUSED(argc);
	RP_UNUSED(argv);

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
		fprintf(stderr, "*** ERROR: $HOME is not set.");
		return EXIT_FAILURE;
	}
	// Append "/.cache".
	cache_dir += "/.cache";

	// Attempt to open rom-properties-xfce.so.
	// TODO: Use rp-stub's DLL search code.
	void *hRpPlugin = dlopen("/usr/lib64/thunarx-2/rom-properties-xfce.so", RTLD_LOCAL|RTLD_LAZY);
	if (!hRpPlugin) {
		fprintf(stderr, "*** ERROR: Cannot dlopen() rom-properties-xfce.so.\n");
		return EXIT_FAILURE;
	}
	pfn_rp_create_thumbnail = (PFN_RP_CREATE_THUMBNAIL)dlsym(hRpPlugin, "rp_create_thumbnail");
	if (!pfn_rp_create_thumbnail) {
		dlclose(hRpPlugin);
		fprintf(stderr, "*** ERROR: rom-properties-xfce.so does not have rp_create_thumbnail().\n");
		return EXIT_FAILURE;
	}

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

	dbus_g_thread_init();
	GMainLoop *loop = g_main_loop_new(nullptr, false);

	// Initialize the D-Bus server.
	// TODO: Distinguish between "already running" and "error"
	// and return non-zero in the error case.
	RpThumbnail *server = RP_THUMBNAIL(g_object_new(TYPE_RP_THUMBNAIL, nullptr));
	if (server->registered) {
		// Server is registered.
		// Run the main loop.
		g_main_loop_run(loop);
	}
	dlclose(hRpPlugin);
	return 0;
}
