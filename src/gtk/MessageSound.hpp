/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * MessageSound.hpp: Message sound effects class.                          *
 *                                                                         *
 * Copyright (c) 2018-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_MESSAGESOUND_HPP__
#define __ROMPROPERTIES_GTK_MESSAGESOUND_HPP__

#include <gtk/gtk.h>
#include "common.h"

class MessageSound
{
	private:
		// MessageSound is a private class.
		MessageSound();
		~MessageSound();
		RP_DISABLE_COPY(MessageSound);

	public:
		/**
		 * Play a message sound effect.
		 * @param notificationType Notification type.
		 * @param message Message for logging.
		 * @param parent Parent window.
		 */
		static void play(GtkMessageType notificationType, const char *message = nullptr, GtkWidget *parent = nullptr);
};

#endif /* __ROMPROPERTIES_GTK_MESSAGESOUND_HPP__ */
