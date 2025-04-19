/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * Ext2AttrView.cpp: Ext2 file system attribute viewer widget.             *
 *                                                                         *
 * Copyright (c) 2022-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://doc.qt.io/qt-5/dnd.html
#include "stdafx.h"
#include "Ext2AttrView.hpp"

// Ext2 flags (also used for Ext3, Ext4, and other Linux file systems)
#include "librpfile/xattr/ext2_flags.h"

// Ext2AttrData
#include "librpfile/xattr/Ext2AttrData.h"

// C++ STL classes
using std::array;
using std::string;

/** Ext2AttrViewPrivate **/

#include "ui_Ext2AttrView.h"
class Ext2AttrViewPrivate
{
public:
	Ext2AttrViewPrivate()
		: flags(0)
	{}

private:
	Q_DISABLE_COPY(Ext2AttrViewPrivate)

public:
	Ui::Ext2AttrView ui;
	int flags;

	// See Ext2AttrData.h
	array<QCheckBox*, EXT2_ATTR_CHECKBOX_MAX> checkBoxes;

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
void Ext2AttrViewPrivate::retranslateUi_nonDesigner(void)
{
	// tr: format string for Ext2 attribute checkbox labels ({:c} == lsattr character)
	const char *const s_lsattr_fmt = C_("Ext2AttrView", "{:c}: {:s}");

	string s_label;
	for (size_t i = 0; i < checkBoxes.size(); i++) {
		const Ext2AttrCheckboxInfo_t *const p = ext2AttrCheckboxInfo(static_cast<Ext2AttrCheckboxID>(i));

		// Prepend the lsattr character to the checkbox label.
		s_label = fmt::format(FRUN(s_lsattr_fmt), p->lsattr_chr,
			pgettext_expr("Ext2AttrView", p->label));

		checkBoxes[i]->setText(U82Q(s_label));
		checkBoxes[i]->setToolTip(qpgettext_expr("Ext2AttrView", p->tooltip));
	}
}

/**
 * Update the flags string display.
 * This uses the same format as e2fsprogs lsattr.
 */
void Ext2AttrViewPrivate::updateFlagsString(void)
{
	QString str = QLatin1String("----------------------");
	assert(str.size() == 22);

	// NOTE: This struct uses bit numbers, not masks.
	struct flags_name {
		uint8_t bit;
		char chr;
	};
	static const array<flags_name, 22> flags_array = {{
		{  0, 's' }, {  1, 'u' }, {  3, 'S' }, { 16, 'D' },
		{  4, 'i' }, {  5, 'a' }, {  6, 'd' }, {  7, 'A' },
		{  2, 'c' }, { 11, 'E' }, { 14, 'j' }, { 12, 'I' },
		{ 15, 't' }, { 17, 'T' }, { 19, 'e' }, { 23, 'C' },
		{ 25, 'x' }, { 30, 'F' }, { 28, 'N' }, { 29, 'P' },
		{ 20, 'V' }, { 10, 'm' }
	}};
	static_assert(flags_array.size() == EXT2_ATTR_CHECKBOX_MAX, "flags_array[] and checkBoxes[] are out of sync!");

	// NOTE: Need to use `unsigned int` because `size_t` results in an ambiguous overload error.
	for (unsigned int i = 0; i < static_cast<unsigned int>(flags_array.size()); i++) {
		if (flags & (1U << flags_array[i].bit)) {
			str[i] = QLatin1Char(flags_array[i].chr);
		}
	}

	ui.lblLsAttr->setText(str);
}

/**
 * Update the flags checkboxes.
 */
void Ext2AttrViewPrivate::updateFlagsCheckboxes(void)
{
	// FIXME: checkBoxes.size() can't be used here because it's apparently not constexpr...
	static_assert(ARRAY_SIZE(checkBoxes) == EXT2_ATTR_CHECKBOX_MAX,
		"checkBoxes and EXT2_ATTR_CHECKBOX_MAX are out of sync!");

	// Flag order, relative to checkboxes
	// NOTE: Uses bit indexes.
	static constexpr array<uint8_t, 22> flag_order = {{
		 5,  7,  2, 23,  6, 16, 19, 11,
		30,  4, 12, 14, 10, 28, 29,  0,
		 3, 15, 17,  1, 25, 20
	}};
	static_assert(flag_order.size() == EXT2_ATTR_CHECKBOX_MAX, "flag_order[] and checkBoxes[] are out of sync!");

	for (size_t i = 0; i < checkBoxes.size(); i++) {
		bool val = !!(flags & (1U << flag_order[i]));
		checkBoxes[i]->setChecked(val);
		checkBoxes[i]->setProperty("Ext2AttrView.value", val);
	}
}

/** Ext2AttrView **/

Ext2AttrView::Ext2AttrView(QWidget *parent)
	: super(parent)
	, d_ptr(new Ext2AttrViewPrivate())
{
	Q_D(Ext2AttrView);
	d->ui.setupUi(this);

	// Make sure we use the system-wide monospace font for
	// widgets that use monospace text.
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
	d->ui.lblLsAttr->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
#else /* QT_VERSION < QT_VERSION_CHECK(5, 2, 0) */
	QFont fntMonospace(QApplication::font());
	fntMonospace.setFamily(QLatin1String("Monospace"));
	fntMonospace.setStyleHint(QFont::TypeWriter);
	d->ui.lblLsAttr->setFont(fntMonospace);
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 2, 0) */

