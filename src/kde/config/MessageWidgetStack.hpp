/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * MessageWidgetStack.hpp: Message widget stack.                           *
 *                                                                         *
 * Copyright (c) 2014-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_CONFIG_MESSAGEWIDGETSTACK__
#define __ROMPROPERTIES_KDE_CONFIG_MESSAGEWIDGETSTACK__

#include <QWidget>
#include "MessageWidget.hpp"

class MessageWidgetStackPrivate;
class MessageWidgetStack : public QWidget
{
	Q_OBJECT

	public:
		explicit MessageWidgetStack(QWidget *parent = 0);
		virtual ~MessageWidgetStack();

	private:
		typedef QWidget super;
		MessageWidgetStackPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(MessageWidgetStack)
		Q_DISABLE_COPY(MessageWidgetStack)

	public:
		/**
		 * Show a message.
		 * @param msg Message text. (supports Qt RichText formatting)
		 * @param icon Icon.
		 * @param timeout Timeout, in milliseconds. (0 for no timeout)
		 * @param closeOnDestroy Close the message when the specified QObject is destroyed.
		 */
		void showMessage(const QString &msg, MessageWidget::MsgIcon icon, int timeout = 0, QObject *closeOnDestroy = 0);

	protected slots:
		/**
		 * A MessageWidget has been dismissed.
		 * @param widget MessageWidget.
		 */
		void messageWidget_dismissed_slot(QWidget *widget);

		/**
		 * A MessageWidget has been destroyed.
		 * @param obj QObject that was destroyed.
		 */
		void messageWidget_destroyed_slot(QObject *obj);
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_MESSAGEWIDGETSTACK__ */
