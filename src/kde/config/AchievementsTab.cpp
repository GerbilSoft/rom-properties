/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * Achievements.cpp: Achievements tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchievementsTab.hpp"

// librpbase
#include "librpbase/Achievements.hpp"
using LibRpBase::Achievements;

#include "ui_AchievementsTab.h"
class AchievementsTabPrivate
{
	public:
		explicit AchievementsTabPrivate() { };

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
	treeWidget->setIconSize(QSize(iconSize, iconSize));

	char ach_filename[32];
	snprintf(ach_filename, sizeof(ach_filename), ":/ach/ach-%dx%d.png", iconSize, iconSize);
	QPixmap pxmAchSheet(QString::fromLatin1(ach_filename));
	if (pxmAchSheet.isNull())
		return;
	snprintf(ach_filename, sizeof(ach_filename), ":/ach/ach-gray-%dx%d.png", iconSize, iconSize);
	QPixmap pxmAchGraySheet(QString::fromLatin1(ach_filename));
	if (pxmAchGraySheet.isNull())
		return;

	// Make sure the bitmaps have the expected size.
	assert(pxmAchSheet.width() == (int)(iconSize * Achievements::ACH_SPRITE_SHEET_COLS));
	assert(pxmAchSheet.height() == (int)(iconSize * Achievements::ACH_SPRITE_SHEET_ROWS));
	assert(pxmAchGraySheet.width() == (int)(iconSize * Achievements::ACH_SPRITE_SHEET_COLS));
	assert(pxmAchGraySheet.height() == (int)(iconSize * Achievements::ACH_SPRITE_SHEET_ROWS));
	if (pxmAchSheet.width() != (int)(iconSize * Achievements::ACH_SPRITE_SHEET_COLS) ||
	    pxmAchSheet.height() != (int)(iconSize * Achievements::ACH_SPRITE_SHEET_ROWS) ||
	    pxmAchGraySheet.width() != (int)(iconSize * Achievements::ACH_SPRITE_SHEET_COLS) ||
	    pxmAchGraySheet.height() != (int)(iconSize * Achievements::ACH_SPRITE_SHEET_ROWS))
	{
		// Incorrect size. We can't use it.
		return;
	}

	// TODO: Custom renderer to show the description in a smaller font.
	const Achievements *pAch = Achievements::instance();

	for (int i = 0; i < (int)Achievements::ID::Max; i++) {
		// Is the achievement unlocked?
		const Achievements::ID id = (Achievements::ID)i;
		const time_t timestamp = pAch->isUnlocked(id);
		const bool unlocked = (timestamp != -1);

		// Determine row and column.
		const int col = (i % Achievements::ACH_SPRITE_SHEET_COLS);
		const int row = (i / Achievements::ACH_SPRITE_SHEET_COLS);

		// Extract the sub-icon.
		const QRect subRect(col*iconSize, row*iconSize, iconSize, iconSize);
		QPixmap pxmSubIcon = unlocked
			? pxmAchSheet.copy(subRect)
			: pxmAchGraySheet.copy(subRect);

		// Get the name and description.
		QString s_ach = U82Q(pAch->getName(id)) + QChar(L'\n');
		// TODO: Locked description?
		s_ach += U82Q(pAch->getDescUnlocked(id));

		// Add the list item.
		QTreeWidgetItem *const treeWidgetItem = new QTreeWidgetItem(treeWidget);
		treeWidgetItem->setIcon(0, QIcon(pxmSubIcon));
		treeWidgetItem->setData(1, Qt::DisplayRole, s_ach);
		treeWidgetItem->setData(1, Qt::UserRole, unlocked);
		if (unlocked) {
			treeWidgetItem->setData(2, Qt::DisplayRole, QDateTime::fromMSecsSinceEpoch(timestamp * 1000));
		}
	}

	// Set column stretch modes.
	QHeaderView *const pHeader = treeWidget->header();
	pHeader->setStretchLastSection(false);
	pHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	pHeader->setSectionResizeMode(1, QHeaderView::Stretch);
	pHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void AchievementsTab::loadDefaults(void)
{
	// Nothing to do here.
}

/**
 * Save the configuration.
 * @param pSettings QSettings object.
 */
void AchievementsTab::save(QSettings *pSettings)
{
	// Nothing to do here.
	Q_UNUSED(pSettings)
}
