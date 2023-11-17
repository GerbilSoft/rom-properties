/*******************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                             *
 * XAttrViewPropertiesDialogPlugin.cpp: KPropertiesDialogPlugin implementation *
 *                                                                             *
 * Copyright (c) 2016-2023 by David Korth.                                     *
 * SPDX-License-Identifier: GPL-2.0-or-later                                   *
 *******************************************************************************/

/**
 * References:
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.h
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.cpp
 * - https://github.com/KDE/calligra-history/blob/master/libs/main/KoDocInfoPropsFactory.cpp
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/kodocinfopropspage.desktop
 */

#include "stdafx.h"
#include "XAttrViewPropertiesDialogPlugin.hpp"

#include "XAttrView.hpp"
#include "check-uid.hpp"

#include "librpbase/config/Config.hpp"
using LibRpBase::Config;

/**
 * Instantiate an XAttrView object for the given KFileItem.
 * @param fileItem KFileItem
 * @param props KPropertiesDialog
 * @return XAttrView object, or nullptr if the file is not supported.
 */
XAttrView *XAttrViewPropertiesDialogPlugin::createXAttrView(const KFileItem &fileItem, KPropertiesDialog *props)
{
	XAttrView *const xattrView = new XAttrView(fileItem.url(), props);
	if (!xattrView->hasAttributes()) {
		// No attributes. Don't show the page.
		delete xattrView;
		return nullptr;
	}

	xattrView->setObjectName(QLatin1String("xattrView"));
	return xattrView;
}

/**
 * Instantiate properties pages for the given KPropertiesDialog.
 * @param parent KPropertiesDialog (NOTE: QObject* is used for registerPlugin() compatibility.)
 * @param args
 */
XAttrViewPropertiesDialogPlugin::XAttrViewPropertiesDialogPlugin(QObject *parent, const QVariantList &args)
	: super(qobject_cast<KPropertiesDialog*>(parent))
{
	Q_UNUSED(args)
	CHECK_UID();

	KPropertiesDialog *const props = qobject_cast<KPropertiesDialog*>(parent);
	assert(props != nullptr);
	if (!props) {
		// Parent *must* be KPropertiesDialog.
		throw std::runtime_error("Parent object must be KPropertiesDialog.");
	}

	// Check if a single file was specified.
	KFileItemList items = props->items();
	if (items.size() != 1) {
		// Either zero items or more than one item.
		return;
	}

	const KFileItem &fileItem = items[0];

	// Check if XAttrView is enabled.
	const Config *const config = Config::instance();
	if (!config->showXAttrView()) {
		// XAttrView is disabled.
		return;
	}

	// Create the XAttrView.
	XAttrView *const xattrView = createXAttrView(fileItem, props);
	if (xattrView) {
		// tr: XAttrView tab title
		props->addPage(xattrView, U82Q(C_("XAttrView", "xattrs")));

		// Monitor this XAttrView for changes.
		m_xattrView.insert(xattrView);
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		connect(xattrView, &XAttrView::modified,
		        this, &XAttrViewPropertiesDialogPlugin::xattrView_modified_slot);
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
		connect(xattrView, SIGNAL(modified()),
		        this, SLOT(xattrView_modified_slot()));
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
	}
}

/** KPropertiesDialogPlugin overrides **/

/**
 * Apply changes to the file(s).
 */
void XAttrViewPropertiesDialogPlugin::applyChanges(void)
{
	// Apply changes to all XAttrView objects.
	for (XAttrView *xattrView : m_xattrView) {
		xattrView->applyChanges();
	}
	setDirty(false);
}

/** Signal handlers from XAttrView widgets **/

/**
 * An XAttrView widget was modified.
 */
void XAttrViewPropertiesDialogPlugin::xattrView_modified_slot(void)
{
	setDirty(true);
}

/**
 * An XAttrView widget was destroyed.
 * @param obj XAttrView widget
 */
void XAttrViewPropertiesDialogPlugin::xattrView_destroyed_slot(QObject *obj)
{
	if (!obj)
		return;

	// Remove this object from the set.
	m_xattrView.erase(reinterpret_cast<XAttrView*>(obj));
}

