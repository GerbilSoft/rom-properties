/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * MessageSound.cpp: Message sound effects class.                          *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.gtk.h"
#include "MessageSound.hpp"

#include <gdk/gdkdisplay.h>
#if GTK_CHECK_VERSION(4,0,0)
#  include <gdk/wayland/gdkwayland.h>
#elif GTK_CHECK_VERSION(3,7,8)
#  include <gdk/gdkwayland.h>
#endif

#if defined(HAVE_GSOUND)
#  include <gsound.h>
#elif defined(HAVE_LIBCANBERRA_GTK)
#  include <canberra-gtk.h>
#else
#  error Neither GSound nor LibCanberra GTK are available
#endif

namespace MessageSound {

#ifdef HAVE_GSOUND
/**
 * GSound playback callback.
 * This callback destroys the GSound context.
 * @param source_object GSoundContext
 * @param res GAsyncResult
 * @param user_data User data
 */
static void _gsound_play_callback(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	RP_UNUSED(res);
	RP_UNUSED(user_data);
	g_object_unref(source_object);
}
#endif /* HAVE_GSOUND */

/**
 * Play a message sound effect.
 * @param notificationType Notification type.
 * @param message Message for logging.
 * @param parent Parent window.
 */
void play(GtkMessageType notificationType, const char *message, GtkWidget *parent)
{
	// Check if event sounds are enabled.
	GtkSettings *const settings = gtk_settings_get_default();
	gboolean bEnableEventSounds = false;
	g_object_get(settings, "gtk-enable-event-sounds", &bEnableEventSounds, nullptr);
	if (!bEnableEventSounds) {
		// Event sounds are disabled.
		return;
	}

	const char *event_id;
	switch (notificationType) {
		default:
		case GTK_MESSAGE_INFO:
		case GTK_MESSAGE_QUESTION:
		case GTK_MESSAGE_OTHER:
			event_id = "dialog-information";
			break;
		case GTK_MESSAGE_WARNING:
			event_id = "dialog-warning";
			break;
		case GTK_MESSAGE_ERROR:
			event_id = "dialog-error";
			break;
	}

	// NOTE: Description cannot be nullptr. Otherwise, the sound won't play.

#if defined(HAVE_GSOUND)
	GSoundContext *const ctx = gsound_context_new(nullptr, nullptr);

	GHashTable *attrs = g_hash_table_new_full(g_str_hash, g_str_equal, nullptr, g_free);

	// Sound properties.
	g_hash_table_insert(attrs, (void*)GSOUND_ATTR_EVENT_ID, (void*)g_strdup(event_id));
	g_hash_table_insert(attrs, (void*)GSOUND_ATTR_EVENT_DESCRIPTION,
		(void*)g_strdup(message ? message : ""));

	// Set some properties that would usually be set by libcanberra-gtk.
	const char *name = gtk_window_get_default_icon_name();
	if (name != nullptr) {
		g_hash_table_insert(attrs, (void*)GSOUND_ATTR_APPLICATION_ICON_NAME, (void*)g_strdup(name));
	}

	GdkDisplay *display;
	if (parent) {
		display = gtk_widget_get_display(parent);
	} else {
		display = gdk_display_get_default();
	}

	// Verify that this is an X11 display before attempting to set the attribute.
#if GTK_CHECK_VERSION(3,7,8)
	if (GDK_IS_WAYLAND_DISPLAY(display)) {
		// Wayland. Cannot be X11.
		name = nullptr;
	} else
#endif /* GTK_CHECK_VERSION(3,7,8) */
	{
		// Assuming X11.
		name = gdk_display_get_name(display);
	}

	if (name != nullptr) {
		g_hash_table_insert(attrs, (void*)GSOUND_ATTR_WINDOW_X11_DISPLAY, (void*)g_strdup(name));
	}

	// NOTE: g_object_unref() on ctx immediately after calling
	// gsound_context_play_simplev() results in no sound.
	// The context will be deleted in the callback.
	gsound_context_play_fullv(ctx, attrs, nullptr, _gsound_play_callback, nullptr);
	g_hash_table_destroy(attrs);
#elif defined(HAVE_LIBCANBERRA_GTK)
	if (parent) {
		ca_gtk_play_for_widget(gtk_widget_get_toplevel_widget(parent), 0,
			CA_PROP_EVENT_ID, event_id,
			CA_PROP_EVENT_DESCRIPTION, (message ? message : ""),
			nullptr);
	} else {
		ca_context *const ctx = ca_gtk_context_get();
		if (!ctx)
			return;

		ca_context_play(ctx, 0,
			CA_PROP_EVENT_ID, event_id,
			CA_PROP_EVENT_DESCRIPTION, (message ? message : ""),
			nullptr);
	}
#endif
}

} //namespace MessageSound
