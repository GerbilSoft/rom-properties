/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * QTreeViewOpt.hpp: QTreeView with drawing optimizations.                 *
 * Specifically, don't update rows that are offscreen.                     *
 *                                                                         *
 * Copyright (c) 2013-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_KDE_CONFIG_QTREEVIEWOPT_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_QTREEVIEWOPT_HPP__

// Qt includes and classes.
#include <QTreeView>

class QTreeViewOpt : public QTreeView
{
	Q_OBJECT

	public:
		explicit QTreeViewOpt(QWidget *parent = 0);

	private:
		typedef QTreeView super;
		Q_DISABLE_COPY(QTreeViewOpt);

	public:
#if QT_VERSION >= 0x050000
		virtual void dataChanged(const QModelIndex &topLeft,
			const QModelIndex &bottomRight,
			const QVector<int> &roles = QVector<int>()) override final;
#else /* QT_VERSION < 0x050000 */
		virtual void dataChanged(const QModelIndex &topLeft,
			const QModelIndex &bottomRight) override final;
#endif
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_QTREEVIEWOPT_HPP__ */
