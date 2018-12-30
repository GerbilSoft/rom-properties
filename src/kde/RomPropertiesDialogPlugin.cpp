/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomPropertiesDialogPlugin.cpp: KPropertiesDialogPlugin.                 *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
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
using LibRpBase::RpFile;

// libi18n
#include "libi18n/i18n.h"

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// C++ includes.
#include <memory>
using std::unique_ptr;

// Qt includes.
#include <QtCore/QFileInfo>

RomPropertiesDialogPlugin::RomPropertiesDialogPlugin(KPropertiesDialog *props, const QVariantList&)
	: super(props)
{
	// Check if a single file was specified.
	KFileItemList items = props->items();
	if (items.size() != 1) {
		// Either zero items or more than one item.
		return;
	}

	// Get the local path. This is needed in order to
	// support files on the local file system that
	// don't have a file:/ URL, e.g. desktop:/.
	// References:
	// - https://bugs.kde.org/show_bug.cgi?id=392100
	// - https://cgit.kde.org/kio.git/commit/?id=7d6e4965dfcd7fc12e8cba7b1506dde22de5d2dd
	// TODO: https://cgit.kde.org/kdenetwork-filesharing.git/commit/?id=abf945afd4f08d80cdc53c650d80d300f245a73d
	// (and other uses) [use mostLocalPath()]
	const KFileItem &fileItem = items.at(0);
	const QString filename = fileItem.localPath();
	if (filename.isEmpty()) {
		// Unable to get a local path.
		return;
	}

	// Single file, and it's local.
	// TODO: Use KIO and transparent decompression?
	// TODO: RpQFile wrapper.
	// For now, using RpFile, which is an stdio wrapper.
	unique_ptr<RpFile> file(new RpFile(Q2U8(filename), RpFile::FM_OPEN_READ_GZ));
	if (!file || !file->isOpen())
		return;

	// Get the appropriate RomData class for this ROM.
	// file is dup()'d by RomData.
	RomData *const romData = RomDataFactory::create(file.get());
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
