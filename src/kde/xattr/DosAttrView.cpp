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
		, canWriteAttrs(false)
	{}

private:
	Q_DISABLE_COPY(DosAttrViewPrivate)

public:
	Ui::DosAttrView ui;
	unsigned int attrs;
	bool canWriteAttrs;

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
	// TODO: Use the new signal/slot syntax in Qt5/Qt6 builds?
#define INIT_RHAS_CHECKBOX(attr, obj) \
	connect(d->ui.obj, SIGNAL(clicked(bool)), \
		this, SLOT(checkBox_writable_clicked_slot(bool))); \
	d->ui.obj->setProperty("DosAttrView.attr", (attr))
#define INIT_OTHER_CHECKBOX(attr, obj) \
	connect(d->ui.obj, SIGNAL(clicked(bool)), \
		this, SLOT(checkBox_noToggle_clicked_slot(bool))); \
	d->ui.obj->setProperty("DosAttrView.attr", (attr))

	INIT_RHAS_CHECKBOX(FILE_ATTRIBUTE_READONLY, chkReadOnly);
	INIT_RHAS_CHECKBOX(FILE_ATTRIBUTE_HIDDEN, chkHidden);
	INIT_RHAS_CHECKBOX(FILE_ATTRIBUTE_ARCHIVE, chkArchive);
	INIT_RHAS_CHECKBOX(FILE_ATTRIBUTE_SYSTEM, chkSystem);

	INIT_OTHER_CHECKBOX(FILE_ATTRIBUTE_COMPRESSED, chkCompressed);
	INIT_OTHER_CHECKBOX(FILE_ATTRIBUTE_ENCRYPTED, chkEncrypted);
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
 * Set if MS-DOS attributes can be written.
 * This controls if the RHAS checkboxes can be edited by the user.
 * @param widget DosAttrView
 * @param can_write True if they can; false if they can't.
 */
void DosAttrView::setCanWriteAttrs(bool canWriteAttrs)
{
	Q_D(DosAttrView);
	d->canWriteAttrs = canWriteAttrs;
}

/**
 * Can MS-DOS attributes be written?
 * This controls if the RHAS checkboxes can be edited by the user.
 * @param widget DosAttrView
 * @return True if they can; false if they can't.
 */
bool DosAttrView::canWriteAttrs(void) const
{
	Q_D(const DosAttrView);
	return d->canWriteAttrs;
}

/** Widget slots **/

/**
 * Allow writable bitfield checkboxes to be toggled.
 * @param checked New check state
 */
void DosAttrView::checkBox_writable_clicked_slot(bool checked)
{
	Q_D(DosAttrView);
	if (!d->canWriteAttrs) {
		// Cannot write to this file. Use the no-toggle signal handler.
		checkBox_noToggle_clicked_slot(checked);
		return;
	}

	QAbstractButton *const sender = qobject_cast<QAbstractButton*>(QObject::sender());
	if (!sender)
		return;

	// Update the attributes bitfield and the qdata in the checkbox widget.
	const uint32_t attr = sender->property("DosAttrView.attr").toUInt();
	sender->setProperty("DosAttrView.value", checked);

	const uint32_t old_attrs = d->attrs;
	if (checked) {
		d->attrs |= attr;
	} else {
		d->attrs &= ~attr;
	}

	// Emit a signal so the parent widget can indicate the attributes were modified.
	// FIXME: Store old attributes somewhere so they can be reverted if setting fails?
	if (likely(old_attrs != d->attrs)) {
		emit modified(old_attrs, d->attrs);
	}
}

/**
 * Prevent bitfield checkboxes from being toggled.
 * @param checked New check state
 */
void DosAttrView::checkBox_noToggle_clicked_slot(bool checked)
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
