/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DMG.hpp: Virtual Boy ROM reader.                                        *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "VirtualBoy.hpp"
#include "librpbase/RomData_p.hpp"

#include "data/NintendoPublishers.hpp"
#include "vb_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(VirtualBoy)

class VirtualBoyPrivate : public RomDataPrivate
{
	public:
		VirtualBoyPrivate(VirtualBoy *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(VirtualBoyPrivate)

	public:
		/**
		 * Is character a valid JIS X 0201 codepoint?
		 * @param c The character
		 * @return Wether or not character is valid
		 */
		static bool inline isJISX0201(unsigned char c);
		
		/**
		 * Is character a valid Publisher ID character?
		 * @param c The character
		 * @return Wether or not character is valid
		 */
		static bool inline isPublisherID(char c);

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

VirtualBoyPrivate::VirtualBoyPrivate(VirtualBoy *q, IRpFile *file)
	: super(q, file)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/**
 * Is character a valid JIS X 0201 codepoint?
 * @param c The character
 * @return Wether or not character is valid
 */
bool inline VirtualBoyPrivate::isJISX0201(unsigned char c){
	return (c >= ' ' && c <= '~') || (c > 0xA0 && c < 0xE0);
}

/**
 * Is character a valid Game ID character?
 * @param c The character
 * @return Wether or not character is valid
 */
bool inline VirtualBoyPrivate::isPublisherID(char c){
	// Valid characters:
	// - Uppercase letters
	// - Digits
	return (isupper(c) || isdigit(c));
}

/**
 * Is character a valid Game ID character?
 * @param c The character
 * @return Wether or not character is valid
 */
bool inline VirtualBoyPrivate::isGameID(char c){
	// Valid characters:
	// - Uppercase letters
	// - Digits
	// - Space (' ')
	// - Hyphen ('-')
	return (isupper(c) || isdigit(c) || c == ' ' || c == '-');
}

/** VirtualBoy **/

/**
 * Read a Virtual Boy ROM image.
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
	: super(new VirtualBoyPrivate(this, file))
{
	RP_D(VirtualBoy);
	d->className = "VirtualBoy";

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Seek to the beginning of the header.
	const int64_t filesize = d->file->size();
	// File must be at least 0x220 bytes,
	// and cannot be larger than 16 MB.
	if (filesize < 0x220 || filesize > (16*1024*1024)) {
		// File size is out of range.
		return;
	}

	// Read the ROM header.
	const unsigned int header_addr = (unsigned int)(filesize - 0x220);
	d->file->seek(header_addr);
	size_t size = d->file->read(&d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader))
		return;

	// Make sure this is actually a Virtual Boy ROM.
	DetectInfo info;
	info.header.addr = header_addr;
	info.header.size = sizeof(d->romHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->romHeader);
	info.ext = nullptr;	// Not needed for Virtual Boy.
	info.szFile = filesize;
	d->isValid = (isRomSupported(&info) >= 0);
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
	if (!info || !info->header.pData)
		return -1;

	// File size constraints:
	// - Must be at least 16 KB.
	// - Cannot be larger than 16 MB.
	// - Must be a power of two.
	// NOTE: The only retail ROMs were 512 KB, 1 MB, and 2 MB,
	// but the system supports up to 16 MB, and some homebrew
	// is less than 512 KB.
	if (info->szFile < 16*1024 ||
	    info->szFile > 16*1024*1024 ||
	    ((info->szFile & (info->szFile - 1)) != 0))
	{
		// File size is not valid.
		return -1;
	}

	// Virtual Boy header is located at
	// 0x220 before the end of the file.
	if (info->szFile < 0x220)
		return -1;
	const uint32_t header_addr_expected = (uint32_t)(info->szFile - 0x220);
	if (info->header.addr > header_addr_expected)
		return -1;
	else if (info->header.addr + info->header.size < header_addr_expected + 0x20)
		return -1;

	// Determine the offset.
	const unsigned int offset = header_addr_expected - info->header.addr;

	// Get the ROM header.
	const VB_RomHeader *romHeader =
		reinterpret_cast<const VB_RomHeader*>(&info->header.pData[offset]);

	// NOTE: The following is true for every Virtual Boy ROM:
	// 1) First 20 bytes of title are non-control JIS X 0201 characters (padded with space if needed)
	// 2) 21th byte is a null
	// 3) Game ID is either VxxJ (for Japan) or VxxE (for USA) (xx are alphanumeric uppercase characters)
	// 4) ROM version is always 0, but let's not count on that.
	// 5) And, obviously, the publisher is always valid, but again let's not rely on this
	// NOTE: We're supporting all no-intro ROMs except for "Space Pinball (Unknown) (Proto).vb" as it doesn't have a valid header at all
	if (romHeader->title[20] != 0) {
		// title[20] is not NULL.
		return -1;
	}

	// Make sure the title is valid JIS X 0201.
	for (int i = ARRAY_SIZE(romHeader->title)-2-1; i >= 0; i--) {
		if (!VirtualBoyPrivate::isJISX0201(romHeader->title[i])) {
			// Invalid title character.
			return -1;
		}
	}

	// NOTE: Game ID is VxxJ or VxxE for retail ROMs,
	// but homebrew ROMs can have anything here.
	// Valid characters:
	// - Uppercase letters
	// - Digits
	// - Space (' ') [not for publisher]
	// - Hyphen ('-') [not for publisher]
	if (!VirtualBoyPrivate::isPublisherID(romHeader->publisher[0]) ||
	    !VirtualBoyPrivate::isPublisherID(romHeader->publisher[1]))
	{
		// Invalid publisher ID.
		return -1;
	}

	for (int i = ARRAY_SIZE(romHeader->gameid)-1; i >= 0; i--) {
		if (!VirtualBoyPrivate::isGameID(romHeader->gameid[i])) {
			// Invalid game ID.
			return -1;
		}
	}

	// Looks like a Virtual Boy ROM.
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @return System name, or nullptr if not supported.
 */
const char *VirtualBoy::systemName(unsigned int type) const
{
	RP_D(const VirtualBoy);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	static_assert(SYSNAME_TYPE_MASK == 3,
		"VirtualBoy::systemName() array index optimization needs to be updated.");
	
	static const char *const sysNames[4] = {
		"Nintendo Virtual Boy", "Virtual Boy", "VB", nullptr,
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
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *VirtualBoy::supportedFileExtensions_static(void)
{
	// NOTE: These extensions may cause conflicts on
	// Windows if fallback handling isn't working.
	static const char *const exts[] = {
		".vb",	// Visual Basic .NET source files

		nullptr
	};
	return exts;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int VirtualBoy::loadFieldData(void)
{
	RP_D(VirtualBoy);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Virtual Boy ROM header, excluding the vector table.
	const VB_RomHeader *const romHeader = &d->romHeader;
	d->fields->reserve(5);	// Maximum of 5 fields.

	// Title
	d->fields->addField_string(C_("VirtualBoy", "Title"),
		cp1252_sjis_to_utf8(romHeader->title, sizeof(romHeader->title)));

	// Game ID and publisher.
	string id6(romHeader->gameid, sizeof(romHeader->gameid));
	id6.append(romHeader->publisher, sizeof(romHeader->publisher));
	d->fields->addField_string(C_("VirtualBoy", "Game ID"),
		latin1_to_utf8(id6.data(), (int)id6.size()));

	// Look up the publisher.
	const char *const publisher = NintendoPublishers::lookup(romHeader->publisher);
	string s_publisher;
	if (publisher) {
		s_publisher = publisher;
	} else {
		if (isalnum(romHeader->publisher[0]) &&
		    isalnum(romHeader->publisher[1]))
		{
			s_publisher = rp_sprintf(C_("VirtualBoy", "Unknown (%.2s)"),
				romHeader->publisher);
		} else {
			s_publisher = rp_sprintf(C_("VirtualBoy", "Unknown (%02X %02X)"),
				(uint8_t)romHeader->publisher[0],
				(uint8_t)romHeader->publisher[1]);
		}
	}
	d->fields->addField_string(C_("VirtualBoy", "Publisher"), s_publisher);

	// Revision
	d->fields->addField_string_numeric(C_("VirtualBoy", "Revision"),
		romHeader->version, RomFields::FB_DEC, 2);

	// Region
	const char *region;
	switch (romHeader->gameid[3]) {
		case 'J':
			region = C_("Region", "Japan");
			break;
		case 'E':
			region = C_("Region", "USA");
			break;
		default:
			region = C_("VirtualBoy", "Unknown");
			break;
	}
	d->fields->addField_string(C_("VirtualBoy", "Region"), region);

	return (int)d->fields->count();
}

}
