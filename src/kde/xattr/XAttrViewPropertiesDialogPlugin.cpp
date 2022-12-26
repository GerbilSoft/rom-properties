/*******************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                             *
 * XAttrViewPropertiesDialogPlugin.cpp: KPropertiesDialogPlugin for XAttrView. *
 *                                                                             *
 * Copyright (c) 2016-2022 by David Korth.                                     *
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
#include "check-uid.hpp"
#include "XAttrViewPropertiesDialogPlugin.hpp"
#include "XAttrView.hpp"

/**
 * Instantiate a XAttrView for the given KPropertiesDialog.
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

	XAttrView *const xattrView = new XAttrView(items[0].url());
	if (!xattrView->hasAttributes()) {
		// No attributes. Don't show the page.
		delete xattrView;
		return;
	}

	// We have attributes. Show the page.
	xattrView->setObjectName(QLatin1String("xattrView"));

	// tr: Tab title.
	// TODO: Change to "Extended Attributes"?
	props->addPage(xattrView, QLatin1String("xattrs"));
}
