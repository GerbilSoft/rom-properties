/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * AchievementsItemDelegate.cpp: Achievements item delegate for rp-config. *
 *                                                                         *
 * Copyright (c) 2013-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Qt includes.
#include <QStyledItemDelegate>

class AchievementsItemDelegate : public QStyledItemDelegate
{
Q_OBJECT
typedef QStyledItemDelegate super;

public:
	explicit AchievementsItemDelegate(QObject *parent);

private:
	Q_DISABLE_COPY(AchievementsItemDelegate)

private:
	// Font retrieval
	static QFont fontName(const QWidget *widget = nullptr);
	static QFont fontDesc(const QWidget *widget = nullptr);

public:
	void paint(QPainter *painter, const QStyleOptionViewItem &option,
		   const QModelIndex &index) const final;
	QSize sizeHint(const QStyleOptionViewItem &option,
		       const QModelIndex &index) const final;
};
