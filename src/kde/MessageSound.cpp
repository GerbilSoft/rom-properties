/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * MessageSound.hpp: Message sound effects abstraction class.              *
 *                                                                         *
 * Copyright (c) 2018-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MessageSound.hpp"

#include <QtCore/QPluginLoader>
#include <QtCore/QVariant>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#  include <kmessageboxnotifyinterface.h>
#else
// FIXME: KDE4 support.
#endif

/**
 * Play a message sound effect.
 * @param notificationType Notification type.
 * @param message Message for logging. (not supported on all systems)
 * @param parent Parent window. (not supported on all systems)
 */
void MessageSound::play(QMessageBox::Icon notificationType, const QString &message, QWidget *parent)
{
	QPluginLoader lib(QStringLiteral("kf5/FrameworkIntegrationPlugin"));
	QObject *const rootObj = lib.instance();
	if (rootObj) {
		KMessageBoxNotifyInterface *iface = rootObj->property(KMESSAGEBOXNOTIFY_PROPERTY)
			.value<KMessageBoxNotifyInterface*>();
		if (iface) {
			iface->sendNotification(notificationType, message, parent);
		}
	}
}
