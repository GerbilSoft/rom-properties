/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * iNES.cpp: Nintendo Entertainment System/Famicom ROM reader.             *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 * Copyright (c) 2016 by Egor.                                             *
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


#include "iNES.hpp"
#include "NintendoPublishers.hpp"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

 // C includes. (C++ namespace)
#include <cstring>
#include <cctype>

 // C++ includes.
#include <vector>
#include <algorithm>
using std::vector;

namespace LibRomData {

	class NESPrivate
	{
	public:
		NESPrivate() { }

	private:
		NESPrivate(const NESPrivate &other);
		NESPrivate &operator=(const NESPrivate &other);

	public:
		/** RomFields **/

		// Monospace string formatting.
		static const RomFields::StringDesc nes_string_monospace;

		// ROM fields.
		static const struct RomFields::Desc nes_fields[];

		/** Internal ROM data. **/

		/**
		* NES ROM header.
		*
		* NOTE: Strings are NOT null-terminated!
		*/
#define NES_RomHeader_SIZE 16
#pragma pack(1)
		struct PACKED NES_RomHeader {
			uint8_t magic[4]; // "NES\x1A"
			uint8_t prgrom;
			uint8_t chrrom;
			uint8_t flags6;
			uint8_t flags7;
			uint8_t flags8;
			uint8_t flags9;
			uint8_t flags10;
			uint8_t flags11;
			uint8_t flags12;
			uint8_t flags13;
			uint8_t unused[2];
		};
		static_assert(sizeof(NESPrivate::NES_RomHeader) == NES_RomHeader_SIZE,
			"iNES_RomHeader_SIZE is not 16 bytes.");
#pragma pack()
		enum NES_Flags6 {
			NES_F6_MIRROR_HORI = 0,
			NES_F6_MIRROR_VERT = (1 << 0),
			NES_F6_MIRROR_FOUR = (1 << 3),
			NES_F6_BATTERY = (1 << 1),
			NES_F6_TRAINER = (1 << 2),
			NES_F6_MAPPER_MASK = 0xF0,
			NES_F6_MAPPER_SHIFT = 4,
		};
		enum NES_Flags7 {
			NES_F7_VS = (1 << 0),
			NES_F7_PC10 = (1 << 1),
			NES_F7_NES2_MASK = (1 << 3) | (1 << 2),
			NES_F7_NES2_INES_VAL = 0,
			NES_F7_NES2_NES2_VAL = (1 << 3),
			NES_F7_MAPPER_MASK = 0xF0,
			NES_F7_MAPPER_SHIFT = 4,
		};
		// NES 2.0 stuff
		// Not gonna make enums for those:
		// Byte 8 - Mapper variant
		//   top nibble = submapper, bottom nibble = mapper plane
		// Byte 9 - Rom size upper bits
		//   top = CROM, bottom = PROM
		// Byte 10 - pram
		//   top = battery pram, bottom = normal pram
		// Byte 11 - cram
		//   top = battery cram, bottom = normal cram
		// Byte 13 - vs unisystem
		//   top = vs mode, bottom = ppu version
		enum NES2_Flags12 {
			NES2_F12_NTSC = 0,
			NES2_F12_PAL = (1 << 0),
			NES2_F12_DUAL = (1 << 1),
			NES2_F12_REGION = (1 << 1) | (1 << 0),
		};
	public:
		// ROM header.
		NES_RomHeader romHeader;
		bool isNes2;
	};

	/** NESPrivate **/

	// Monospace string formatting.
	const RomFields::StringDesc NESPrivate::nes_string_monospace = {
		RomFields::StringDesc::STRF_MONOSPACE
	};

	// ROM fields.
	const struct RomFields::Desc NESPrivate::nes_fields[] = {
	};

	/** Internal ROM data. **/

	/** DMG **/

	/**
	* Read a NES ROM.
	*
	* A ROM file must be opened by the caller. The file handle
	* will be dup()'d and must be kept open in order to load
	* data from the ROM.
	*
	* To close the file, either delete this object or call close().
	*
	* NOTE: Check isValid() to determine if this is a valid ROM.
	*
	* @param file Open ROM file.
	*/
	NES::NES(IRpFile *file)
		: RomData(file, NESPrivate::nes_fields, ARRAY_SIZE(NESPrivate::nes_fields))
		, d(new NESPrivate())
	{
		
		d->isNes2 = false; // TODO: nes2.0 -Egor

		// TODO: Only validate that this is an NES ROM here.
		// Load fields elsewhere.
		if (!m_file) {
			// Could not dup() the file handle.
			return;
		}
		
		// Seek to the beginning of the header.
		m_file->rewind();

		// Read the ROM header. [0x10 bytes]
		uint8_t header[0x10];
		size_t size = m_file->read(header, sizeof(header));
		if (size != sizeof(header))
			return;

		// Check if this ROM is supported.
		DetectInfo info;
		info.pHeader = header;
		info.szHeader = sizeof(header);
		info.ext = nullptr; // Not needed for DMG.
		info.szFile = 0;	// TODO: Maybe this is needed for NES 2.0 -Egor
		m_isValid = (isRomSupported(&info) >= 0);

		if (m_isValid) {
			// Save the header for later.
			memcpy(&d->romHeader, header, sizeof(d->romHeader));
		}
	}