	// Add an event filter for the top-level window so we can
	// handle QEvent::StyleChange.
	installEventFilterInTopLevelWidget(this);

	// Create the checkboxes.
	static constexpr int col_count = 4;
	int col = 0, row = 0;
	for (size_t i = 0; i < d->checkBoxes.size(); i++) {
		const Ext2AttrCheckboxInfo_t *const p = ext2AttrCheckboxInfo(static_cast<Ext2AttrCheckboxID>(i));

		QCheckBox *const checkBox = new QCheckBox(this);
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
void Ext2AttrView::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(Ext2AttrView);
		d->ui.retranslateUi(this);
		d->retranslateUi_nonDesigner();
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/**
 * Event filter for top-level windows.
 * @param object QObject
 * @param event Event
 * @return True to filter the event; false to pass it through.
 */
bool Ext2AttrView::eventFilter(QObject *object, QEvent *event)
{
	Q_UNUSED(object);

	if (event->type() == QEvent::StyleChange) {
		// Update the monospace font.
		Q_D(Ext2AttrView);
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
		d->ui.lblLsAttr->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
#else /* QT_VERSION < QT_VERSION_CHECK(5, 2, 0) */
		QFont fntMonospace(QApplication::font());
		fntMonospace.setFamily(QLatin1String("Monospace"));
		fntMonospace.setStyleHint(QFont::TypeWriter);
		d->ui.lblLsAttr->setFont(fntMonospace);
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 2, 0) */
	}

	// Allow the event to propagate.
	return false;
}

/**
 * Get the current Ext2 attributes.
 * @return Ext2 attributes
 */
int Ext2AttrView::flags(void) const
{
	Q_D(const Ext2AttrView);
	return d->flags;
}

/**
 * Set the current Ext2 attributes.
 * @param flags Ext2 attributes
 */
void Ext2AttrView::setFlags(int flags)
{
	Q_D(Ext2AttrView);
	if (d->flags != flags) {
		d->flags = flags;
		d->updateFlagsDisplay();
	}
}

/**
 * Clear the current Ext2 attributes.
 */
void Ext2AttrView::clearFlags(void)
{
	Q_D(Ext2AttrView);
	if (d->flags != 0) {
		d->flags = 0;
		d->updateFlagsDisplay();
	}
}

/** Widget slots **/

/**
 * Disable user modifications of checkboxes.
 */
void Ext2AttrView::checkBox_clicked_slot(bool checked)
{
	QAbstractButton *const sender = qobject_cast<QAbstractButton*>(QObject::sender());
	if (!sender)
		return;

	// Get the saved Ext2AttrView value.
	const bool value = sender->property("Ext2AttrView.value").toBool();
	if (checked != value) {
		// Toggle this box.
		sender->setChecked(value);
	}
}
