/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * LinuxAttrView.cpp: Linux file system attribute viewer widget.           *
 *                                                                         *
 * Copyright (c) 2022-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://doc.qt.io/qt-5/dnd.html
#include "stdafx.h"
#include "LinuxAttrView.hpp"

// EXT2 flags (also used for EXT3, EXT4, and other Linux file systems)
#include "librpfile/xattr/ext2_flags.h"

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
		enum CheckboxID : uint8_t {
			chkAppendOnly,
			chkNoATime,
			chkCompressed,
			chkNoCOW,

			chkNoDump,
			chkDirSync,
			chkExtents,
			chkEncrypted,

			chkCasefold,
			chkImmutable,
			chkIndexed,
			chkJournalled,

			chkNoCompress,
			chkInlineData,
			chkProject,
			chkSecureDelete,

			chkFileSync,
			chkNoTailMerge,
			chkTopDir,
			chkUndelete,

			chkDAX,
			chkVerity,

			CHECKBOX_MAX
		};

		struct CheckboxInfo {
			const char *name;	// object name
			const char *label;
			const char *tooltip;
		};

		static const CheckboxInfo checkboxInfo[CHECKBOX_MAX];

	public:
		Ui::LinuxAttrView ui;
		int flags;

		// See enum CheckboxID and checkboxInfo.
		QCheckBox *checkBoxes[CHECKBOX_MAX];

	public:
		/**
		 * Retranslate parts of the UI that aren't present in the .ui file.
		 */
		void retranslateUi_nonDesigner(void);

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

const LinuxAttrViewPrivate::CheckboxInfo LinuxAttrViewPrivate::checkboxInfo[LinuxAttrViewPrivate::CHECKBOX_MAX] = {
	{"chkAppendOnly", NOP_C_("LinuxAttrView", "a: append only"),
	 NOP_C_("LinuxAttrView", "File can only be opened in append mode for writing.")},

	{"chkNoATime", NOP_C_("LinuxAttrView", "A: no atime"),
	 NOP_C_("LinuxAttrView", "Access time record is not modified.")},

	{"chkCompressed", NOP_C_("LinuxAttrView", "c: compressed"),
	 NOP_C_("LinuxAttrView", "File is compressed.")},

	{"chkNoCOW", NOP_C_("LinuxAttrView", "C: no CoW"),
	 NOP_C_("LinuxAttrView", "Not subject to copy-on-write updates.")},

	{"chkNoDump", NOP_C_("LinuxAttrView", "d: no dump"),
	// tr: "dump" is the name of the executable, so it should not be localized.
	 NOP_C_("LinuxAttrView", "This file is not a candidate for dumping with the dump(8) program.")},

	{"chkDirSync", NOP_C_("LinuxAttrView", "D: dir sync"),
	 NOP_C_("LinuxAttrView", "Changes to this directory are written synchronously to the disk.")},

	{"chkExtents", NOP_C_("LinuxAttrView", "e: extents"),
	 NOP_C_("LinuxAttrView", "File is mapped on disk using extents.")},

	{"chkEncrypted", NOP_C_("LinuxAttrView", "E: encrypted"),
	 NOP_C_("LinuxAttrView", "File is encrypted.")},

	{"chkCasefold", NOP_C_("LinuxAttrView", "F: casefold"),
	 NOP_C_("LinuxAttrView", "Files stored in this directory use case-insensitive filenames.")},

	{"chkImmutable", NOP_C_("LinuxAttrView", "i: immutable"),
	 NOP_C_("LinuxAttrView", "File cannot be modified, deleted, or renamed.")},

	{"chkIndexed", NOP_C_("LinuxAttrView", "I: indexed"),
	 NOP_C_("LinuxAttrView", "Directory is indexed using hashed trees.")},

	{"chkJournalled", NOP_C_("LinuxAttrView", "j: journalled"),
	 NOP_C_("LinuxAttrView", "File data is written to the journal before writing to the file itself.")},

	{"chkNoCompress", NOP_C_("LinuxAttrView", "m: no compress"),
	 NOP_C_("LinuxAttrView", "File is excluded from compression.")},

	{"chkInlineData", NOP_C_("LinuxAttrView", "N: inline data"),
	 NOP_C_("LinuxAttrView", "File data is stored inline in the inode.")},

	{"chkProject", NOP_C_("LinuxAttrView", "P: project"),
	 NOP_C_("LinuxAttrView", "Directory will enforce a hierarchical structure for project IDs.")},

	{"chkSecureDelete", NOP_C_("LinuxAttrView", "s: secure del"),
	 NOP_C_("LinuxAttrView", "File's blocks will be zeroed when deleted.")},

	{"chkFileSync", NOP_C_("LinuxAttrView", "S: sync"),
	 NOP_C_("LinuxAttrView", "Changes to this file are written synchronously to the disk.")},

	{"chkNoTailMerge", NOP_C_("LinuxAttrView", "t: no tail merge"),
	 NOP_C_("LinuxAttrView", "If the file system supports tail merging, this file will not have a partial block fragment at the end of the file merged with other files.")},

	{"chkTopDir", NOP_C_("LinuxAttrView", "T: top dir"),
	 NOP_C_("LinuxAttrView", "Directory will be treated like a top-level directory by the ext3/ext4 Orlov block allocator.")},

	{"chkUndelete", NOP_C_("LinuxAttrView", "u: undelete"),
	 NOP_C_("LinuxAttrView", "File's contents will be saved when deleted, potentially allowing for undeletion. This is known to be broken.")},

	{"chkDAX", NOP_C_("LinuxAttrView", "x: DAX"),
	 NOP_C_("LinuxAttrView", "Direct access")},

	{"chkVerity", NOP_C_("LinuxAttrView", "V: fs-verity"),
	 NOP_C_("LinuxAttrView", "File has fs-verity enabled.")},
};

