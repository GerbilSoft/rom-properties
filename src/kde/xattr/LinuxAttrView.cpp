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

// LinuxAttrData (TODO: Rework into functions for libromdata.so.4)
#include "librpfile/xattr/LinuxAttrData.h"

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

		// See LinuxAttrData.h
		QCheckBox *checkBoxes[LINUX_ATTR_CHECKBOX_MAX];

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

/**
 * Retranslate parts of the UI that aren't present in the .ui file.
 */
void LinuxAttrViewPrivate::retranslateUi_nonDesigner(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(checkBoxes); i++) {
		checkBoxes[i]->setText(U82Q(dpgettext_expr(RP_I18N_DOMAIN, "LinuxAttrView", linuxAttrCheckboxInfo[i].label)));
		checkBoxes[i]->setToolTip(U82Q(dpgettext_expr(RP_I18N_DOMAIN, "LinuxAttrView", linuxAttrCheckboxInfo[i].tooltip)));
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
	static_assert(ARRAY_SIZE(checkBoxes) == ARRAY_SIZE(linuxAttrCheckboxInfo),
		"checkBoxes and checkboxInfo are out of sync!");

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
	static const int col_count = 4;
	int col = 0, row = 0;
	for (size_t i = 0; i < ARRAY_SIZE(d->checkBoxes); i++) {
		const LinuxAttrCheckboxInfo *const p = &linuxAttrCheckboxInfo[i];

		QCheckBox *const checkBox = new QCheckBox();
		checkBox->setObjectName(U82Q(p->name));
		d->ui.gridLayout->addWidget(checkBox, row, col);

		// Connect a signal to prevent modifications.
		connect(checkBox, SIGNAL(clicked(bool)),
			this, SLOT(checkBox_clicked_slot(bool)));

		d->checkBoxes[i] = checkBox;

		// Next checkbox
		col++;
		if (col == col_count) {
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
	QAbstractButton *const sender = qobject_cast<QAbstractButton*>(QObject::sender());
	if (!sender)
		return;

	// Get the saved LinuxAttrView value.
	const bool value = sender->property("LinuxAttrView.value").toBool();
	if (checked != value) {
		// Toggle this box.
		sender->setChecked(value);
	}
}
