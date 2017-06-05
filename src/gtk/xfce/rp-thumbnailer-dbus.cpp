/***************************************************************************
 * ROM Properties Page shell extension. (GNOME)                            *
 * rp-thumbnailer-dbus.cpp: D-Bus thumbnailer service.                     *
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

#include "rp-thumbnailer-dbus.hpp"
#include "librpbase/common.h"

#include <glib-object.h>
#include "SpecializedThumbnailer1.h"

// C++ includes.
#include <deque>
#include <string>
#include <unordered_map>
using std::deque;
using std::make_pair;
using std::string;
using std::unordered_map;

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
enum RpThumbnailerSignals {
	SIGNAL_0,

	// RpThumbnailer has been idle for long enough and should exit.
	SIGNAL_SHUTDOWN,

	SIGNAL_LAST
};

/* Property identifiers. */
enum RpThumbnailerProperties {
	PROP_0,

	PROP_CONNECTION,
	PROP_CACHE_DIR,
	PROP_PFN_RP_CREATE_THUMBNAIL,
	PROP_EXPORTED,

	PROP_LAST
};

// Internal functions.
static void	rp_thumbnailer_constructed	(GObject	*object);
static void	rp_thumbnailer_dispose		(GObject	*object);
static void	rp_thumbnailer_finalize		(GObject	*object);

static void	rp_thumbnailer_get_property	(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);
static void	rp_thumbnailer_set_property	(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);

static gboolean	rp_thumbnailer_timeout		(RpThumbnailer	*thumbnailer);
static gboolean	rp_thumbnailer_process		(RpThumbnailer	*thumbnailer);

// D-Bus methods.
static gboolean	rp_thumbnailer_queue		(OrgFreedesktopThumbnailsSpecializedThumbnailer1 *skeleton,
						 GDBusMethodInvocation *invocation,
						 const gchar	*uri,
						 const gchar	*mime_type,
						 const char	*flavor,
						 bool		 urgent,
						 RpThumbnailer	*thumbnailer);
static gboolean	rp_thumbnailer_dequeue		(OrgFreedesktopThumbnailsSpecializedThumbnailer1 *skeleton,
						 GDBusMethodInvocation *invocation,
						 guint		 handle,
						 RpThumbnailer	*thumbnailer);

struct _RpThumbnailerClass {
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

struct _RpThumbnailer {
	GObject __parent__;
	OrgFreedesktopThumbnailsSpecializedThumbnailer1 *skeleton;

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

	/** Properties. **/

	// D-Bus connection.
	GDBusConnection *connection;

	// Thumbnail cache directory.
	gchar *cache_dir;

	// rp_create_thumbnail() function pointer.
	PFN_RP_CREATE_THUMBNAIL pfn_rp_create_thumbnail;

