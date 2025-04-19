/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * XfsAttrView.cpp: XFS file system attribute viewer widget.               *
 *                                                                         *
 * Copyright (c) 2022-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://doc.qt.io/qt-5/dnd.html
#include "stdafx.h"
#include "XfsAttrView.hpp"

// XFS flags
#include "librpfile/xattr/xfs_flags.h"

// XfsAttrData
#include "librpfile/xattr/XfsAttrData.h"

/** XfsAttrViewPrivate **/

#include "ui_XfsAttrView.h"
class XfsAttrViewPrivate
{
public:
	XfsAttrViewPrivate()
		: xflags(0)
		, projectId(0)
	{}

private:
	Q_DISABLE_COPY(XfsAttrViewPrivate)

public:
	Ui::XfsAttrView ui;
	uint32_t xflags;
	uint32_t projectId;

	// See XfsAttrData.h
	std::array<QCheckBox*, XFS_ATTR_CHECKBOX_MAX> checkBoxes;

public:
	/**
	 * Retranslate parts of the UI that aren't present in the .ui file.
	 */
	void retranslateUi_nonDesigner(void);

public:
	/**
	 * Update the xflags checkboxes.
	 */
	void updateXFlagsCheckboxes(void);

	/**
	 * Update the project ID.
	 */
	void updateProjectId(void);

	/**
	 * Update the display.
	 */
	inline void updateDisplay(void)
	{
		updateXFlagsCheckboxes();
		updateProjectId();
	}
};

/**
 * Retranslate parts of the UI that aren't present in the .ui file.
 */
void XfsAttrViewPrivate::retranslateUi_nonDesigner(void)
{
	for (size_t i = 0; i < checkBoxes.size(); i++) {
		const XfsAttrCheckboxInfo_t *const p = xfsAttrCheckboxInfo(static_cast<XfsAttrCheckboxID>(i));
		checkBoxes[i]->setText(qpgettext_expr("XfsAttrView", p->label));
		checkBoxes[i]->setToolTip(qpgettext_expr("XfsAttrView", p->tooltip));
	}
}

/**
 * Update the xflags checkboxes.
 */
void XfsAttrViewPrivate::updateXFlagsCheckboxes(void)
{
	static_assert(ARRAY_SIZE(checkBoxes) == XFS_ATTR_CHECKBOX_MAX,
		"checkBoxes and XFS_ATTR_CHECKBOX_MAX are out of sync!");

	// NOTE: Bit 2 is skipped, and the last attribute is 0x80000000.
	uint32_t tmp_xflags = xflags;
	for (size_t i = 0; i < checkBoxes.size(); i++, tmp_xflags >>= 1) {
		if (i == 2)
			tmp_xflags >>= 1;
		bool val = (tmp_xflags & 1);
		checkBoxes[i]->setChecked(val);
		checkBoxes[i]->setProperty("XfsAttrView.value", val);
	}

	// Final attribute (XFS_chkHasAttr / FS_XFLAG_HASATTR)
	bool val = !!(xflags & FS_XFLAG_HASATTR);
	checkBoxes[XFS_chkHasAttr]->setChecked(val);
	checkBoxes[XFS_chkHasAttr]->setProperty("XfsAttrView.value", val);
}

/**
 * Update the project ID display.
 */
void XfsAttrViewPrivate::updateProjectId(void)
{
	ui.lblProjectId->setText(QString::number(projectId));
}

/** XfsAttrView **/

XfsAttrView::XfsAttrView(QWidget *parent)
	: super(parent)
	, d_ptr(new XfsAttrViewPrivate())
{
	Q_D(XfsAttrView);
	d->ui.setupUi(this);

	// Make sure we use the system-wide monospace font for
	// widgets that use monospace text.
	d->ui.lblProjectId->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	// Create the checkboxes.
	static constexpr int col_count = 4;
	int col = 0, row = 0;
	for (size_t i = 0; i < d->checkBoxes.size(); i++) {
		const XfsAttrCheckboxInfo_t *const p = xfsAttrCheckboxInfo(static_cast<XfsAttrCheckboxID>(i));

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
void XfsAttrView::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(XfsAttrView);
		d->ui.retranslateUi(this);
		d->retranslateUi_nonDesigner();
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/**
 * Get the current XFS xflags.
 * @return XFS xflags
 */
uint32_t XfsAttrView::xflags(void) const
{
	Q_D(const XfsAttrView);
	return d->xflags;
}

/**
 * Set the current XFS xflags.
 * @param xflags XFS xflags
 */
void XfsAttrView::setXFlags(uint32_t xflags)
{
	Q_D(XfsAttrView);
	if (d->xflags != xflags) {
		d->xflags = xflags;
		d->updateXFlagsCheckboxes();
	}
}

/**
 * Clear the current XFS xflags.
 */
void XfsAttrView::clearXFlags(void)
{
	Q_D(XfsAttrView);
	if (d->xflags != 0) {
		d->xflags = 0;
		d->updateXFlagsCheckboxes();
	}
}

/**
 * Get the current XFS project ID.
 * @return XFS project ID
 */
uint32_t XfsAttrView::projectId(void) const
{
	Q_D(const XfsAttrView);
	return d->projectId;
}

/**
 * Set the current XFS project ID.
 * @param projectId XFS project ID
 */
void XfsAttrView::setProjectId(uint32_t projectId)
{
	Q_D(XfsAttrView);
	if (d->projectId != projectId) {
		d->projectId = projectId;
		d->updateProjectId();
	}
}

/**
 * Clear the current XFS project ID.
 */
void XfsAttrView::clearProjectId(void)
{
	Q_D(XfsAttrView);
	if (d->projectId != 0) {
		d->projectId = 0;
		d->updateProjectId();
	}
}

/** Widget slots **/

/**
 * Disable user modifications of checkboxes.
 */
void XfsAttrView::checkBox_clicked_slot(bool checked)
{
	QAbstractButton *const sender = qobject_cast<QAbstractButton*>(QObject::sender());
	if (!sender)
		return;

	// Get the saved XfsAttrView value.
	const bool value = sender->property("XfsAttrView.value").toBool();
	if (checked != value) {
		// Toggle this box.
		sender->setChecked(value);
	}
}
