/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * MessageSound.cpp: Message sound effects class.                          *
 *                                                                         *
 * Copyright (c) 2018-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MessageSound.hpp"

#include <QtCore/QPluginLoader>
#include <QtCore/QVariant>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#  include <kmessageboxnotifyinterface.h>
#else
#  include <knotification.h>
#endif

#include "RpQtNS.hpp"

namespace MessageSound {

/**
 * Play a message sound effect.
 * @param notificationType Notification type.
 * @param message Message for logging.
 * @param parent Parent window.
 */
void play(QMessageBox::Icon notificationType, const QString &message, QWidget *parent)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QPluginLoader lib(QStringLiteral(RP_KDE_LOWER "/FrameworkIntegrationPlugin"));
	QObject *const rootObj = lib.instance();
	if (rootObj) {
		KMessageBoxNotifyInterface *const iface = rootObj->property(KMESSAGEBOXNOTIFY_PROPERTY)
			.value<KMessageBoxNotifyInterface*>();
		if (iface) {
			iface->sendNotification(notificationType, message, parent);
		}
	}
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
	// FIXME: KNotification::event() doesn't seem to work.
	// This might not be too important nowadays, since KDE4 is ancient...
	QString messageType;
	switch (notificationType) {
		default:
		case QMessageBox::Information:
			messageType = QLatin1String("messageInformation");
			break;
		case QMessageBox::Warning:
			messageType = QLatin1String("messageWarning");
			break;
		case QMessageBox::Question:
			messageType = QLatin1String("messageQuestion");
			break;
		case QMessageBox::Critical:
			messageType = QLatin1String("messageCritical");
			break;
	}
	KNotification::event(messageType, message, QPixmap(), parent);
#endif
}

} //namespace MessageSound
