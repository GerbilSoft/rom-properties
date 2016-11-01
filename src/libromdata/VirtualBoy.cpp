/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DMG.hpp: Virtual Boy ROM reader.                                        *
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

#include "VirtualBoy.hpp"
#include "NintendoPublishers.hpp"
#include "vb_structs.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>
#include <cctype>

// C++ includes.
#include <vector>
#include <algorithm>
using std::vector;

namespace LibRomData {

class VirtualBoyPrivate{
	public:
		VirtualBoyPrivate() { }

	private:
		VirtualBoyPrivate(const VirtualBoyPrivate &other);
		VirtualBoyPrivate &operator=(const VirtualBoyPrivate &other);
	public:
		/** RomFields **/
		static const struct RomFields::Desc vb_fields[];
	
	public:
		/**
		 * Is character a valid JIS X 0201 codepoint?
		 * @param c The character
		 * @return Wether or not character is valid
		 */
		static bool inline isJISX0201(unsigned char c);
		
		/**
		 * Is character a valid Game ID character?
		 * @param c The character
		 * @return Wether or not character is valid
		 */
		static bool inline isGameID(char c);
	
	public:
		// ROM header.
		VB_RomHeader romHeader;
};
/** VirtualBoyPrivate **/

// ROM fields.
const struct RomFields::Desc VirtualBoyPrivate::vb_fields[] = {
	{_RP("Title"), RomFields::RFT_STRING, nullptr},
	{_RP("Game ID"), RomFields::RFT_STRING, nullptr},
	{_RP("Publisher"), RomFields::RFT_STRING, nullptr},
	{_RP("Revision"), RomFields::RFT_STRING, nullptr},
	{_RP("Region"), RomFields::RFT_STRING, nullptr},
};

/**
 * Is character a valid JIS X 0201 codepoint?
 * @param c The character
 * @return Wether or not character is valid
 */
static bool inline isJISX0201(unsigned char c){
	return (c>=' ' && c<='~') || (c>0xA0 && c<0xE0);
}

/**
 * Is character a valid Game ID character?
 * @param c The character
 * @return Wether or not character is valid
 */
static bool inline isGameID(char c){
	return (c>='A' && c<='Z') || (c>='0' && c<='9');
}

/** VirtualBoy **/

/**
 * Read a Virtual Boy ROM.
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
VirtualBoy::VirtualBoy(IRpFile *file)
	: super(file, VirtualBoyPrivate::vb_fields, ARRAY_SIZE(VirtualBoyPrivate::vb_fields))
	, d(new VirtualBoyPrivate())
	
{
	// TODO: Only validate that this is an Virtual Boy ROM here.
	// Load fields elsewhere.
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Seek to the beginning of the header.
	int64_t filesize = m_file->fileSize();
	if(filesize<0x220) return; // too small
	m_file->seek(filesize-0x220);

	// Read the header. [0x20 bytes]
	uint8_t header[0x20];
	size_t size = m_file->read(header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM is supported.
	DetectInfo info;
	info.header.addr = filesize-0x220;
	info.header.size = sizeof(header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for Virtual Boy.
	info.szFile = filesize;
	m_isValid = isRomSupported(&info)>=0;
	
	if (m_isValid) {
		// Save the header for later.
		// TODO: Save the vector table?
		memcpy(&d->romHeader, header, sizeof(d->romHeader));
	}
}

VirtualBoy::~VirtualBoy()
{
	delete d;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int VirtualBoy::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == info->szFile - 0x220);
	if (!info || !info->header.pData ||
		info->header.addr != info->szFile - 0x220 ||
		info->header.size < 0x20)
		return -1;
	
	const VB_RomHeader *romHeader =
		reinterpret_cast<const VB_RomHeader*>(info->header.pData);
	switch(info->szFile){
		// NOTE: there are only 3 types of rom in existance: 512KiB, 1MiB, and 2MiB.
		case 0x80000:
		case 0x100000:
		case 0x200000:
			// NOTE: The following is true for every Virtual Boy ROM:
			// 1) First 20 bytes of title are non-control JIS X 0201 characters (padded with space if needed)
			// 2) 21th byte is a null
			// 3) Game ID is either VxxJ (for Japan) or VxxE (for USA) (xx are alphanumeric uppercase characters)
			// 4) ROM version is always 0, but let's not count on that.
			// 5) And, obviously, the publisher is always valid, but again let's not rely on this
			// NOTE: We're supporting all no-intro ROMs except for "Space Pinball (Unknown) (Proto).vb" as it doesn't have a valid header at all
			if(romHeader->title[20] || romHeader->gameid[0] != 'V' || ( romHeader->gameid[3] != 'J' && romHeader->gameid[3] != 'E' )){
				return -1;
			}
			for(int i=0;i<20;i++){
				if(!isJISX0201(romHeader->title[i])){
					return -1;
				}
			}
			if(!isGameID(romHeader->gameid[1]) || !isGameID(romHeader->gameid[2])){
				return -1;
			}
			return 0;
	}
	
	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int VirtualBoy::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @return System name, or nullptr if not supported.
 */
const rp_char *VirtualBoy::systemName(uint32_t type) const
{
	if (!m_isValid || !isSystemNameTypeValid(type))
		return nullptr;
	
	static_assert(SYSNAME_TYPE_MASK == 3,
		"VirtualBoy::systemName() array index optimization needs to be updated.");
	
	static const rp_char *const sysNames[4] = {
		_RP("Nintendo Virtual Boy"), _RP("Virtual Boy"), _RP("VB"), nullptr,
	};
	
	return sysNames[type & SYSNAME_TYPE_MASK];
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
vector<const rp_char*> VirtualBoy::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		_RP(".vb")
	};
	return vector<const rp_char*>(exts, exts + ARRAY_SIZE(exts));
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
vector<const rp_char*> VirtualBoy::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int VirtualBoy::loadFieldData(void)
{
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!m_file || !m_file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Virtual Boy ROM header, excluding the vector table.
	const VB_RomHeader *const romHeader = &d->romHeader;
	
	// Title
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->title,sizeof(romHeader->title)));
	
	// Game ID
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->gameid,sizeof(romHeader->gameid)));
	
	// Publisher
	const rp_char* publisher = NintendoPublishers::lookup(romHeader->publisher);
	m_fields->addData_string(publisher?publisher:_RP("Unknown"));
	
	// Revision
	m_fields->addData_string_numeric(romHeader->version, RomFields::FB_DEC, 2);
	
	// Region
	const rp_char* region;
	switch(romHeader->gameid[3]){
		case 'J':
			region = _RP("Japan");
			break;
		case 'E':
			region = _RP("USA");
			break;
		default:
			region = _RP("Unknown");
			break;
	}
	m_fields->addData_string(region);
	
	return (int)m_fields->count();
}

}