/**
 * Retranslate parts of the UI that aren't present in the .ui file.
 */
void LinuxAttrViewPrivate::retranslateUi_nonDesigner(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(checkBoxes); i++) {
		checkBoxes[i]->setText(U82Q(dpgettext_expr(RP_I18N_DOMAIN, "LinuxAttrView", checkboxInfo[i].label)));
		checkBoxes[i]->setToolTip(U82Q(dpgettext_expr(RP_I18N_DOMAIN, "LinuxAttrView", checkboxInfo[i].tooltip)));
	}
}

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
		if (flags & (1U << flags_array[i].bit)) {
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
	// Flag order, relative to checkboxes
	// NOTE: Uses bit indexes.
	static const uint8_t flag_order[] = {
		 5,  7,  2, 23,  6, 16, 19, 11,
		30,  4, 12, 14, 10, 28, 29,  0,
		 3, 15, 17,  1, 25, 20
	};

	for (size_t i = 0; i < ARRAY_SIZE(checkBoxes); i++) {
		bool val = !!(flags & (1U << flag_order[i]));
		checkBoxes[i]->setChecked(val);
		checkBoxes[i]->setProperty("LinuxAttrView.value", val);
	}
}

/** LinuxAttrView **/

LinuxAttrView::LinuxAttrView(QWidget *parent)
	: super(parent)
	, d_ptr(new LinuxAttrViewPrivate())
{
	Q_D(LinuxAttrView);
	d->ui.setupUi(this);

	// Create the checkboxes.
	static const int max_col = 4;
	int col = 0, row = 0;
	for (size_t i = 0; i < ARRAY_SIZE(d->checkBoxes); i++) {
		const LinuxAttrViewPrivate::CheckboxInfo *const p = &d->checkboxInfo[i];

		QCheckBox *const checkBox = new QCheckBox();
		checkBox->setObjectName(U82Q(p->name));
		d->ui.gridLayout->addWidget(checkBox, row, col);

		// Connect a signal to prevent modifications.
		connect(checkBox, SIGNAL(clicked(bool)),
			this, SLOT(checkBox_clicked_slot(bool)));

		d->checkBoxes[i] = checkBox;

		// Next checkbox
		col++;
		if (col == max_col) {
			col = 0;
			row++;
		}
	}

	// Retranslate the checkboxes.
	d->retranslateUi_nonDesigner();
}

/**
 * Widget state has changed.
 * @param event State change event.
 */
void LinuxAttrView::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(LinuxAttrView);
		d->ui.retranslateUi(this);
		d->retranslateUi_nonDesigner();
	}

	// Pass the event to the base class.
	super::changeEvent(event);
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

/**
 * Set the current Linux attributes.
 * @param flags Linux attributes
 */
void LinuxAttrView::setFlags(int flags)
{
	Q_D(LinuxAttrView);
	if (d->flags != flags) {
		d->flags = flags;
		d->updateFlagsDisplay();
	}
}

/**
 * Clear the current Linux attributes.
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