	// Is the D-Bus object exported?
	bool exported;
};

/** Type information. **/
// NOTE: We can't use G_DEFINE_DYNAMIC_TYPE() here because
// that requires a GTypeModule, which we don't have.

static void     rp_thumbnailer_init		(RpThumbnailer      *thumbnailer);
static void     rp_thumbnailer_class_init		(RpThumbnailerClass *klass);

static gpointer rp_thumbnailer_parent_class = nullptr;

static void
rp_thumbnailer_class_intern_init(gpointer klass)
{
	  rp_thumbnailer_parent_class = g_type_class_peek_parent(klass);
	  rp_thumbnailer_class_init(static_cast<RpThumbnailerClass*>(klass));
}

GType
rp_thumbnailer_get_type(void)
{
	static GType rp_thumbnailer_type_id = 0;
	if (!rp_thumbnailer_type_id) {
		static const GTypeInfo g_define_type_info = {
			sizeof(RpThumbnailerClass),	// class_size
			nullptr,			// base_init
			nullptr,			// base_finalize
			(GClassInitFunc)rp_thumbnailer_class_intern_init,
			nullptr,			// class_finalize
			nullptr,			// class_data
			sizeof(RpThumbnailer),		// instance_size
			0,				// n_preallocs
			(GInstanceInitFunc)rp_thumbnailer_init,
			nullptr				// value_table
		};
		rp_thumbnailer_type_id = g_type_register_static(G_TYPE_OBJECT,
							"RpThumbnailer",
							&g_define_type_info,
							(GTypeFlags) 0);
	}
	return rp_thumbnailer_type_id;
}

/** End type information. **/

static void
rp_thumbnailer_class_init(RpThumbnailerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = rp_thumbnailer_dispose;
	gobject_class->finalize = rp_thumbnailer_finalize;
	gobject_class->constructed = rp_thumbnailer_constructed;
	gobject_class->get_property = rp_thumbnailer_get_property;
	gobject_class->set_property = rp_thumbnailer_set_property;

	// Register signals.
	klass->signal_ids[SIGNAL_0] = 0;

	// RpThumbnailer has been idle for long enough and should exit.
	klass->signal_ids[SIGNAL_SHUTDOWN] = g_signal_new("shutdown",
		TYPE_RP_THUMBNAILER, G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 0);

	// Register properties.
	g_object_class_install_property(gobject_class, PROP_CONNECTION,
		g_param_spec_object("connection", "connection", "D-Bus connection.",
			G_TYPE_DBUS_CONNECTION, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));
	g_object_class_install_property(gobject_class, PROP_CACHE_DIR,
		g_param_spec_string("cache_dir", "cache_dir", "XDG cache directory.",
			nullptr, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));
	g_object_class_install_property(gobject_class, PROP_PFN_RP_CREATE_THUMBNAIL,
		g_param_spec_pointer("pfn_rp_create_thumbnail", "pfn_rp_create_thumbnail",
			"rp_create_thumbnail() function pointer.",
			(GParamFlags)(G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));
	g_object_class_install_property(gobject_class, PROP_EXPORTED,
		g_param_spec_boolean("exported", "exported", "Is the D-Bus object exported?",
			false, G_PARAM_READABLE));
}

static void
rp_thumbnailer_init(RpThumbnailer *thumbnailer)
{
	thumbnailer->skeleton = nullptr;
	thumbnailer->shutdown_emitted = false;
	thumbnailer->timeout_id = 0;
	thumbnailer->idle_process = 0;
	thumbnailer->last_handle = 0;
	thumbnailer->handle_queue = new deque<guint>();
	thumbnailer->uri_map = new unordered_map<guint, request_info>();
	thumbnailer->uri_map->reserve(8);

	/** Properties. **/
	thumbnailer->connection = nullptr;
	thumbnailer->cache_dir = nullptr;
	thumbnailer->pfn_rp_create_thumbnail = nullptr;
	thumbnailer->exported = false;
}

static void
rp_thumbnailer_constructed(GObject *object)
{
	g_return_if_fail(IS_RP_THUMBNAILER(object));
	RpThumbnailer *const thumbnailer = RP_THUMBNAILER(object);

	GError *error = nullptr;
	thumbnailer->skeleton = org_freedesktop_thumbnails_specialized_thumbnailer1_skeleton_new();
	g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(thumbnailer->skeleton),
		thumbnailer->connection, "/com/gerbilsoft/rom_properties/SpecializedThumbnailer1", &error);
	if (error) {
		g_critical("Error exporting RpThumbnailer on session bus: %s", error->message);
		g_error_free(error);
		thumbnailer->exported = false;
	} else {
		// Connect signals to the relevant functions.
		g_signal_connect(thumbnailer->skeleton, "handle-queue",
			G_CALLBACK(rp_thumbnailer_queue), thumbnailer);
		g_signal_connect(thumbnailer->skeleton, "handle-dequeue",
			G_CALLBACK(rp_thumbnailer_dequeue), thumbnailer);
		
		// Make sure we shut down after inactivity.
		thumbnailer->timeout_id = g_timeout_add_seconds(SHUTDOWN_TIMEOUT_SECONDS,
			(GSourceFunc)rp_thumbnailer_timeout, thumbnailer);

		// Object is exported.
		thumbnailer->exported = true;
		g_object_notify(G_OBJECT(thumbnailer), "exported");
	}
}

static void
rp_thumbnailer_dispose(GObject *object)
{
	RpThumbnailer *const thumbnailer = RP_THUMBNAILER(object);

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

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_thumbnailer_parent_class)->dispose(object);
}

