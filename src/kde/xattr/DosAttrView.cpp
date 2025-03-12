/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * DosAttrView.cpp: MS-DOS file system attribute viewer widget.            *
 *                                                                         *
 * Copyright (c) 2022-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://doc.qt.io/qt-5/dnd.html
#include "stdafx.h"
#include "DosAttrView.hpp"

// MS-DOS and Windows attributes
// NOTE: Does not depend on the Windows SDK.
#include "librpfile/xattr/dos_attrs.h"

/** DosAttrViewPrivate **/

#include "ui_DosAttrView.h"
class DosAttrViewPrivate
{
public:
	DosAttrViewPrivate()
		: attrs(0)
		, validAttrs(0)
	{}

private:
	Q_DISABLE_COPY(DosAttrViewPrivate)

public:
	Ui::DosAttrView ui;
	unsigned int attrs;
	unsigned int validAttrs;

public:
	/**
	 * Update a single checkbox.
	 * @param attr Attribute to check
	 * @param checkBox Checkbox widget
	 */
	inline void updateCheckbox(unsigned int attr, QCheckBox *checkBox);

	/**
	 * Update the attributes display.
	 */
	void updateAttrsDisplay(void);

public:
	/**
	 * Connect a checkbox signal.
	 * @param checkBox Checkbox
	 * @param receiver Receiver
	 */
	static inline void connectCheckboxSignal(QCheckBox *checkBox, DosAttrView *receiver);
};

/**
 * Update a single checkbox.
 * @param attr Attribute to check
 * @param checkBox Checkbox widget
 */
inline void DosAttrViewPrivate::updateCheckbox(unsigned int attr, QCheckBox *checkBox)
{
	const bool val = !!(attrs & attr);
	const bool enable = !!(validAttrs & attr);
	checkBox->setChecked(val);
	checkBox->setEnabled(enable);
	checkBox->setProperty("DosAttrView.value", val);
}

/**
 * Update the attributes display.
 */
void DosAttrViewPrivate::updateAttrsDisplay(void)
{
	updateCheckbox(FILE_ATTRIBUTE_READONLY, ui.chkReadOnly);
	updateCheckbox(FILE_ATTRIBUTE_HIDDEN, ui.chkHidden);
	updateCheckbox(FILE_ATTRIBUTE_ARCHIVE, ui.chkArchive);
	updateCheckbox(FILE_ATTRIBUTE_SYSTEM, ui.chkSystem);

	updateCheckbox(FILE_ATTRIBUTE_COMPRESSED, ui.chkCompressed);
	updateCheckbox(FILE_ATTRIBUTE_ENCRYPTED, ui.chkEncrypted);
}

/**
 * Connect a checkbox signal.
 * @param checkBox Checkbox
 * @param receiver Receiver
 */
inline void DosAttrViewPrivate::connectCheckboxSignal(QCheckBox *checkBox, DosAttrView *receiver)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	QObject::connect(checkBox, &QCheckBox::clicked,
	                 receiver, &DosAttrView::checkBox_clicked_slot);
#else /* QT_VERSION < QT_VERSION_CHECK(5, 0, 0) */
	QObject::connect(checkBox, SIGNAL(clicked(bool)),
	                 receiver, SLOT(checkBox_clicked_slot(bool)));
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) */
}

/** DosAttrView **/

DosAttrView::DosAttrView(QWidget *parent)
	: super(parent)
	, d_ptr(new DosAttrViewPrivate())
{
	Q_D(DosAttrView);
	d->ui.setupUi(this);

	// Connect checkbox signals.
	d->connectCheckboxSignal(d->ui.chkReadOnly, this);
	d->connectCheckboxSignal(d->ui.chkHidden, this);
	d->connectCheckboxSignal(d->ui.chkArchive, this);
	d->connectCheckboxSignal(d->ui.chkSystem, this);

	d->connectCheckboxSignal(d->ui.chkCompressed, this);
	d->connectCheckboxSignal(d->ui.chkEncrypted, this);
}

/**
 * Get the current MS-DOS attributes.
 * @return MS-DOS attributes
 */
unsigned int DosAttrView::attrs(void) const
{
	Q_D(const DosAttrView);
	return d->attrs;
}

/**
 * Set the current MS-DOS attributes.
 * @param attrs MS-DOS attributes
 */
void DosAttrView::setAttrs(unsigned int attrs)
{
	Q_D(DosAttrView);
	if (d->attrs != attrs) {
		d->attrs = attrs;
		d->updateAttrsDisplay();
	}
}

/**
 * Clear the current MS-DOS attributes.
 */
void DosAttrView::clearAttrs(void)
{
	Q_D(DosAttrView);
	if (d->attrs != 0) {
		d->attrs = 0;
		d->updateAttrsDisplay();
	}
}

/**
 * Get the valid MS-DOS attributes.
 * @return Valid MS-DOS attributes
 */
unsigned int DosAttrView::validAttrs(void) const
{
	Q_D(const DosAttrView);
	return d->validAttrs;
}

/**
 * Set the valid MS-DOS attributes.
 * @param validAttrs Valid MS-DOS attributes
 */
void DosAttrView::setValidAttrs(unsigned int validAttrs)
{
	Q_D(DosAttrView);
	if (d->validAttrs != validAttrs) {
		d->validAttrs = validAttrs;
		d->updateAttrsDisplay();
	}
}

/**
 * Clear the valid MS-DOS attributes.
 */
void DosAttrView::clearValidAttrs(void)
{
	Q_D(DosAttrView);
	if (d->validAttrs != 0) {
		d->validAttrs = 0;
		d->updateAttrsDisplay();
	}
}

/**
 * Set the current *and* valid MS-DOS attributes at the same time.
 * @param attrs MS-DOS attributes
 * @param validAttrs Valid MS-DOS attributes
 */
void DosAttrView::setCurrentAndValidAttrs(unsigned int attrs, unsigned int validAttrs)
{
	Q_D(DosAttrView);
	if (d->attrs != attrs || d->validAttrs != validAttrs) {
		d->attrs = attrs;
		d->validAttrs = validAttrs;
		d->updateAttrsDisplay();
	}
}

/** Widget slots **/

/**
 * Disable user modifications of checkboxes.
 */
void DosAttrView::checkBox_clicked_slot(bool checked)
{
	QAbstractButton *const sender = qobject_cast<QAbstractButton*>(QObject::sender());
	if (!sender)
		return;

	// Get the saved DosAttrView value.
	const bool value = sender->property("DosAttrView.value").toBool();
	if (checked != value) {
		// Toggle this box.
		sender->setChecked(value);
	}
}
