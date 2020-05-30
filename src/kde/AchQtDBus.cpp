/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * AchQtDBus.cpp: QtDBus notifications for achievements.                   *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchQtDBus.hpp"
using LibRpBase::Achievements;

// QtDBus
#include "notificationsinterface.h"

class AchQtDBusPrivate
{
	public:
		AchQtDBusPrivate();
		~AchQtDBusPrivate();

	private:
		RP_DISABLE_COPY(AchQtDBusPrivate)

	public:
		// Static AchQtDBus instance.
		// TODO: Q_GLOBAL_STATIC() equivalent, though we
		// may need special initialization if the compiler
		// doesn't support thread-safe statics.
		static AchQtDBus instance;
		bool hasRegistered;

	public:
		/**
		 * Notification function. (static)
		 * @param user_data User data. (this)
		 * @param name Achievement name.
		 * @param desc Achievement description.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int notifyFunc(intptr_t user_data, const char *name, const char *desc);

		/**
		 * Notification function. (non-static)
		 * @param name Achievement name.
		 * @param desc Achievement description.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int notifyFunc(const char *name, const char *desc);
};

/** AchQtDBusPrivate **/

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
AchQtDBus AchQtDBusPrivate::instance;

AchQtDBusPrivate::AchQtDBusPrivate()
	: hasRegistered(false)
{
	// NOTE: Cannot register here because the static Achievements
	// instance might not be fully initialized yet.
}

AchQtDBusPrivate::~AchQtDBusPrivate()
{
	if (hasRegistered) {
		Achievements *const pAch = Achievements::instance();
		pAch->clearNotifyFunction(notifyFunc, reinterpret_cast<intptr_t>(this));
	}
}

/**
 * Notification function. (static)
 * @param user_data User data. (this)
 * @param name Achievement name.
 * @param desc Achievement description.
 * @return 0 on success; negative POSIX error code on error.
 */
int AchQtDBusPrivate::notifyFunc(intptr_t user_data, const char *name, const char *desc)
{
	AchQtDBusPrivate *const pAchQtP = reinterpret_cast<AchQtDBusPrivate*>(user_data);
	return pAchQtP->notifyFunc(name, desc);
}

/**
 * Notification function. (non-static)
 * @param name Achievement name.
 * @param desc Achievement description.
 * @return 0 on success; negative POSIX error code on error.
 */
int AchQtDBusPrivate::notifyFunc(const char *name, const char *desc)
{
	// Connect to org.freedesktop.Notifications.
	org::freedesktop::Notifications iface(
		QLatin1String("org.freedesktop.Notifications"),
		QLatin1String("/org/freedesktop/Notifications"),
		QDBusConnection::sessionBus());
	if (!iface.isValid()) {
		// Invalid interface.
		return -EIO;
	}

	// Build the text.
	// TODO: Better formatting?
	QString text = QLatin1String("<u>");
	text += QString::fromUtf8(name);
	text += QLatin1String("</u>\n");
	text += QString::fromUtf8(desc);

	iface.Notify(
		QLatin1String("rom-properties"),	// app-name [s]
		0,					// replaces_id [u]
		QLatin1String("answer-correct"),	// app_icon [s]
		QLatin1String("Achievement Unlocked"),	// summary [s]
		text,					// body [s]
		QStringList(),				// actions [as]
		QVariantMap(),				// hints [a{sv}]
		5000);					// timeout (ms) [i]

	// NOTE: Not waiting for a response.
	return 0;
}

/** AchQtDBus **/

AchQtDBus::AchQtDBus()
	: d_ptr(new AchQtDBusPrivate())
{ }

AchQtDBus::~AchQtDBus()
{
	delete d_ptr;
}

/**
 * Get the AchQtDBus instance.
 *
 * This automatically initializes librpbase's Achievement
 * object and reloads the achievements data if it has been
 * modified.
 *
 * @return AchQtDBus instance.
 */
AchQtDBus *AchQtDBus::instance(void)
{
	AchQtDBus *const q = &AchQtDBusPrivate::instance;

	// NOTE: Cannot register in the private class constructor because
	// the Achievements instance might not be fully initialized yet.
	// Registering here instead.
	Achievements *const pAch = Achievements::instance();
	pAch->setNotifyFunction(AchQtDBusPrivate::notifyFunc, reinterpret_cast<intptr_t>(q->d_ptr));

	return q;
}