static void
rp_thumbnailer_finalize(GObject *object)
{
	RpThumbnailer *const thumbnailer = RP_THUMBNAILER(object);

	if (thumbnailer->skeleton) {
		g_object_unref(thumbnailer->skeleton);
	}

	delete thumbnailer->handle_queue;
	delete thumbnailer->uri_map;

	/** Properties. **/
	free(thumbnailer->cache_dir);

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rp_thumbnailer_parent_class)->finalize(object);
}

/**
 * Get a property from the RpThumbnailer.
 * @param object	[in] RpThumbnailer object.
 * @param prop_id	[in] Property ID.
 * @param value		[out] Value.
 * @param pspec		[in] Parameter specification.
 */
static void
rp_thumbnailer_get_property(GObject *object,
	guint prop_id, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(IS_RP_THUMBNAILER(object));
	RpThumbnailer *const thumbnailer = RP_THUMBNAILER(object);

	switch (prop_id) {
		case PROP_CONNECTION:
			g_value_set_object(value, thumbnailer->connection);
			break;
		case PROP_CACHE_DIR:
			g_value_set_string(value, thumbnailer->cache_dir);
			break;
		case PROP_PFN_RP_CREATE_THUMBNAIL:
			g_value_set_pointer(value, (gpointer)thumbnailer->pfn_rp_create_thumbnail);
			break;
		case PROP_EXPORTED:
			g_value_set_boolean(value, thumbnailer->exported);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/**
 * Set a property in the RpThumbnailer.
 * @param object	[in] RpThumbnailer object.
 * @param prop_id	[in] Property ID.
 * @param value		[in] Value.
 * @param pspec		[in] Parameter specification.
 */
static void
rp_thumbnailer_set_property(GObject *object,
	guint prop_id, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail(IS_RP_THUMBNAILER(object));
	RpThumbnailer *const thumbnailer = RP_THUMBNAILER(object);

	switch (prop_id) {
		case PROP_CONNECTION: {
			GDBusConnection *const old_connection = thumbnailer->connection;
			GDBusConnection *const connection = static_cast<GDBusConnection*>(g_value_get_object(value));
			// TODO: Atomic exchange?
			thumbnailer->connection = (connection
				? static_cast<GDBusConnection*>(g_object_ref(connection))
				: nullptr);
			if (old_connection) {
				g_object_unref(old_connection);
			}
			break;
		}

		case PROP_CACHE_DIR: {
			gchar *old_cache_dir = thumbnailer->cache_dir;
			thumbnailer->cache_dir = g_value_dup_string(value);
			g_free(old_cache_dir);
			break;
		}

		case PROP_PFN_RP_CREATE_THUMBNAIL:
			thumbnailer->pfn_rp_create_thumbnail =
				(PFN_RP_CREATE_THUMBNAIL)g_value_get_pointer(value);
			break;

		case PROP_EXPORTED:
			// FIXME: Read-only property.
			// Need to show some error message...
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
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
rp_thumbnailer_queue(OrgFreedesktopThumbnailsSpecializedThumbnailer1 *skeleton,
	GDBusMethodInvocation *invocation,
	const gchar *uri, const gchar *mime_type,
	const char *flavor, bool urgent,
	RpThumbnailer *thumbnailer)
{
	g_dbus_async_return_val_if_fail(IS_RP_THUMBNAILER(thumbnailer), invocation, false);
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
		thumbnailer->idle_process = g_idle_add((GSourceFunc)rp_thumbnailer_process, thumbnailer);
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
rp_thumbnailer_dequeue(OrgFreedesktopThumbnailsSpecializedThumbnailer1 *skeleton,
	GDBusMethodInvocation *invocation,
	guint handle,
	RpThumbnailer *thumbnailer)
{
	g_dbus_async_return_val_if_fail(IS_RP_THUMBNAILER(thumbnailer), invocation, false);
	g_dbus_async_return_val_if_fail(handle != 0, invocation, false);

	// TODO
	org_freedesktop_thumbnails_specialized_thumbnailer1_complete_dequeue(skeleton, invocation);
	return true;
}

/**
 * Inactivity timeout has elapsed.
 * @param thumbnailer RpThumbnailer object.
 */
static gboolean
rp_thumbnailer_timeout(RpThumbnailer *thumbnailer)
{
	g_return_val_if_fail(IS_RP_THUMBNAILER(thumbnailer), false);
	if (!thumbnailer->handle_queue->empty()) {
		// Still processing stuff.
		return true;
	}

	// Stop the timeout and shut down the thumbnailer.
	RpThumbnailerClass *const klass = RP_THUMBNAILER_GET_CLASS(thumbnailer);
	thumbnailer->timeout_id = 0;
	thumbnailer->shutdown_emitted = true;
	g_signal_emit(thumbnailer, klass->signal_ids[SIGNAL_SHUTDOWN], 0);
	g_debug("Shutting down due to %u seconds of inactivity.", SHUTDOWN_TIMEOUT_SECONDS);
	return false;
}

/**
 * Process a thumbnail.
 * @param thumbnailer RpThumbnailer object.
 */
static gboolean
rp_thumbnailer_process(RpThumbnailer *thumbnailer)
{
	g_return_val_if_fail(IS_RP_THUMBNAILER(thumbnailer), FALSE);

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

	// NOTE: cache_dir and pfn_rp_create_thumbnail should NOT be nullptr
	// at this point, but we're checking it anyway.
	if (!thumbnailer->cache_dir || thumbnailer->cache_dir[0] == 0) {
		// No cache directory...
		org_freedesktop_thumbnails_specialized_thumbnailer1_emit_error(
			thumbnailer->skeleton, handle, "",
			0, "Thumbnail cache directory is empty.");
		goto cleanup;
	}
	if (!thumbnailer->pfn_rp_create_thumbnail) {
		// No thumbnailer function.
		org_freedesktop_thumbnails_specialized_thumbnailer1_emit_error(
			thumbnailer->skeleton, handle, "",
			0, "No thumbnailer function is available.");
		goto cleanup;
	}

	// TODO: Make sure the URI to thumbnail is not in the cache directory.

	// Make sure the thumbnail directory exists.
	cache_filename = thumbnailer->cache_dir;
	cache_filename += "/thumbnails/";
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
	ret = thumbnailer->pfn_rp_create_thumbnail(filename, cache_filename.c_str(),
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
				(GSourceFunc)rp_thumbnailer_timeout, thumbnailer);
		}
	}
	return !isEmpty;
}

/**
 * Create an RpThumbnailer object.
 * @param connection			[in] GDBusConnection
 * @param cache_dir			[in] Cache directory.
 * @param pfn_rp_create_thumbnail	[in] rp_create_thumbnail() function pointer.
 * @return RpThumbnailer object.
 */
RpThumbnailer*
rp_thumbnailer_new(GDBusConnection *connection,
	const gchar *cache_dir,
	PFN_RP_CREATE_THUMBNAIL pfn_rp_create_thumbnail)
{
	return static_cast<RpThumbnailer*>(
		g_object_new(TYPE_RP_THUMBNAILER,
			"connection", connection,
			"cache_dir", cache_dir,
			"pfn_rp_create_thumbnail", pfn_rp_create_thumbnail,
			nullptr));
}

/**
 * Is the RpThumbnailer object exported?
 * @return True if it's exported; false if it isn't.
 */
gboolean
rp_thumbnailer_is_exported(RpThumbnailer *thumbnailer)
{
	g_return_val_if_fail(IS_RP_THUMBNAILER(thumbnailer), false);
	return thumbnailer->exported;
}
