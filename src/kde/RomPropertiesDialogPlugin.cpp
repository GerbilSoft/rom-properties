/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomPropertiesDialogPlugin.cpp: KPropertiesDialogPlugin.                 *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.h
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/KoDocumentInfoPropsPage.cpp
 * - https://github.com/KDE/calligra-history/blob/master/libs/main/KoDocInfoPropsFactory.cpp
 * - https://github.com/KDE/calligra-history/blob/5e323f11f11ec487e1ef801d61bb322944f454a5/libs/main/kodocinfopropspage.desktop
 */

#include "RomPropertiesDialogPlugin.hpp"
#include "RomDataView.hpp"
#include "RpQt.hpp"

// librpbase
#include "librpbase/RomData.hpp"
#include "librpbase/file/RpFile.hpp"
using LibRpBase::RomData;
using LibRpBase::IRpFile;
using LibRpBase::RpFile;

// RpFileKio
#include "RpFile_kio.hpp"

// libi18n
#include "libi18n/i18n.h"

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// C includes.
#include <unistd.h>

// Qt includes.
#include <QtCore/QFileInfo>

RomPropertiesDialogPlugin::RomPropertiesDialogPlugin(KPropertiesDialog *props, const QVariantList&)
	: super(props)
{
	if (getuid() == 0 || geteuid() == 0) {
		qCritical("*** rom-properties-kde%u does not support running as root.", QT_VERSION >> 16);
		return;
	}

	// Check if a single file was specified.
	KFileItemList items = props->items();
	if (items.size() != 1) {
		// Either zero items or more than one item.
		return;
	}

	// Get the local path if it's available. This is needed in order to
	// support files on the local file system that don't have a file:/
	// URL, e.g. desktop:/.
	// References:
	// - https://bugs.kde.org/show_bug.cgi?id=392100
	// - https://cgit.kde.org/kio.git/commit/?id=7d6e4965dfcd7fc12e8cba7b1506dde22de5d2dd
	// TODO: https://cgit.kde.org/kdenetwork-filesharing.git/commit/?id=abf945afd4f08d80cdc53c650d80d300f245a73d
	// (and other uses) [use mostLocalPath()]
	IRpFile *file = nullptr;
	const KFileItem &fileItem = items.at(0);
	const QString filename = fileItem.localPath();
	if (!filename.isEmpty()) {
		// Got a local path. Use RpFile.
		file = new RpFile(Q2U8(filename), RpFile::FM_OPEN_READ_GZ);
	} else {
		// Unable to get a local path. Use RpFileKio.
		file = new RpFileKio(fileItem.url());
	}

	if (!file->isOpen()) {
		// Unable to open the file.
		file->unref();
		return;
	}

	// Get the appropriate RomData class for this ROM.
	RomData *const romData = RomDataFactory::create(file);
	file->unref();	// file is ref()'d by RomData.
	if (!romData) {
		// ROM is not supported.
		return;
	}

	// ROM is supported. Show the properties.
	RomDataView *const romDataView = new RomDataView(romData, props);
	// tr: Tab title.
	props->addPage(romDataView, U82Q(C_("RomDataView", "ROM Properties")));

	// Make sure the underlying file handle is closed,
	// since we don't need it once the RomData has been
	// loaded by RomDataView.
	romData->close();

	// RomDataView takes a reference to the RomData object.
	// We don't need to hold on to it.
	romData->unref();
}

RomPropertiesDialogPlugin::~RomPropertiesDialogPlugin()
{ }
