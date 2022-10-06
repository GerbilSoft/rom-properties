/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * AchievementsItemDelegate.cpp: Achievements item delegate for rp-config. *
 *                                                                         *
 * Copyright (c) 2013-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_CONFIG_ACHIEVEMENTSITEMDELEGATE_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_ACHIEVEMENTSITEMDELEGATE_HPP__

// Qt includes.
#include <QStyledItemDelegate>

class AchievementsItemDelegatePrivate;
class AchievementsItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
	typedef QStyledItemDelegate super;

	public:
		explicit AchievementsItemDelegate(QObject *parent);
		~AchievementsItemDelegate() final;

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

#endif /* __ROMPROPERTIES_KDE_CONFIG_ACHIEVEMENTSITEMDELEGATE_HPP__ */
