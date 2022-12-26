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
	bool val;
#define UPDATE_CHECKBOX(flag, obj) \
	val = !!(flags & (flag)); \
	ui.obj->setChecked(val); \
	ui.obj->setProperty("LinuxAttrView.value", val)

	UPDATE_CHECKBOX(FS_APPEND_FL, chkAppendOnly);
	UPDATE_CHECKBOX(FS_NOATIME_FL, chkNoATime);
	UPDATE_CHECKBOX(FS_COMPR_FL, chkCompressed);
	UPDATE_CHECKBOX(FS_NOCOW_FL, chkNoCOW);

	UPDATE_CHECKBOX(FS_NODUMP_FL, chkNoDump);
	UPDATE_CHECKBOX(FS_DIRSYNC_FL, chkDirSync);
	UPDATE_CHECKBOX(FS_EXTENT_FL, chkExtents);
	UPDATE_CHECKBOX(FS_ENCRYPT_FL, chkEncrypted);

	UPDATE_CHECKBOX(FS_CASEFOLD_FL, chkCasefold);
	UPDATE_CHECKBOX(FS_IMMUTABLE_FL, chkImmutable);
	UPDATE_CHECKBOX(FS_INDEX_FL, chkIndexed);
	UPDATE_CHECKBOX(FS_JOURNAL_DATA_FL, chkJournalled);

	UPDATE_CHECKBOX(FS_NOCOMP_FL, chkNoCompress);
	UPDATE_CHECKBOX(FS_INLINE_DATA_FL, chkInline);
	UPDATE_CHECKBOX(FS_PROJINHERIT_FL, chkProject);
	UPDATE_CHECKBOX(FS_SECRM_FL, chkSecureDelete);

	UPDATE_CHECKBOX(FS_SYNC_FL, chkFileSync);
	UPDATE_CHECKBOX(FS_NOTAIL_FL, chkNoTailMerge);
	UPDATE_CHECKBOX(FS_TOPDIR_FL, chkTopDir);
	UPDATE_CHECKBOX(FS_UNRM_FL, chkUndelete);

	UPDATE_CHECKBOX(FS_DAX_FL, chkDAX);
	UPDATE_CHECKBOX(FS_VERITY_FL, chkVerity);
}

/** LinuxAttrView **/

LinuxAttrView::LinuxAttrView(QWidget *parent)
	: super(parent)
	, d_ptr(new LinuxAttrViewPrivate())
{
	Q_D(LinuxAttrView);
	d->ui.setupUi(this);

	// Connect checkbox signals.
#define CONNECT_CHECKBOX_SIGNAL(obj) \
	connect(d->ui.obj, SIGNAL(clicked(bool)), \
		this, SLOT(checkBox_clicked_slot(bool)))

	CONNECT_CHECKBOX_SIGNAL(chkAppendOnly);
	CONNECT_CHECKBOX_SIGNAL(chkNoATime);
	CONNECT_CHECKBOX_SIGNAL(chkCompressed);
	CONNECT_CHECKBOX_SIGNAL(chkNoCOW);

	CONNECT_CHECKBOX_SIGNAL(chkNoDump);
	CONNECT_CHECKBOX_SIGNAL(chkDirSync);
	CONNECT_CHECKBOX_SIGNAL(chkExtents);
	CONNECT_CHECKBOX_SIGNAL(chkEncrypted);

	CONNECT_CHECKBOX_SIGNAL(chkCasefold);
	CONNECT_CHECKBOX_SIGNAL(chkImmutable);
	CONNECT_CHECKBOX_SIGNAL(chkIndexed);
	CONNECT_CHECKBOX_SIGNAL(chkJournalled);

	CONNECT_CHECKBOX_SIGNAL(chkNoCompress);
	CONNECT_CHECKBOX_SIGNAL(chkInline);
	CONNECT_CHECKBOX_SIGNAL(chkProject);
	CONNECT_CHECKBOX_SIGNAL(chkSecureDelete);

	CONNECT_CHECKBOX_SIGNAL(chkDAX);
	CONNECT_CHECKBOX_SIGNAL(chkVerity);
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

/** Widget slots **/

/**
 * Disable user modifications of checkboxes.
 */
void LinuxAttrView::checkBox_clicked_slot(bool checked)
{
	QAbstractButton *sender = qobject_cast<QAbstractButton*>(QObject::sender());
	if (!sender)
		return;

	// Get the saved LinuxAttrView value.
	const bool value = sender->property("LinuxAttrView.value").toBool();
	if (checked != value) {
		// Toggle this box.
		sender->setChecked(value);
	}
}
