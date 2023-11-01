/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * AchievementsTab.cpp: Achievements tab for rp-config.                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchievementsTab.hpp"
#include "AchievementsItemDelegate.hpp"
#include "../AchSpriteSheet.hpp"

// librpbase
#include "librpbase/Achievements.hpp"
using LibRpBase::Achievements;

#include "ui_AchievementsTab.h"
class AchievementsTabPrivate
{
public:
	explicit AchievementsTabPrivate() = default;

private:
	Q_DISABLE_COPY(AchievementsTabPrivate)

public:
	Ui::AchievementsTab ui;
};

/** AchievementsTab **/

AchievementsTab::AchievementsTab(QWidget *parent)
	: super(parent)
	, d_ptr(new AchievementsTabPrivate())
{
	Q_D(AchievementsTab);
	d->ui.setupUi(this);

	// Initialize the treeWidget's item delegate.
	d->ui.treeWidget->setItemDelegate(new AchievementsItemDelegate(this));

	// Load the achievements.
	reset();
}

AchievementsTab::~AchievementsTab()
{
	delete d_ptr;
}

/**
 * Widget state has changed.
 * @param event State change event.
 */
void AchievementsTab::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(AchievementsTab);
		d->ui.retranslateUi(this);
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/**
 * Reset the configuration.
 */
void AchievementsTab::reset(void)
{
	// Load the achievements.
	Q_D(AchievementsTab);
	QTreeWidget *const treeWidget = d->ui.treeWidget;
	treeWidget->clear();

	// TODO: Ideal icon size?
	// Using 32x32 for now.
	static const int iconSize = 32;
	const AchSpriteSheet achSpriteSheet(iconSize);
	treeWidget->setIconSize(QSize(iconSize, iconSize));

	const Achievements *const pAch = Achievements::instance();
	for (int i = 0; i < (int)Achievements::ID::Max; i++) {
		// Is the achievement unlocked?
		const Achievements::ID id = (Achievements::ID)i;
		const time_t timestamp = pAch->isUnlocked(id);
		const bool unlocked = (timestamp != -1);

		// Get the icon.
		QPixmap icon = achSpriteSheet.getIcon(id, !unlocked);

		// Get the name and description.
		QString s_ach = U82Q(pAch->getName(id)) + QChar(L'\n');
		// TODO: Locked description?
		s_ach += U82Q(pAch->getDescUnlocked(id));

		// Add the list item.
		QTreeWidgetItem *const treeWidgetItem = new QTreeWidgetItem(treeWidget);
		treeWidgetItem->setIcon(0, QIcon(icon));
		treeWidgetItem->setData(1, Qt::DisplayRole, s_ach);
		treeWidgetItem->setData(1, Qt::UserRole, unlocked);
		if (unlocked) {
			treeWidgetItem->setData(2, Qt::DisplayRole, QDateTime::fromMSecsSinceEpoch(timestamp * 1000));
		}
	}

	// Set column stretch modes.
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QHeaderView *const pHeader = treeWidget->header();
	pHeader->setStretchLastSection(false);
	pHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	pHeader->setSectionResizeMode(1, QHeaderView::Stretch);
	pHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);
#else /* QT_VERSION <= QT_VERSION_CHECK(5,0,0) */
	// Qt 4 doesn't have QHeaderView::setSectionResizeMode().
	// We'll run a manual resize on each column initially.
	for (int i = 0; i < 3; i++) {
		treeWidget->resizeColumnToContents(i);
	}
#endif
}
