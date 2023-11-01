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

class AchievementsItemDelegatePrivate;
class AchievementsItemDelegate : public QStyledItemDelegate
{
Q_OBJECT
typedef QStyledItemDelegate super;

public:
	explicit AchievementsItemDelegate(QObject *parent);
	~AchievementsItemDelegate() override;

protected:
	AchievementsItemDelegatePrivate *const d_ptr;
	Q_DECLARE_PRIVATE(AchievementsItemDelegate)
private:
	Q_DISABLE_COPY(AchievementsItemDelegate)

public:
	void paint(QPainter *painter, const QStyleOptionViewItem &option,
		   const QModelIndex &index) const final;
	QSize sizeHint(const QStyleOptionViewItem &option,
		       const QModelIndex &index) const final;
};
