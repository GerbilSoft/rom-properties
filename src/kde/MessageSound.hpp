/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * MessageSound.hpp: Message sound effects class.                          *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <QMessageBox>

namespace MessageSound {

/**
 * Play a message sound effect.
 * @param notificationType Notification type.
 * @param message Message for logging.
 * @param parent Parent window.
 */
void play(QMessageBox::Icon notificationType, const QString &message = QString(), QWidget *parent = nullptr);

} //namespace MessageSound
