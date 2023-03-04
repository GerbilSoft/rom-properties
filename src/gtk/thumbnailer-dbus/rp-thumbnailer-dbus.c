/***************************************************************************
 * ROM Properties Page shell extension. (D-Bus Thumbnailer)                *
 * rp-thumbnailer-dbus.c: D-Bus thumbnailer service.                       *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - https://github.com/linneman/dbus-example
 */

#include "rp-thumbnailer-dbus.h"
#include "common.h"

#include <glib-object.h>
#include "SpecializedThumbnailer1.h"

// C includes.
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// from tumbler-utils.h
#define g_dbus_async_return_val_if_fail(expr, invocation, val) \
G_STMT_START { \
	if (G_UNLIKELY(!(expr))) { \
		GError *dbus_async_return_if_fail_error = NULL; \
		g_set_error(&dbus_async_return_if_fail_error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, \
			"Assertion \"%s\" failed", #expr); \
		g_dbus_method_invocation_return_gerror(invocation, dbus_async_return_if_fail_error); \
		g_clear_error(&dbus_async_return_if_fail_error); \
		return (val); \
	} \
} G_STMT_END

/* Property identifiers. */
enum RpThumbnailerProperties {
	PROP_0,

	PROP_CONNECTION,
	PROP_CACHE_DIR,
	PROP_PFN_RP_CREATE_THUMBNAIL2,
	PROP_EXPORTED,

	PROP_LAST
};

/* Signal identifiers */
enum RpThumbnailerSignals {
	SIGNAL_SHUTDOWN,	// RpThumbnailer has been idle for long enough and should exit.

	SIGNAL_LAST
};

// Internal functions.
static void	rp_thumbnailer_constructed	(GObject	*object);
static void	rp_thumbnailer_dispose		(GObject	*object);
static void	rp_thumbnailer_finalize		(GObject	*object);

static void	rp_thumbnailer_set_property	(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);
static void	rp_thumbnailer_get_property	(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);

static gboolean	rp_thumbnailer_timeout		(RpThumbnailer	*thumbnailer);
static gboolean	rp_thumbnailer_process		(RpThumbnailer	*thumbnailer);

// D-Bus methods.
static gboolean	rp_thumbnailer_queue		(SpecializedThumbnailer1 *skeleton,
						 GDBusMethodInvocation *invocation,
						 const gchar	*uri,
						 const gchar	*mime_type,
						 const char	*flavor,
						 bool		 urgent,
						 RpThumbnailer	*thumbnailer);
static gboolean	rp_thumbnailer_dequeue		(SpecializedThumbnailer1 *skeleton,
						 GDBusMethodInvocation *invocation,
						 guint32	 handle,
						 RpThumbnailer	*thumbnailer);

static GParamSpec *props[PROP_LAST];
static guint signals[SIGNAL_LAST];

struct _RpThumbnailerClass {
	GObjectClass __parent__;
};

#define SHUTDOWN_TIMEOUT_SECONDS 30U

// Thumbnail request information.
struct request_info {
	gchar *uri;
	guint32 handle;
	bool large;	// False for 'normal' (128x128); true for 'large' (256x256)
	bool urgent;	// 'urgent' value
};

struct _RpThumbnailer {
	GObject __parent__;
	SpecializedThumbnailer1 *skeleton;

	// Has the shutdown signal been emitted?
	bool shutdown_emitted;

	// Shutdown timeout.
	guint timeout_id;

	// Idle function for processing.
	guint idle_process;

	// Last handle value.
	guint32 last_handle;

	// Request queue.
	GQueue request_queue;	// element is struct request_info*

	/** Properties. **/

	// D-Bus connection.
	GDBusConnection *connection;

	// Thumbnail cache directory.
	gchar *cache_dir;

	// rp_create_thumbnail2() function pointer.
	PFN_RP_CREATE_THUMBNAIL2 pfn_rp_create_thumbnail2;

	// Is the D-Bus object exported?
	bool exported;
};

/** Type information. **/
// NOTE: We can't use G_DEFINE_DYNAMIC_TYPE() here because
// that requires a GTypeModule, which we don't have.

static void	rp_thumbnailer_class_init	(RpThumbnailerClass *klass, gpointer class_data);
static void	rp_thumbnailer_init		(RpThumbnailer *instance, gpointer g_class);

static gpointer	rp_thumbnailer_parent_class = NULL;

GType
rp_thumbnailer_get_type(void)
{
	static GType rp_thumbnailer_type_id = 0;
	if (!rp_thumbnailer_type_id) {
		static const GTypeInfo g_define_type_info = {
			sizeof(RpThumbnailerClass),			// class_size
			NULL,						// base_init
			NULL,						// base_finalize
			(GClassInitFunc)rp_thumbnailer_class_init,	// class_init
			NULL,						// class_finalize
			NULL,						// class_data
			sizeof(RpThumbnailer),				// instance_size
			0,						// n_preallocs
			(GInstanceInitFunc)rp_thumbnailer_init,		// instance_init
			NULL						// value_table
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
rp_thumbnailer_class_init(RpThumbnailerClass *klass, gpointer class_data)
{
	RP_UNUSED(class_data);
	rp_thumbnailer_parent_class = g_type_class_peek_parent(klass);

	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = rp_thumbnailer_dispose;
	gobject_class->finalize = rp_thumbnailer_finalize;
	gobject_class->constructed = rp_thumbnailer_constructed;
	gobject_class->set_property = rp_thumbnailer_set_property;
	gobject_class->get_property = rp_thumbnailer_get_property;

	/** Properties **/

	props[PROP_CONNECTION] = g_param_spec_object(
		"connection", "connection", "D-Bus connection.",
		G_TYPE_DBUS_CONNECTION,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	props[PROP_CACHE_DIR] = g_param_spec_string(
		"cache-dir", "cache-dir", "XDG cache directory.",
		NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	props[PROP_PFN_RP_CREATE_THUMBNAIL2] = g_param_spec_pointer(
		"pfn-rp-create-thumbnail2", "pfn-rp-create-thumbnail2", "rp_create_thumbnail2() function pointer.",
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	props[PROP_EXPORTED] = g_param_spec_boolean(
		"exported", "exported", "Is the D-Bus object exported?",
		false,
		G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);

	/** Signals **/

	// RpThumbnailer has been idle for long enough and should exit.
	signals[SIGNAL_SHUTDOWN] = g_signal_new("shutdown",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL,
		G_TYPE_NONE, 0);
}

static void
rp_thumbnailer_init(RpThumbnailer *thumbnailer, gpointer g_class)
{
	// g_object_new() guarantees that all values are initialized to 0.
	// Nothing to do here.
	RP_UNUSED(thumbnailer);
	RP_UNUSED(g_class);
}

static void
rp_thumbnailer_constructed(GObject *object)
{
	g_return_if_fail(RP_IS_THUMBNAILER(object));
	RpThumbnailer *const thumbnailer = RP_THUMBNAILER(object);

	GError *error = NULL;
	thumbnailer->skeleton = specialized_thumbnailer1_skeleton_new();
	g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(thumbnailer->skeleton),
		thumbnailer->connection, "/com/gerbilsoft/rom_properties/SpecializedThumbnailer1", &error);
	if (error) {
		g_critical("Error exporting RpThumbnailer on session bus: %s", error->message);
		g_error_free(error);
		thumbnailer->exported = false;
		return;
	}

	// Connect signals to the relevant functions.
	g_signal_connect(thumbnailer->skeleton, "handle-queue",
		G_CALLBACK(rp_thumbnailer_queue), thumbnailer);
	g_signal_connect(thumbnailer->skeleton, "handle-dequeue",
		G_CALLBACK(rp_thumbnailer_dequeue), thumbnailer);
		
	// Make sure we shut down after inactivity.
	thumbnailer->timeout_id = g_timeout_add_seconds(SHUTDOWN_TIMEOUT_SECONDS,
		G_SOURCE_FUNC(rp_thumbnailer_timeout), thumbnailer);

	// Object is exported.
	thumbnailer->exported = true;
	g_object_notify_by_pspec(G_OBJECT(thumbnailer), props[PROP_EXPORTED]);
}

static void
rp_thumbnailer_dispose(GObject *object)
{
	RpThumbnailer *const thumbnailer = RP_THUMBNAILER(object);

	// Unexport the object.
	if (G_LIKELY(thumbnailer->exported)) {
		g_dbus_interface_skeleton_unexport(G_DBUS_INTERFACE_SKELETON(thumbnailer->skeleton));
		// TODO: Do we call g_object_notify_by_pspec() here?
		thumbnailer->exported = false;
	}

	// Unregister timer sources.
	g_clear_handle_id(&thumbnailer->timeout_id, g_source_remove);
	g_clear_handle_id(&thumbnailer->idle_process, g_source_remove);

	/** Properties **/
	g_clear_object(&thumbnailer->connection);

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_thumbnailer_parent_class)->dispose(object);
}

static void
rp_thumbnailer_finalize(GObject *object)
{
	RpThumbnailer *const thumbnailer = RP_THUMBNAILER(object);
	g_clear_object(&thumbnailer->skeleton);

	// Delete any remaining requests and free the queue.
	for (GList *p = thumbnailer->request_queue.head; p != NULL; p = p->next) {
		if (p->data) {
			struct request_info *const req = (struct request_info*)p->data;
			g_free(req->uri);
			g_free(req);
		}
	}
	g_queue_clear(&thumbnailer->request_queue);

	/** Properties **/
	g_free(thumbnailer->cache_dir);

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rp_thumbnailer_parent_class)->finalize(object);
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
	g_return_if_fail(RP_IS_THUMBNAILER(object));
	RpThumbnailer *const thumbnailer = RP_THUMBNAILER(object);

	switch (prop_id) {
		case PROP_CONNECTION: {
			g_clear_object(&thumbnailer->connection);

			GDBusConnection *const connection = (GDBusConnection*)g_value_get_object(value);
			thumbnailer->connection = (G_IS_DBUS_CONNECTION(connection)
				? (GDBusConnection*)g_object_ref(connection)
				: NULL);
			break;
		}

		case PROP_CACHE_DIR: {
			g_free(thumbnailer->cache_dir);
			thumbnailer->cache_dir = g_value_dup_string(value);
			break;
		}

		case PROP_PFN_RP_CREATE_THUMBNAIL2:
			thumbnailer->pfn_rp_create_thumbnail2 =
				(PFN_RP_CREATE_THUMBNAIL2)g_value_get_pointer(value);
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
	g_return_if_fail(RP_IS_THUMBNAILER(object));
	RpThumbnailer *const thumbnailer = RP_THUMBNAILER(object);

	switch (prop_id) {
		case PROP_CONNECTION:
			g_value_set_object(value, thumbnailer->connection);
			break;
		case PROP_CACHE_DIR:
			g_value_set_string(value, thumbnailer->cache_dir);
			break;
		case PROP_PFN_RP_CREATE_THUMBNAIL2:
			g_value_set_pointer(value, (gpointer)thumbnailer->pfn_rp_create_thumbnail2);
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
rp_thumbnailer_queue(SpecializedThumbnailer1 *skeleton,
	GDBusMethodInvocation *invocation,
	const gchar *uri, const gchar *mime_type,
	const char *flavor, bool urgent,
	RpThumbnailer *thumbnailer)
{
	RP_UNUSED(mime_type);
	g_dbus_async_return_val_if_fail(RP_IS_THUMBNAILER(thumbnailer), invocation, false);
	g_dbus_async_return_val_if_fail(uri != NULL, invocation, false);

	if (G_UNLIKELY(thumbnailer->shutdown_emitted)) {
		// The shutdown signal was emitted.
		// Can't queue anything else.
		g_dbus_method_invocation_return_error(invocation,
			G_DBUS_ERROR, G_DBUS_ERROR_NO_SERVER, "Service is shutting down.");
		return true;
	}

	// Stop the inactivity timeout.
	g_clear_handle_id(&thumbnailer->timeout_id, g_source_remove);

	// Queue the URI for processing.
	guint32 handle = ++thumbnailer->last_handle;
	if (G_UNLIKELY(handle == 0)) {
		// Overflow. Increment again so we
		// don't return a handle of 0.
		handle = ++thumbnailer->last_handle;
	}

	// Add the URI to the queue.
	// NOTE: Currently handling all flavors that aren't "large" as "normal".
	struct request_info *const req = g_malloc(sizeof(struct request_info));
	req->uri = g_strdup(uri);
	req->handle = handle;
	req->large = flavor && (g_ascii_strcasecmp(flavor, "large") == 0);
	req->urgent = urgent;
	// TODO Put 'urgent' requests at the front of the queue?
	g_queue_push_tail(&thumbnailer->request_queue, req);

	// Make sure the idle process is started.
	// TODO: Make it multi-threaded? (needs atomic compare and/or mutex...)
	if (thumbnailer->idle_process == 0) {
		thumbnailer->idle_process = g_idle_add(G_SOURCE_FUNC(rp_thumbnailer_process), thumbnailer);
	}

	specialized_thumbnailer1_complete_queue(skeleton, invocation, handle);
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
rp_thumbnailer_dequeue(SpecializedThumbnailer1 *skeleton,
	GDBusMethodInvocation *invocation,
	guint32 handle,
	RpThumbnailer *thumbnailer)
{
	g_dbus_async_return_val_if_fail(RP_IS_THUMBNAILER(thumbnailer), invocation, false);
	g_dbus_async_return_val_if_fail(handle != 0, invocation, false);

	// TODO
	specialized_thumbnailer1_complete_dequeue(skeleton, invocation);
	return true;
}

/**
 * Inactivity timeout has elapsed.
 * @param thumbnailer RpThumbnailer object.
 */
static gboolean
rp_thumbnailer_timeout(RpThumbnailer *thumbnailer)
{
	g_return_val_if_fail(RP_IS_THUMBNAILER(thumbnailer), false);
	if (!g_queue_is_empty(&thumbnailer->request_queue)) {
		// Still processing stuff.
		return true;
	}

	// Stop the timeout and shut down the thumbnailer.
	thumbnailer->timeout_id = 0;
	thumbnailer->shutdown_emitted = true;
	g_signal_emit(thumbnailer, signals[SIGNAL_SHUTDOWN], 0);
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
	g_return_val_if_fail(RP_IS_THUMBNAILER(thumbnailer), FALSE);

	gchar *md5_string = NULL;	// owned by us
	const struct request_info *req;	// request info from the map
	gchar *cache_filename = NULL;	// cache filename (g_strdup_printf())
	size_t cache_filename_sz;	// size of cache_filename
	int pos, pos2;			// snprintf() position
	int ret;

	// Process one thumbnail.
	req = (struct request_info*)g_queue_pop_head(&thumbnailer->request_queue);
	if (req == NULL) {
		// Nothing in the queue.
		goto cleanup;
	}

	// NOTE: cache_dir and pfn_rp_create_thumbnail2 should NOT be NULL
	// at this point, but we're checking it anyway.
	if (!thumbnailer->cache_dir || thumbnailer->cache_dir[0] == 0) {
		// No cache directory...
		specialized_thumbnailer1_emit_error(
			thumbnailer->skeleton, req->handle, "",
			0, "Thumbnail cache directory is empty.");
		goto finished;
	}
	if (!thumbnailer->pfn_rp_create_thumbnail2) {
		// No thumbnailer function.
		specialized_thumbnailer1_emit_error(
			thumbnailer->skeleton, req->handle, "",
			0, "No thumbnailer function is available.");
		goto finished;
	}

	// TODO: Make sure the URI to thumbnail is not in the cache directory.

	// Make sure the thumbnail directory exists.
	// g_malloc() sizes:
	// - "/thumbnails/" == 12
	// - "large" / "normal" == 6
	// - "/" == 1
	// - MD5 as a string == 32
	// - ".png" == 4
	// NULL terminator == 1
	cache_filename_sz = strlen(thumbnailer->cache_dir) + 12 + 6 + 1 + 32 + 4 + 1;
	cache_filename = g_malloc(cache_filename_sz);
	pos = snprintf(cache_filename, cache_filename_sz, "%s/thumbnails/%s",
		thumbnailer->cache_dir, (req->large ? "large" : "normal"));
	// pos does NOT include the NULL terminator, so check >=.
	if (pos < 0 || ((size_t)pos + 1 + 32 + 4) > cache_filename_sz) {
		// Not enough memory.
		specialized_thumbnailer1_emit_error(
			thumbnailer->skeleton, req->handle, req->uri,
			0, "Cannot snprintf() the thumbnail cache directory name.");
		goto finished;
	}

	if (g_mkdir_with_parents(cache_filename, 0777) != 0) {
		specialized_thumbnailer1_emit_error(
			thumbnailer->skeleton, req->handle, req->uri,
			0, "Cannot mkdir() the thumbnail cache directory.");
		goto finished;
	}

	// Reference: https://specifications.freedesktop.org/thumbnail-spec/thumbnail-spec-latest.html
	md5_string = g_compute_checksum_for_data(G_CHECKSUM_MD5, (const guchar*)req->uri, strlen(req->uri));
	if (!md5_string) {
		// Cannot compute the checksum...
		specialized_thumbnailer1_emit_error(
			thumbnailer->skeleton, req->handle, req->uri,
			0, "g_compute_checksum_for_data() failed.");
		goto finished;
	}

	// Append the MD5.
	pos2 = snprintf(&cache_filename[pos], cache_filename_sz - pos, "/%s.png", md5_string);
	// pos and pos2 do NOT include the NULL terminator, so check >=.
	if (pos2 < 0 || ((size_t)pos + (size_t)pos2) >= cache_filename_sz) {
		// Not enough memory.
		specialized_thumbnailer1_emit_error(
			thumbnailer->skeleton, req->handle, req->uri,
			0, "Cannot snprintf() the thumbnail filename.");
		goto finished;
	}

	// Thumbnail the image.
	ret = thumbnailer->pfn_rp_create_thumbnail2(req->uri, cache_filename, req->large ? 256 : 128, 0);
	if (ret == 0) {
		// Image thumbnailed successfully.
		g_debug("rom-properties thumbnail: %s -> %s [OK]", req->uri, cache_filename);
		specialized_thumbnailer1_emit_ready(
			thumbnailer->skeleton, req->handle, req->uri);
	} else {
		// Error thumbnailing the image...
		g_debug("rom-properties thumbnail: %s -> %s [ERR=%d]", req->uri, cache_filename, ret);
		specialized_thumbnailer1_emit_error(
			thumbnailer->skeleton, req->handle, req->uri,
			2, "Image thumbnailing failed... (TODO: return code)");
	}

finished:
	// Request is finished. Emit the finished signal.
	specialized_thumbnailer1_emit_finished(
		thumbnailer->skeleton, req->handle);

cleanup:
	// Free allocated things.
	g_free(cache_filename);
	g_free(md5_string);

	// req was allocated using g_malloc() before it was
	// added to the queue. We'll need to free it here.
	g_free((gpointer)req);

	// Return TRUE if we still have more thumbnails queued.
	const bool isEmpty = g_queue_is_empty(&thumbnailer->request_queue);
	if (isEmpty) {
		// Clear the idle process.
		// FIXME: Atomic compare and/or mutex? (assuming multi-threaded...)
		thumbnailer->idle_process = 0;

		// Restart the inactivity timeout.
		if (G_LIKELY(thumbnailer->timeout_id == 0)) {
			thumbnailer->timeout_id = g_timeout_add_seconds(SHUTDOWN_TIMEOUT_SECONDS,
				G_SOURCE_FUNC(rp_thumbnailer_timeout), thumbnailer);
		}
	}
	return !isEmpty;
}

/**
 * Create an RpThumbnailer object.
 * @param connection			[in] GDBusConnection
 * @param cache_dir			[in] Cache directory
 * @param pfn_rp_create_thumbnail2	[in] rp_create_thumbnail2() function pointer
 * @return RpThumbnailer object
 */
RpThumbnailer*
rp_thumbnailer_new(GDBusConnection *connection,
	const gchar *cache_dir,
	PFN_RP_CREATE_THUMBNAIL2 pfn_rp_create_thumbnail2)
{
	return g_object_new(RP_TYPE_THUMBNAILER,
		"connection", connection,
		"cache-dir", cache_dir,
		"pfn-rp-create-thumbnail2", pfn_rp_create_thumbnail2,
		NULL);
}

/**
 * Is the RpThumbnailer object exported?
 * @return True if it's exported; false if it isn't.
 */
gboolean
rp_thumbnailer_is_exported(RpThumbnailer *thumbnailer)
{
	g_return_val_if_fail(RP_IS_THUMBNAILER(thumbnailer), false);
	return thumbnailer->exported;
}
