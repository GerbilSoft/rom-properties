/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * MessageSound.hpp: Message sound effects class.                          *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <gtk/gtk.h>
#include "common.h"

namespace MessageSound {

/**
 * Play a message sound effect.
 * @param notificationType Notification type.
 * @param message Message for logging.
 * @param parent Parent window.
 */
void play(GtkMessageType notificationType, const char *message = nullptr, GtkWidget *parent = nullptr);

} //namespace MessageSound
