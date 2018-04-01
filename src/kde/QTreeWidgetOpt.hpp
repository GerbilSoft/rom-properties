/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * QTreeWidgetOpt.hpp: QTreeWidget with drawing optimizations.             *
 * Specifically, don't update rows that are offscreen.                     *
 *                                                                         *
 * Copyright (c) 2013-2018 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_KDE_QTREEWIDGETOPT_HPP__
#define __ROMPROPERTIES_KDE_QTREEWIDGETOPT_HPP__

// Qt includes and classes.
#include <QTreeWidget>

class QTreeWidgetOpt : public QTreeWidget
{
	Q_OBJECT

	public:
		explicit QTreeWidgetOpt(QWidget *parent = 0);

	private:
		typedef QTreeWidget super;
		Q_DISABLE_COPY(QTreeWidgetOpt);

	public:
#if QT_VERSION >= 0x050000
		void dataChanged(const QModelIndex &topLeft,
			const QModelIndex &bottomRight,
			const QVector<int> &roles = QVector<int>()) final;
#else /* QT_VERSION < 0x050000 */
		void dataChanged(const QModelIndex &topLeft,
			const QModelIndex &bottomRight) final;
#endif
};

#endif /* __ROMPROPERTIES_KDE_QTREEWIDGETOPT_HPP__ */
