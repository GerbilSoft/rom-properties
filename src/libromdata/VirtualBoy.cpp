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
#include "TextFuncs.hpp"
#include "common.h"

// C includes. (C++ namespace)
#include <cstring>
#include <cctype>

// C++ includes.
#include <vector>
#include <algorithm>
using std::vector;

namespace LibRomData {

static const struct RomFields::Desc vb_fields[] = {
	{_RP("Title"), RomFields::RFT_STRING, nullptr},
	{_RP("GameID"), RomFields::RFT_STRING, nullptr},
	{_RP("Publisher"), RomFields::RFT_STRING, nullptr},
	{_RP("Revision"), RomFields::RFT_STRING, nullptr},
};

/**
 * Virtual Boy ROM header.
 * 
 * NOTE: Strings Mayare NOT null-terminated!
 */
struct VB_RomHeader {
	char title[21];
	uint8_t reserved[4];
	char publisher[2];
	char gameid[4];
	uint8_t version;
};

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
	: RomData(file, vb_fields, ARRAY_SIZE(vb_fields))
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
	info.pHeader = header;
	info.szHeader = sizeof(header);
	info.ext = nullptr;	// Not needed for Virtual Boy.
	info.szFile = filesize;
	m_isValid = isRomSupported(&info)>=0;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int VirtualBoy::isRomSupported_static(const DetectInfo *info)
{
	if (!info)
		return -1;
	
	if (info->szHeader >= 0x20) {
		const VB_RomHeader *romHeader =
			reinterpret_cast<const VB_RomHeader*>(info->pHeader);
		switch(info->szFile){
			// NOTE: there are only 3 types of rom in existance: 512KiB, 1MiB, and 2MiB.
			case 0x80000:
			case 0x100000:
			case 0x200000:
				// TODO: Validate strings
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
int VirtualBoy::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @return System name, or nullptr if not supported.
 */
const rp_char *VirtualBoy::systemName(void) const
{
	return _RP("Virtual Boy");
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
	vector<const rp_char*> ret;
	ret.reserve(1);
	ret.push_back(_RP(".vb"));
	return ret;
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

	// Read the header. [0x20 bytes]
	uint8_t header[0x20];
	int64_t filesize = m_file->fileSize();
	if(filesize < 0x220){
		// File isn't big enough for a Virtual Boy header...
		return -EIO;
	}
	m_file->seek(filesize-0x220);
	size_t size = m_file->read(header, sizeof(header));
	if (size != sizeof(header)) {
		return -EIO;
		
	}

	// Virtual Boy ROM header, excluding the vector table.
	const VB_RomHeader *romHeader = reinterpret_cast<const VB_RomHeader*>(header);
	
	
	// Title
	m_fields->addData_string(ascii_to_rp_string(romHeader->title,21));
	
	// Game ID
	m_fields->addData_string(ascii_to_rp_string(romHeader->gameid,4));
	
	// Publisher
	m_fields->addData_string(NintendoPublishers::lookup(romHeader->publisher));
	
	// Revision
	m_fields->addData_string_numeric(romHeader->version, RomFields::FB_DEC, 2);
	
	return (int)m_fields->count();
}

}
