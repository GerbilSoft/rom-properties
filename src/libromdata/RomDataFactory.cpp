/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomDataFactory.hpp: RomData factory class.                              *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

// librpbase
#include "librpbase/common.h"
#include "librpbase/RomData.hpp"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/RpFile.hpp"
#include "librpbase/file/FileSystem.hpp"
#include "librpbase/file/RelatedFile.hpp"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <string>
#include <unordered_map>
#include <vector>
using std::string;
using std::unordered_map;
using std::vector;

// RomData subclasses: Consoles
#include "Console/Dreamcast.hpp"
#include "Console/DreamcastSave.hpp"
#include "Console/GameCube.hpp"
#include "Console/GameCubeSave.hpp"
#include "Console/MegaDrive.hpp"
#include "Console/N64.hpp"
#include "Console/NES.hpp"
#include "Console/PlayStationSave.hpp"
#include "Console/Sega8Bit.hpp"
#include "Console/SegaSaturn.hpp"
#include "Console/SNES.hpp"
#include "Console/WiiU.hpp"

// RomData subclasses: Handhelds
#include "Handheld/DMG.hpp"
#include "Handheld/GameBoyAdvance.hpp"
#include "Handheld/Lynx.hpp"
#include "Handheld/Nintendo3DS.hpp"
#include "Handheld/Nintendo3DSFirm.hpp"
#include "Handheld/NintendoDS.hpp"
#include "Handheld/VirtualBoy.hpp"

// RomData subclasses: Textures
#include "Texture/DirectDrawSurface.hpp"
#include "Texture/KhronosKTX.hpp"
#include "Texture/SegaPVR.hpp"
#include "Texture/ValveVTF.hpp"
#include "Texture/ValveVTF3.hpp"

// RomData subclasses: Other
#include "Other/Amiibo.hpp"
#include "Other/ELF.hpp"
#include "Other/EXE.hpp"
#include "Other/NintendoBadge.hpp"

// Special case for Dreamcast save files.
#include "Console/dc_structs.h"

namespace LibRomData {

class RomDataFactoryPrivate
{
	private:
		RomDataFactoryPrivate();
		~RomDataFactoryPrivate();

	private:
		RP_DISABLE_COPY(RomDataFactoryPrivate)

	public:
		typedef int (*pFnIsRomSupported)(const RomData::DetectInfo *info);
		typedef const char *const * (*pFnSupportedFileExtensions)(void);
		typedef RomData* (*pFnNewRomData)(IRpFile *file);

		struct RomDataFns {
			pFnIsRomSupported isRomSupported;
			pFnNewRomData newRomData;
			pFnSupportedFileExtensions supportedFileExtensions;
			bool hasThumbnail;

			// Extra fields for files whose headers
			// appear at specific addresses.
			uint32_t address;
			uint32_t size;
		};

		/**
		 * Templated function to construct a new RomData subclass.
		 * @param klass Class name.
		 */
		template<typename klass>
		static LibRpBase::RomData *RomData_ctor(LibRpBase::IRpFile *file)
		{
			return new klass(file);
		}

#define GetRomDataFns(sys, hasThumbnail) \
	{sys::isRomSupported_static, \
	 RomDataFactoryPrivate::RomData_ctor<sys>, \
	 sys::supportedFileExtensions_static, \
	 hasThumbnail, 0, 0}
#define GetRomDataFns_addr(sys, hasThumbnail, address, size) \
	{sys::isRomSupported_static, \
	 RomDataFactoryPrivate::RomData_ctor<sys>, \
	 sys::supportedFileExtensions_static, \
	 hasThumbnail, address, size}

		// RomData subclasses that use a header.
		// Headers with addresses other than 0 should be
		// placed at the end of this array.
		static const RomDataFns romDataFns_header[];

		// RomData subclasses that use a footer.
		static const RomDataFns romDataFns_footer[];

