/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * QTreeViewOpt.cpp: QTreeView with drawing optimizations.                 *
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

#include "QTreeViewOpt.hpp"

QTreeViewOpt::QTreeViewOpt(QWidget *parent)
	: super(parent)
{ }

/**
 * Data has changed in the item model.
 * @param topLeft	[in] Top-left item.
 * @param bottomRight	[in] Bottom-right item.
 * @param roles		[in] (Qt5) Roles that have changed.
 */
#if QT_VERSION >= 0x050000
void QTreeViewOpt::dataChanged(const QModelIndex &topLeft,
	const QModelIndex &bottomRight,
	const QVector<int> &roles)
#else /* QT_VERSION < 0x050000 */
void QTreeViewOpt::dataChanged(const QModelIndex &topLeft,
	const QModelIndex &bottomRight)
#endif
{
	bool propagateEvent = true;
	// TODO: Support for checking multiple items.
	if (topLeft == bottomRight) {
		// Single item. This might be an icon animation.
		// If it is, make sure the icon is onscreen.
		QRect itemRect = this->visualRect(topLeft);

		// Get the viewport rect.
		QRect viewportRect(QPoint(0, 0), this->viewport()->size());
		if (!viewportRect.intersects(itemRect)) {
			// Item is NOT visible.
			// Don't propagate the event.
			propagateEvent = false;
		}
	}

	if (propagateEvent) {
		// Propagate the dataChanged() event.
#if QT_VERSION >= 0x050000
		super::dataChanged(topLeft, bottomRight, roles);
#else /* QT_VERSION < 0x050000 */
		super::dataChanged(topLeft, bottomRight);
#endif
	}
}