	NES::~NES()
	{
		delete d;
	}

	/** ROM detection functions. **/

	/**
	* Is a ROM image supported by this class?
	* @param info DetectInfo containing ROM detection information.
	* @return Class-specific system ID (>= 0) if supported; -1 if not.
	*/
	int NES::isRomSupported_static(const DetectInfo *info)
	{
		if (!info)
			return -1;

		if (info->szHeader >= 0x10) {
			// Check the system name.
			const NESPrivate::NES_RomHeader *romHeader =
				reinterpret_cast<const NESPrivate::NES_RomHeader*>(info->pHeader);
			if (!memcmp(romHeader->magic, "NES\x1A", sizeof(romHeader->magic))) {
				// Found a NES ROM.
				// TODO: return different thingy on NES 2.0 -Egor
				return 0;
			}
		}

		// Not supported.
		return -1;
	}

	/**
	* Is a ROM image supported by this object?
	* @param info DetectInfo containing ROM detection information.
	* @return Class-specific system ID (>= 0) if supported; -1 if not.
	*/
	int NES::isRomSupported(const DetectInfo *info) const
	{
		return isRomSupported_static(info);
	}

	/**
	* Get the name of the system the loaded ROM is designed for.
	* @param type System name type. (See the SystemName enum.)
	* @return System name, or nullptr if type is invalid.
	*/
	const rp_char *NES::systemName(uint32_t type) const
	{
		if (!m_isValid || !isSystemNameTypeValid(type))
			return nullptr;

		static_assert(SYSNAME_TYPE_MASK == 3,
			"NES::systemName() array index optimization needs to be updated.");

		static const rp_char *const sysNames[] = {
			_RP("Nintendo Entertainment System"),_RP("Nintendo Entertainment System"), _RP("NES"), nullptr,
			// TODO: don't forget to add vs/pc-10, noob -Egor
		};

		uint32_t idx = (type & SYSNAME_TYPE_MASK);

		return sysNames[idx];
	}

	/**
	* Get a list of all supported file extensions.
	* This is to be used for file type registration;
	* subclasses don't explicitly check the extension.
	*
	* NOTE: The extensions include the leading dot,
	* e.g. ".bin" instead of "bin".
	*
	* NOTE 2: The strings in the std::vector should *not*
	* be freed by the caller.
	*
	* @return List of all supported file extensions.
	*/
	vector<const rp_char*> NES::supportedFileExtensions_static(void)
	{
		vector<const rp_char*> ret;
		ret.reserve(1);
		ret.push_back(_RP(".nes"));
		return ret;
	}

	/**
	* Get a list of all supported file extensions.
	* This is to be used for file type registration;
	* subclasses don't explicitly check the extension.
	*
	* NOTE: The extensions include the leading dot,
	* e.g. ".bin" instead of "bin".
	*
	* NOTE 2: The strings in the std::vector should *not*
	* be freed by the caller.
	*
	* @return List of all supported file extensions.
	*/
	vector<const rp_char*> NES::supportedFileExtensions(void) const
	{
		return supportedFileExtensions_static();
	}

	/**
	* Load field data.
	* Called by RomData::fields() if the field data hasn't been loaded yet.
	* @return Number of fields read on success; negative POSIX error code on error.
	*/
	int NES::loadFieldData(void)
	{
		if (m_fields->isDataLoaded()) {
			// Field data *has* been loaded...
			return 0;
		}
		else if (!m_file || !m_file->isOpen()) {
			// File isn't open.
			return -EBADF;
		}
		else if (!m_isValid) {
			// ROM image isn't valid.
			return -EIO;
		}

		// NES ROM header
		const NESPrivate::NES_RomHeader *romHeader = &d->romHeader;

		char buffer[64];
		int len;
		// TODO: do everything -Egor
		if (!d->isNes2) {
			// Old iNES
			
		}
		else {
			// NES 2.0
		}
		return (int)m_fields->count();
	}

}