/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * DragImageTreeView.hpp: Drag & Drop QTreeView subclass.                  *
 *                                                                         *
 * Copyright (c) 2019-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://doc.qt.io/qt-5/dnd.html
// - https://wiki.qt.io/QList_Drag_and_Drop_Example

#pragma once

#include <QTreeView>

class DragImageTreeView : public QTreeView
{
	Q_OBJECT

public:
	explicit DragImageTreeView(QWidget *parent = nullptr)
		: super(parent)
	{}

private:
	typedef QTreeView super;
	Q_DISABLE_COPY(DragImageTreeView)

protected:
	/** Overridden QWidget functions **/
	void startDrag(Qt::DropActions supportedActions) override;
};
