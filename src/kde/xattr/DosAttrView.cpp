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
			: attrs(0) { }

	private:
		Q_DISABLE_COPY(DosAttrViewPrivate)

	public:
		Ui::DosAttrView ui;
		unsigned int attrs;

	public:
		/**
		 * Update the attributes display.
		 */
		void updateAttrsDisplay(void);
};

/**
 * Update the attributes display.
 */
void DosAttrViewPrivate::updateAttrsDisplay(void)
{
	bool val;
#define UPDATE_CHECKBOX(attr, obj) \
	val = !!(attrs & (attr)); \
	ui.obj->setChecked(val); \
	ui.obj->setProperty("DosAttrView.value", val)

	UPDATE_CHECKBOX(FILE_ATTRIBUTE_READONLY, chkReadOnly);
	UPDATE_CHECKBOX(FILE_ATTRIBUTE_HIDDEN, chkHidden);
	UPDATE_CHECKBOX(FILE_ATTRIBUTE_ARCHIVE, chkArchive);
	UPDATE_CHECKBOX(FILE_ATTRIBUTE_SYSTEM, chkSystem);

	UPDATE_CHECKBOX(FILE_ATTRIBUTE_COMPRESSED, chkCompressed);
	UPDATE_CHECKBOX(FILE_ATTRIBUTE_ENCRYPTED, chkEncrypted);
}

/** DosAttrView **/

DosAttrView::DosAttrView(QWidget *parent)
	: super(parent)
	, d_ptr(new DosAttrViewPrivate())
{
	Q_D(DosAttrView);
	d->ui.setupUi(this);

	// Connect checkbox signals.
#define CONNECT_CHECKBOX_SIGNAL(obj) \
	connect(d->ui.obj, SIGNAL(clicked(bool)), \
		this, SLOT(checkBox_clicked_slot(bool)))

	CONNECT_CHECKBOX_SIGNAL(chkReadOnly);
	CONNECT_CHECKBOX_SIGNAL(chkHidden);
	CONNECT_CHECKBOX_SIGNAL(chkArchive);
	CONNECT_CHECKBOX_SIGNAL(chkSystem);

	CONNECT_CHECKBOX_SIGNAL(chkCompressed);
	CONNECT_CHECKBOX_SIGNAL(chkEncrypted);
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
