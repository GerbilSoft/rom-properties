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
#include "libromdata/RomDataFactory.hpp"
#include "RomDataView.hpp"

RomPropertiesDialogPlugin::RomPropertiesDialogPlugin(KPropertiesDialog *props, const QVariantList&)
	: KPropertiesDialogPlugin(props)
{
	// Check if a single file was specified.
#if QT_VERSION >= 0x050000
	QUrl url = props->url();
#else /* QT_VERSION < 0x050000 */
	KUrl url = props->kurl();
#endif
	if (url.isValid() && url.isLocalFile()) {
		// Single file, and it's local.
		// Open it and read the first 65536+512 bytes.
		// TODO: Use KIO and transparent decompression?
		QString filename = url.toLocalFile();
		if (!filename.isEmpty()) {
			// TODO: rp_QFile() wrapper?
			// For now, using stdio.
			FILE *file = fopen(filename.toUtf8().constData(), "rb");
			if (file) {
				// Get the appropriate RomData class for this ROM.
				LibRomData::RomData *romData = LibRomData::RomDataFactory::getInstance(file);
				if (romData) {
					// MD ROM. Show the properties.
					RomDataView *romDataView = new RomDataView(romData, props);
					props->addPage(romDataView, tr("ROM Properties"));
				}
			}

			// RomData classes dup() the file, so
			// we can close the original one.
			fclose(file);
		}
	}
}

RomPropertiesDialogPlugin::~RomPropertiesDialogPlugin()
{ }