		/**
		 * Attempt to open the other file in a Dreamcast .VMI+.VMS pair.
		 * @param file One opened file in the .VMI+.VMS pair.
		 * @return DreamcastSave if valid; nullptr if not.
		 */
		static RomData *openDreamcastVMSandVMI(IRpFile *file);
};

/** RomDataFactoryPrivate **/

const RomDataFactoryPrivate::RomDataFns RomDataFactoryPrivate::romDataFns_header[] = {
	// Consoles
	GetRomDataFns(Dreamcast, true),
	GetRomDataFns(DreamcastSave, true),
	GetRomDataFns(GameCube, true),
	GetRomDataFns(GameCubeSave, true),
	GetRomDataFns(MegaDrive, false),
	GetRomDataFns(N64, false),
	GetRomDataFns(NES, false),
	GetRomDataFns(SNES, false),
	GetRomDataFns(SegaSaturn, false),
	GetRomDataFns(WiiU, true),

	// Handhelds
	GetRomDataFns(DMG, false),
	GetRomDataFns(GameBoyAdvance, false),
	GetRomDataFns(Lynx, false),
	GetRomDataFns(Nintendo3DS, true),
	GetRomDataFns(Nintendo3DSFirm, false),
	GetRomDataFns(NintendoDS, true),

	// Textures
	GetRomDataFns(DirectDrawSurface, true),
	GetRomDataFns(KhronosKTX, true),
	GetRomDataFns(SegaPVR, true),
	GetRomDataFns(ValveVTF, true),
	GetRomDataFns(ValveVTF3, true),

	// Other
	GetRomDataFns(Amiibo, true),
	GetRomDataFns(ELF, false),
	GetRomDataFns(NintendoBadge, true),

	// The following formats have 16-bit magic numbers,
	// so they should go at the end of the address=0 section.
	GetRomDataFns(EXE, false),	// TODO: Thumbnailing on non-Windows platforms.
	GetRomDataFns(PlayStationSave, true),

	// Headers with non-zero addresses.
	GetRomDataFns_addr(Sega8Bit, false, 0x7FE0, 0x20),

	{nullptr, nullptr, nullptr, false, 0, 0}
};

const RomDataFactoryPrivate::RomDataFns RomDataFactoryPrivate::romDataFns_footer[] = {
	GetRomDataFns(VirtualBoy, false),
	{nullptr, nullptr, nullptr, false, 0, 0}
};

/**
 * Attempt to open the other file in a Dreamcast .VMI+.VMS pair.
 * @param file One opened file in the .VMI+.VMS pair.
 * @return DreamcastSave if valid; nullptr if not.
 */
RomData *RomDataFactoryPrivate::openDreamcastVMSandVMI(IRpFile *file)
{
	// We're assuming the file extension was already checked.
	// VMS files are always a multiple of 512 bytes,
	// or 160 bytes for some monochrome ICONDATA_VMS.
	// VMI files are always 108 bytes;
	int64_t filesize = file->size();
	bool has_dc_vms = (filesize % DC_VMS_BLOCK_SIZE == 0) ||
			  (filesize == DC_VMS_ICONDATA_MONO_MINSIZE);
	bool has_dc_vmi = (filesize == DC_VMI_Header_SIZE);
	if (!(has_dc_vms ^ has_dc_vmi)) {
		// Can't be none or both...
		return nullptr;
	}

	// Determine which file we should look for.
	IRpFile *vms_file;
	IRpFile *vmi_file;
	IRpFile **other_file;	// Points to vms_file or vmi_file.

	const char *rel_ext;
	if (has_dc_vms) {
		// We have the VMS file.
		// Find the VMI file.
		vms_file = file;
		vmi_file = nullptr;
		other_file = &vmi_file;
		rel_ext = ".VMI";
	} else /*if (has_dc_vmi)*/ {
		// We have the VMI file.
		// Find the VMS file.
		vms_file = nullptr;
		vmi_file = file;
		other_file = &vms_file;
		rel_ext = ".VMS";
	}

	// Attempt to open the other file in the pair.
	// TODO: Verify length.
	// TODO: For .vmi, check the VMS resource name?
	const string filename = file->filename();
	*other_file = FileSystem::openRelatedFile(filename.c_str(), nullptr, rel_ext);
	if (!*other_file) {
		// Can't open the other file.
		return nullptr;
	}

	// Attempt to create a DreamcastSave using both the
	// VMS and VMI files.
	DreamcastSave *dcSave = new DreamcastSave(vms_file, vmi_file);
	delete *other_file;	// Not needed anymore.
	if (!dcSave->isValid()) {
		// Not valid.
		dcSave->unref();
		return nullptr;
	}

	// DreamcastSave opened.
	return dcSave;
}

/** RomDataFactory **/

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
RomData *RomDataFactory::create(IRpFile *file, bool thumbnail)
{
	RomData::DetectInfo info;

	// Get the file size.
	info.szFile = file->size();

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
	const string filename = file->filename();
	if (!filename.empty()) {
		info.ext = FileSystem::file_ext(filename);
	}

	// Special handling for Dreamcast .VMI+.VMS pairs.
	if (info.ext != nullptr &&
	    (!strcasecmp(info.ext, ".vms") ||
	     !strcasecmp(info.ext, ".vmi")))
	{
		// Dreamcast .VMI+.VMS pair.
		// Attempt to open the other file in the pair.
		RomData *romData = RomDataFactoryPrivate::openDreamcastVMSandVMI(file);
		if (romData) {
			if (romData->isValid()) {
				// .VMI+.VMS pair opened.
				return romData;
			}
			// Not a .VMI+.VMS pair.
			romData->unref();
		}

		// Not a .VMI+.VMS pair.
	}

	// Check RomData subclasses that take a header.
	const RomDataFactoryPrivate::RomDataFns *fns =
		&RomDataFactoryPrivate::romDataFns_header[0];
	for (; fns->supportedFileExtensions != nullptr; fns++) {
		if (thumbnail && !fns->hasThumbnail) {
			// Thumbnail is requested, but this RomData class
			// doesn't support any images.
			continue;
		}

		if (fns->address != info.header.addr ||
		    fns->size > info.header.size)
		{
			// Header address has changed.

			// Check the file extension to reduce overhead
			// for file types that don't use this.
			// TODO: Don't hard-code this.
			// Use a pointer to supportedFileExtensions_static() instead?
			if (info.ext == nullptr) {
				// No file extension...
				break;
			} else if (strcasecmp(info.ext, ".sms") != 0 &&
				   strcasecmp(info.ext, ".gg") != 0)
			{
				// Not SMS or Game Gear.
				break;
			}

			// Read the new header data.

			// NOTE: fns->size == 0 is only correct
			// for headers located at 0, since we
			// read the whole 4096+256 bytes for these.
			assert(fns->size != 0);
			assert(fns->size <= sizeof(header));
			if (fns->size == 0 || fns->size > sizeof(header))
				continue;

			// Make sure the file is big enough to
			// have this header.
			if (((int64_t)fns->address + fns->size) > info.szFile)
				continue;

			// Read the header data.
			info.header.addr = fns->address;
			int ret = file->seek(info.header.addr);
			if (ret != 0)
				continue;
			info.header.size = (uint32_t)file->read(header, fns->size);
			if (info.header.size != fns->size)
				continue;
		}

		if (fns->isRomSupported(&info) >= 0) {
			RomData *const romData = fns->newRomData(file);
			if (romData->isValid()) {
				// RomData subclass obtained.
				return romData;
			}

			// Not actually supported.
			romData->unref();
		}
	}

	// Check RomData subclasses that take a footer.
	if (info.szFile > (1LL << 30)) {
		// No subclasses that expect footers support
		// files larger than 1 GB.
		return nullptr;
	}

	bool readFooter = false;
	fns = &RomDataFactoryPrivate::romDataFns_footer[0];
	for (; fns->supportedFileExtensions != nullptr; fns++) {
		if (thumbnail && !fns->hasThumbnail) {
			// Thumbnail is requested, but this RomData class
			// doesn't support any images.
			continue;
		}

		// Do we have a matching extension?
		// FIXME: Instead of hard-coded, check supportedFileExtensions.
		// Currently only supports VirtualBoy.
		if (!info.ext || strcasecmp(info.ext, ".vb") != 0) {
			// Extension doesn't match.
			continue;
		}

		// Make sure we've read the footer.
		if (!readFooter) {
			static const int footer_size = 1024;
			if (info.szFile > footer_size) {
				info.header.addr = (uint32_t)(info.szFile - footer_size);
				info.header.size = (uint32_t)file->seekAndRead(info.header.addr, header, footer_size);
				if (info.header.size == 0) {
					// Seek and/or read error.
					return nullptr;
				}
			}
			readFooter = true;
		}

		if (fns->isRomSupported(&info) >= 0) {
			RomData *const romData = fns->newRomData(file);
			if (romData->isValid()) {
				// RomData subclass obtained.
				return romData;
			}

			// Not actually supported.
			romData->unref();
		}
	}

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
	// FIXME: May need to use string instead of char*
	// for proper hashing.
	unordered_map<const char*, bool> exts;

	const RomDataFactoryPrivate::RomDataFns *fns =
		&RomDataFactoryPrivate::romDataFns_header[0];
	for (; fns->supportedFileExtensions != nullptr; fns++) {
		const char *const *sys_exts = fns->supportedFileExtensions();
		if (!sys_exts)
			continue;

#if !defined(_MSC_VER) || _MSC_VER >= 1700
		exts.reserve(exts.size() + 4);
#endif
		for (; *sys_exts != nullptr; sys_exts++) {
			exts[*sys_exts] |= fns->hasThumbnail;
		}
	}

	fns = &RomDataFactoryPrivate::romDataFns_footer[0];
	for (; fns->supportedFileExtensions != nullptr; fns++) {
		const char *const *sys_exts = fns->supportedFileExtensions();
		if (!sys_exts)
			continue;

#if !defined(_MSC_VER) || _MSC_VER >= 1700
		exts.reserve(exts.size() + 4);
#endif
		for (; *sys_exts != nullptr; sys_exts++) {
			exts[*sys_exts] |= fns->hasThumbnail;
		}
	}

	// Convert to vector<ExtInfo>.
	vector<ExtInfo> vec;
	vec.reserve(exts.size());
	
	ExtInfo extInfo;
	for (auto iter = exts.cbegin(); iter != exts.cend(); ++iter) {
		extInfo.ext = iter->first;
		extInfo.hasThumbnail = iter->second;
		vec.push_back(extInfo);
	}

	return vec;
}

}
