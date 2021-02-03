/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * DragImageTreeView.hpp: Drag & Drop QTreeView subclass.                  *
 *                                                                         *
 * Copyright (c) 2019-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://doc.qt.io/qt-5/dnd.html
// - https://wiki.qt.io/QList_Drag_and_Drop_Example

#ifndef __ROMPROPERTIES_KDE_DRAGIMAGETREEVIEW_HPP__
#define __ROMPROPERTIES_KDE_DRAGIMAGETREEVIEW_HPP__

#include <QTreeView>

class DragImageTreeView : public QTreeView
{
	Q_OBJECT

	public:
		explicit DragImageTreeView(QWidget *parent = nullptr)
			: super(parent) { }

		// Role for an rp_image*.
		static const int RpImageRole = Qt::UserRole + 0x4049;

	private:
		typedef QTreeView super;
		Q_DISABLE_COPY(DragImageTreeView)

	protected:
		/** Overridden QWidget functions **/
		void startDrag(Qt::DropActions supportedActions) override;
};

#endif /* __ROMPROPERTIES_KDE_DRAGIMAGETREEVIEW_HPP__ */
