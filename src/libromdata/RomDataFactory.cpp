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
#include "common.h"

// C++ includes.
#include <unordered_map>
#include <vector>
using std::unordered_map;
using std::vector;

// RomData subclasses.
#include "MegaDrive.hpp"
#include "GameCube.hpp"
#include "NintendoDS.hpp"
#include "DMG.hpp"
#include "GameBoyAdvance.hpp"
#include "GameCubeSave.hpp"
#include "N64.hpp"
#include "SNES.hpp"
#include "DreamcastSave.hpp"

namespace LibRomData {

class RomDataFactoryPrivate
{
	private:
		RomDataFactoryPrivate();
		~RomDataFactoryPrivate();

	private:
		RomDataFactoryPrivate(const RomDataFactoryPrivate &other);
		RomDataFactoryPrivate &operator=(const RomDataFactoryPrivate &other);

	public:
		// FIXME: Put isRomSupported() here.
		// Can't do that right now due to lack of 'new' functions.
		//typedef int (*pFnIsRomSupported)(const RomData::DetectInfo *info);
		typedef vector<const rp_char*> (*pFnSupportedFileExtensions)(void);

		struct RomDataFns {
			//pFnIsRomSupported isRomSupported;
			pFnSupportedFileExtensions supportedFileExtensions;
			bool hasThumbnail;
		};

#define GetRomDataFns(sys, thumbnail) \
	{/*sys::isRomSupported_static,*/ sys::supportedFileExtensions_static, thumbnail}
		static const RomDataFns romDataFns[];
};

const RomDataFactoryPrivate::RomDataFns RomDataFactoryPrivate::romDataFns[] = {
	GetRomDataFns(MegaDrive, false),
	GetRomDataFns(GameCube, true),
	GetRomDataFns(NintendoDS, true),
	GetRomDataFns(DMG, false),
	GetRomDataFns(GameBoyAdvance, false),
	GetRomDataFns(GameCubeSave, true),
	GetRomDataFns(N64, false),
	GetRomDataFns(SNES, false),
	GetRomDataFns(DreamcastSave, true),
	{/*nullptr,*/ nullptr, false}
};

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
	info.header.addr = 0;
	info.header.pData = header;
	info.header.size = (uint32_t)file->read(header, sizeof(header));
	if (info.header.size == 0) {
		// Read error.
		return nullptr;
	}

	// Get the file extension.
	info.ext = nullptr;
	rp_string filename = file->filename();
	if (!filename.empty()) {
		// Get the last dot position.
		size_t dot_pos = filename.find_last_of(_RP_CHR('.'));
		if (dot_pos != rp_string::npos) {
			// Dot must be after the last slash.
			// Or backslash on Windows.)
#ifdef _WIN32
			size_t slash_pos = filename.find_last_of(_RP("/\\"));
#else /* !_WIN32 */
			size_t slash_pos = filename.find_last_of(_RP_CHR('/'));
#endif /* _WIN32 */
			if (slash_pos == rp_string::npos || slash_pos < dot_pos) {
				// Valid file extension.
				info.ext = filename.c_str() + dot_pos;
			}
		}
	}

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
	CheckRomData_imgbf(GameCubeSave);
	CheckRomData(N64);
	CheckRomData(SNES);
	CheckRomData(DreamcastSave);

	// Not supported.
	return nullptr;
}

/**
 * Get all supported file extensions.
 * Used for Win32 COM registration.
 * @return All supported file extensions, including the leading dot.
 */
vector<RomDataFactory::ExtInfo> RomDataFactory::supportedFileExtensions(void)
{
	// In order to handle multiple RomData subclasses
	// that support the same extensions, we're using
	// an unordered_map. If any of the handlers for a
	// given extension support thumbnails, then the
	// thumbnail handlers will be registered.
	unordered_map<const rp_char*, bool> exts;

	const RomDataFactoryPrivate::RomDataFns *fns =
		&RomDataFactoryPrivate::romDataFns[0];
	for (; fns->supportedFileExtensions != nullptr; fns++) {
		vector<const rp_char*> sys_vec = fns->supportedFileExtensions();
#if !defined(_MSC_VER) || _MSC_VER >= 1700
		exts.reserve(exts.size() + sys_vec.size());
#endif
		for (vector<const rp_char*>::const_iterator iter = sys_vec.begin();
		     iter != sys_vec.end(); ++iter)
		{
			exts[*iter] |= fns->hasThumbnail;
		}
	}

	// Convert to vector<ExtInfo>.
	vector<ExtInfo> vec;
	vec.reserve(exts.size());

	ExtInfo extInfo;
	for (unordered_map<const rp_char*, bool>::const_iterator iter = exts.begin();
	     iter != exts.end(); ++iter)
	{
		extInfo.ext = iter->first;
		extInfo.hasThumbnail = iter->second;
		vec.push_back(extInfo);
	}

	return vec;
}

}
