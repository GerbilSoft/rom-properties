/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * LinuxAttrView.cpp: Linux file system attribute viewer widget.           *
 *                                                                         *
 * Copyright (c) 2022 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://doc.qt.io/qt-5/dnd.html
#include "stdafx.h"
#include "LinuxAttrView.hpp"

// EXT2 flags (also used for EXT3, EXT4, and other Linux file systems)
#include "ext2_flags.h"

/** LinuxAttrViewPrivate **/

#include "ui_LinuxAttrView.h"
class LinuxAttrViewPrivate
{
	public:
		LinuxAttrViewPrivate()
			: flags(0) { }

	private:
		Q_DISABLE_COPY(LinuxAttrViewPrivate)

	public:
		Ui::LinuxAttrView ui;
		int flags;

	public:
		/**
		 * Update the flags string display.
		 * This uses the same format as e2fsprogs lsattr.
		 */
		void updateFlagsString(void);

		/**
		 * Update the flags checkboxes.
		 */
		void updateFlagsCheckboxes(void);

		/**
		 * Update the flags display.
		 */
		inline void updateFlagsDisplay(void)
		{
			updateFlagsString();
			updateFlagsCheckboxes();
		}
};

/**
 * Update the flags string display.
 * This uses the same format as e2fsprogs lsattr.
 */
void LinuxAttrViewPrivate::updateFlagsString(void)
{
	QString str = QLatin1String("----------------------");
	assert(str.size() == 22);

	// NOTE: This struct uses bit numbers, not masks.
	struct flags_name {
		uint8_t bit;
		char chr;
	};
	static const struct flags_name flags_array[] = {
		{  0, 's' }, {  1, 'u' }, {  3, 'S' }, { 16, 'D' },
		{  4, 'i' }, {  5, 'a' }, {  6, 'd' }, {  7, 'A' },
		{  2, 'c' }, { 11, 'E' }, { 14, 'j' }, { 12, 'I' },
		{ 15, 't' }, { 17, 'T' }, { 19, 'e' }, { 23, 'C' },
		{ 25, 'x' }, { 30, 'F' }, { 28, 'N' }, { 29, 'P' },
		{ 20, 'V' }, { 10, 'm' }
	};

	for (int i = 0; i < ARRAY_SIZE_I(flags_array); i++) {
		if (flags & (1 << flags_array[i].bit)) {
			str[i] = QLatin1Char(flags_array[i].chr);
		}
	}

	ui.lblLsAttr->setText(str);
}

/**
 * Update the flags checkboxes.
 */
void LinuxAttrViewPrivate::updateFlagsCheckboxes(void)
{
	// Checkboxes
	ui.chkAppendOnly->setChecked(!!(flags & FS_APPEND_FL));
	ui.chkNoATime->setChecked(!!(flags & FS_NOATIME_FL));
	ui.chkCompressed->setChecked(!!(flags & FS_COMPR_FL));
	ui.chkNoCOW->setChecked(!!(flags & FS_NOCOW_FL));
	ui.chkNoDump->setChecked(!!(flags & FS_NODUMP_FL));
	ui.chkDirSync->setChecked(!!(flags & FS_DIRSYNC_FL));
	ui.chkExtents->setChecked(!!(flags & FS_EXTENT_FL));
	ui.chkEncrypted->setChecked(!!(flags & FS_ENCRYPT_FL));
	ui.chkCasefold->setChecked(!!(flags & FS_CASEFOLD_FL));
	ui.chkImmutable->setChecked(!!(flags & FS_IMMUTABLE_FL));
	ui.chkIndexed->setChecked(!!(flags & FS_INDEX_FL));
	ui.chkJournalled->setChecked(!!(flags & FS_JOURNAL_DATA_FL));
	ui.chkNoCompress->setChecked(!!(flags & FS_NOCOMP_FL));
	ui.chkInline->setChecked(!!(flags & FS_INLINE_DATA_FL));
	ui.chkProject->setChecked(!!(flags & FS_PROJINHERIT_FL));
	ui.chkSecureDelete->setChecked(!!(flags & FS_SECRM_FL));
	ui.chkFileSync->setChecked(!!(flags & FS_SYNC_FL));
	ui.chkNoTailMerge->setChecked(!!(flags & FS_NOTAIL_FL));
	ui.chkTopDir->setChecked(!!(flags & FS_TOPDIR_FL));
	ui.chkUndelete->setChecked(!!(flags & FS_UNRM_FL));
	ui.chkDAX->setChecked(!!(flags & FS_DAX_FL));
	ui.chkVerity->setChecked(!!(flags & FS_VERITY_FL));
}

/** LinuxAttrView **/

LinuxAttrView::LinuxAttrView(QWidget *parent)
	: super(parent)
	, d_ptr(new LinuxAttrViewPrivate())
{
	Q_D(LinuxAttrView);
	d->ui.setupUi(this);
}

/**
 * Get the current Linux attributes.
 * @return Linux attributes
 */
int LinuxAttrView::flags(void) const
{
	Q_D(const LinuxAttrView);
	return d->flags;
}

void LinuxAttrView::setFlags(int flags)
{
	Q_D(LinuxAttrView);
	if (d->flags != flags) {
		d->flags = flags;
		d->updateFlagsDisplay();
	}
}

/**
 * Clear the flags.
 */
void LinuxAttrView::clearFlags(void)
{
	Q_D(LinuxAttrView);
	if (d->flags != 0) {
		d->flags = 0;
		d->updateFlagsDisplay();
	}
}
