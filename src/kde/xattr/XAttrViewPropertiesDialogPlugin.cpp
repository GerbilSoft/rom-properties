/*******************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                             *
 * XAttrViewPropertiesDialogPlugin.cpp: KPropertiesDialogPlugin implementation *
 *                                                                             *
 * Copyright (c) 2016-2024 by David Korth.                                     *
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
	if (!config->getBoolConfigOption_default(Config::BoolConfig::Options_ShowXAttrView)) {
		// XAttrView is disabled.
		return;
	}

	// Create the XAttrView.
	XAttrView *const xattrView = createXAttrView(fileItem, props);
	if (xattrView) {
		// tr: XAttrView tab title
		props->addPage(xattrView, QC_("XAttrView", "xattrs"));
	}
}
