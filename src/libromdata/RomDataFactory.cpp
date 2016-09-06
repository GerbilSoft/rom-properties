/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomDataFactory.hpp: RomData factory class.                              *
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

#include "RomDataFactory.hpp"
#include "file/IRpFile.hpp"

// C++ includes.
#include <vector>
using std::vector;

// RomData subclasses.
#include "MegaDrive.hpp"
#include "GameCube.hpp"
#include "NintendoDS.hpp"
#include "DMG.hpp"
#include "GameBoyAdvance.hpp"

namespace LibRomData {

/**
 * Create a RomData class for the specified ROM file.
 *
 * NOTE: RomData::isValid() is checked before returning a
 * created RomData instance, so returned objects can be
 * assumed to be valid as long as they aren't nullptr.
 *
 * If imgbf is non-zero, at least one of the specified image
 * types must be supported by the RomData class in order to
 * be returned.
 *
 * @param file ROM file.
 * @param thumbnail If true, RomData class must support at least one image type.
 * @return RomData class, or nullptr if the ROM isn't supported.
 */
RomData *RomDataFactory::getInstance(IRpFile *file, bool thumbnail)
{
	RomData::DetectInfo info;

	// Get the file size.
	info.szFile = file->fileSize();

	// Read 4,096+256 bytes from the ROM header.
	// This should be enough to detect most systems.
	uint8_t header[4096+256];
	file->rewind();
	info.szHeader = file->read(header, sizeof(header));
	info.pHeader = header;

	// TODO: File extension? (Needed for .gci)
	info.ext = nullptr;

#define CheckRomData(sys) \
	do { \
		if (thumbnail) { \
			/* This RomData class doesn't support any images. */ \
			break; \
		} \
		if (sys::isRomSupported_static(&info) >= 0) { \
			RomData *romData = new sys(file); \
			if (romData->isValid()) \
				return romData; \
			\
			/* Not actually supported. */ \
			delete romData; \
		} \
	} while (0)

#define CheckRomData_imgbf(sys) \
	do { \
		if (sys::isRomSupported_static(&info) >= 0) { \
			if (thumbnail) { \
				/* Check if at least one image type is supported. */ \
				if (sys::supportedImageTypes_static() == 0) \
					break; \
			} \
			RomData *romData = new sys(file); \
			if (romData->isValid()) \
				return romData; \
			\
			/* Not actually supported. */ \
			delete romData; \
		} \
	} while (0)

	CheckRomData(MegaDrive);
	CheckRomData_imgbf(GameCube);
	CheckRomData_imgbf(NintendoDS);
	CheckRomData(DMG);
	CheckRomData(GameBoyAdvance);

	// Not supported.
	return nullptr;
}

/**
 * Get all supported file extensions.
 * Used for Win32 COM registration.
 * @return All supported file extensions, including the leading dot
 */
vector<const rp_char*> RomDataFactory::supportedFileExtensions(void)
{
	vector<const rp_char*> vec;

#define GetFileExtensions(sys) \
	do { \
		vector<const rp_char*> sys_vec = sys::supportedFileExtensions_static(); \
		vec.insert(vec.end(), sys_vec.begin(), sys_vec.end()); \
	} while (0)

	GetFileExtensions(MegaDrive);
	GetFileExtensions(GameCube);
	GetFileExtensions(NintendoDS);
	GetFileExtensions(DMG);
	GetFileExtensions(GameBoyAdvance);

	return vec;
}

}
