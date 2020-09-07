/***************************************************************************
 * RVT-H Tool (qrvthtool)                                                  *
 * MessageSound.hpp: Message sound effects abstraction class.              *
 *                                                                         *
 * Copyright (c) 2018-2019 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_MESSAGESOUND_HPP__
#define __ROMPROPERTIES_KDE_MESSAGESOUND_HPP__

#include <QtWidgets/QMessageBox>

class MessageSound
{
	private:
		// MessageSound is a private class.
		MessageSound();
		~MessageSound();
		Q_DISABLE_COPY(MessageSound);

	public:
		/**
		 * Play a message sound effect.
		 * @param notificationType Notification type.
		 * @param message Message for logging. (not supported on all systems)
		 * @param parent Parent window. (not supported on all systems)
		 */
		static void play(QMessageBox::Icon notificationType, const QString &message = QString(), QWidget *parent = nullptr);
};

#endif /* __ROMPROPERTIES_KDE_MESSAGESOUND_HPP__ */
