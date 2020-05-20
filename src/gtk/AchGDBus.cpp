/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * AchGDBus.cpp: GDBus notifications for achievements.                     *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchGDBus.hpp"
using LibRpBase::Achievements;

// GDBus
#include <glib-object.h>
#include "Notifications.h"

class AchGDBusPrivate
{
	public:
		AchGDBusPrivate();
		~AchGDBusPrivate();

	private:
		RP_DISABLE_COPY(AchGDBusPrivate)

	public:
		// Static AchGDBus instance.
		// TODO: Q_GLOBAL_STATIC() equivalent, though we
		// may need special initialization if the compiler
		// doesn't support thread-safe statics.
		static AchGDBus instance;
		bool hasRegistered;

	public:
		/**
		 * Notification function. (static)
		 * @param user_data User data. (this)
		 * @param desc Achievement description.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int notifyFunc(intptr_t user_data, const char *desc);

		/**
		 * Notification function. (non-static)
		 * @param desc Achievement description.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int notifyFunc(const char *desc);
};

/** AchGDBusPrivate **/

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
AchGDBus AchGDBusPrivate::instance;

AchGDBusPrivate::AchGDBusPrivate()
	: hasRegistered(false)
{
	// NOTE: Cannot register here because the static Achievements
	// instance might not be fully initialized yet.
}

AchGDBusPrivate::~AchGDBusPrivate()
{
	if (hasRegistered) {
		Achievements *const pAch = Achievements::instance();
		pAch->clearNotifyFunction(notifyFunc, reinterpret_cast<intptr_t>(this));
	}
}

/**
 * Notification function. (static)
 * @param user_data User data. (this)
 * @param desc Achievement description.
 * @return 0 on success; negative POSIX error code on error.
 */
int AchGDBusPrivate::notifyFunc(intptr_t user_data, const char *desc)
{
	AchGDBusPrivate *const pAchGP = reinterpret_cast<AchGDBusPrivate*>(user_data);
	return pAchGP->notifyFunc(desc);
}

/**
 * Notification function. (non-static)
 * @param desc Achievement description.
 * @return 0 on success; negative POSIX error code on error.
 */
int AchGDBusPrivate::notifyFunc(const char *desc)
{
	// Connect to the service using gdbus-codegen's generated code.
	OrgFreedesktopNotifications *proxy = nullptr;
	GError *error = nullptr;

	proxy = org_freedesktop_notifications_proxy_new_for_bus_sync(
		G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_NONE,
		"org.freedesktop.Notifications",	// bus name
		"/org/freedesktop/Notifications",	// object path
		nullptr,				// GCancellable
		&error);				// GError
	if (!proxy) {
		// Unable to connect.
		g_error_free(error);
		return -EIO;
	}

	// actions: as
	static const gchar *const actions[] = { "", nullptr };

	// hints: a{sv}
	// NOTE: GDBus seems to take ownership of `hints`...
	GVariantBuilder *const b = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	GVariant *const hints = g_variant_builder_end(b);
	g_variant_builder_unref(b);

	org_freedesktop_notifications_call_notify(
		proxy,				// proxy
		"rom-properties",		// app-name [s]
		0,				// replaces_id [u]
		"answer-correct",		// app_icon [s]
		"Achievement Unlocked",		// summary [s]
		desc,				// body [s]
		actions,			// actions [as]
		hints,				// hints [a{sv}]
		5000,				// timeout (ms) [i]
		nullptr,			// GCancellable
		nullptr,			// GAsyncReadyCallback
		nullptr);			// user_data

	// NOTE: Not waiting for a response.
	g_object_unref(proxy);
	return 0;
}

/** AchGDBus **/

AchGDBus::AchGDBus()
	: d_ptr(new AchGDBusPrivate())
{ }

AchGDBus::~AchGDBus()
{
	delete d_ptr;
}

/**
 * Get the AchGDBus instance.
 *
 * This automatically initializes librpbase's Achievement
 * object and reloads the achievements data if it has been
 * modified.
 *
 * @return AchGDBus instance.
 */
AchGDBus *AchGDBus::instance(void)
{
	AchGDBus *const q = &AchGDBusPrivate::instance;

	// NOTE: Cannot register in the private class constructor because
	// the Achievements instance might not be fully initialized yet.
	// Registering here instead.
	AchGDBusPrivate *const d = q->d_ptr;
	if (!d->hasRegistered) {
		Achievements *const pAch = Achievements::instance();
		pAch->setNotifyFunction(AchGDBusPrivate::notifyFunc, reinterpret_cast<intptr_t>(d));
		d->hasRegistered = true;
	}

	return q;
}
