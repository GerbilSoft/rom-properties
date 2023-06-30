/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomPropertiesDialogPlugin.cpp: KPropertiesDialogPlugin implementation   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.h
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.cpp
 * - https://github.com/KDE/calligra-history/blob/master/libs/main/KoDocInfoPropsFactory.cpp
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/kodocinfopropspage.desktop
 */

#include "stdafx.h"
#include "check-uid.hpp"
#include "RomPropertiesDialogPlugin.hpp"
#include "RomDataView.hpp"

// librpbase, librpfile
using LibRpBase::RomData;
using LibRpFile::IRpFile;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// C++ STL classes
using std::string;

/**
 * Instantiate a RomDataView object for the given QUrl.
 * @param fileItem KFileItem
 * @param props KPropertiesDialog
 * @return RomDataView object, or nullptr if the file is not supported.
 */
RomDataView *RomPropertiesDialogPlugin::createRomDataView(const KFileItem &fileItem, KPropertiesDialog *props)
{
	// Attempt to open the ROM file.
	IRpFile *const file = openQUrl(fileItem.url(), false);
	if (!file) {
		// Unable to open the file.
		return nullptr;
	}

	// Get the appropriate RomData class for this ROM.
	RomData *const romData = RomDataFactory::create(file);
	file->unref();	// file is ref()'d by RomData.
	if (!romData) {
		// ROM is not supported.
		return nullptr;
	}

	// ROM is supported. Show the properties.
	RomDataView *const romDataView = new RomDataView(romData, props);
	romDataView->setObjectName(QLatin1String("romDataView"));

	// Make sure the underlying file handle is closed,
	// since we don't need it once the RomData has been
	// loaded by RomDataView.
	romData->close();

	// RomDataView takes a reference to the RomData object.
	// We don't need to hold on to it.
	romData->unref();

	return romDataView;
}

/**
 * Instantiate RomDataView for the given KPropertiesDialog.
 * @param parent KPropertiesDialog (NOTE: QObject* is used for registerPlugin() compatibility.)
 * @param args
 */
RomPropertiesDialogPlugin::RomPropertiesDialogPlugin(QObject *parent, const QVariantList &args)
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

	// Create the RomDataView.
	RomDataView *const romDataView = createRomDataView(fileItem, props);
	if (romDataView) {
		// tr: RomDataView tab title
		props->addPage(romDataView, U82Q(C_("RomDataView", "ROM Properties")));
	}
}
