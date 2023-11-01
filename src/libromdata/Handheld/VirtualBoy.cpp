/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * VirtualBoy.cpp: Nintendo Virtual Boy ROM reader.                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "VirtualBoy.hpp"
#include "data/NintendoPublishers.hpp"
#include "vb_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::string;

namespace LibRomData {

class VirtualBoyPrivate final : public RomDataPrivate
{
public:
	VirtualBoyPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(VirtualBoyPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

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
	// ROM footer
	VB_RomFooter romFooter;
};

ROMDATA_IMPL(VirtualBoy)

/** VirtualBoyPrivate **/

/* RomDataInfo */
const char *const VirtualBoyPrivate::exts[] = {
	// NOTE: These extensions may cause conflicts on
	// Windows if fallback handling isn't working.

	".vb",	// Visual Basic .NET source files

	nullptr
};
const char *const VirtualBoyPrivate::mimeTypes[] = {
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-virtual-boy-rom",

	nullptr
};
const RomDataInfo VirtualBoyPrivate::romDataInfo = {
	"VirtualBoy", exts, mimeTypes
};

VirtualBoyPrivate::VirtualBoyPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the ROM footer struct.
	memset(&romFooter, 0, sizeof(romFooter));
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
	return (ISUPPER(c) || ISDIGIT(c));
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
	return (ISUPPER(c) || ISDIGIT(c) || c == ' ' || c == '-');
}

/** VirtualBoy **/

/**
 * Read a Virtual Boy ROM image.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM file.
 */
VirtualBoy::VirtualBoy(const IRpFilePtr &file)
	: super(new VirtualBoyPrivate(file))
{
	RP_D(VirtualBoy);
	d->mimeType = "application/x-virtual-boy-rom";	// unofficial

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Seek to the beginning of the footer.
	const off64_t fileSize = d->file->size();
	// File must be at least 0x220 bytes,
	// and cannot be larger than 16 MB.
	if (fileSize < 0x220 || fileSize > (16*1024*1024)) {
		// File size is out of range.
		d->file.reset();
		return;
	}

	// Read the ROM footer.
	const unsigned int footer_addr = static_cast<unsigned int>(fileSize - 0x220);
	d->file->seek(footer_addr);
	size_t size = d->file->read(&d->romFooter, sizeof(d->romFooter));
	if (size != sizeof(d->romFooter)) {
		d->file.reset();
		return;
	}

	// Make sure this is actually a Virtual Boy ROM.
	const DetectInfo info = {
		{footer_addr, sizeof(d->romFooter),
			reinterpret_cast<const uint8_t*>(&d->romFooter)},
		nullptr,	// ext (not needed for VirtualBoy)
		fileSize	// szFile
	};
	d->isValid = (isRomSupported(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
	}
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

	// Virtual Boy footer is located at
	// 0x220 before the end of the file.
	if (info->szFile < 0x220)
		return -1;
	const uint32_t footer_addr_expected = static_cast<uint32_t>(info->szFile - 0x220);
	if (info->header.addr > footer_addr_expected)
		return -1;
	else if (info->header.addr + info->header.size < footer_addr_expected + 0x20)
		return -1;

	// Determine the offset.
	const unsigned int offset = footer_addr_expected - info->header.addr;

	// Get the ROM footer.
	const VB_RomFooter *const romFooter =
		reinterpret_cast<const VB_RomFooter*>(&info->header.pData[offset]);

	// NOTE: The following is true for every Virtual Boy ROM:
	// 1) First 20 bytes of title are non-control JIS X 0201 characters (padded with space if needed)
	// 2) 21th byte is a null
	// 3) Game ID is either VxxJ (for Japan) or VxxE (for USA) (xx are alphanumeric uppercase characters)
	// 4) ROM version is always 0, but let's not count on that.
	// 5) And, obviously, the publisher is always valid, but again let's not rely on this
	// NOTE: We're supporting all no-intro ROMs except for "Space Pinball (Unknown) (Proto).vb" as it doesn't have a valid footer at all
	if (romFooter->title[20] != 0) {
		// title[20] is not NULL.
		return -1;
	}

	// Make sure the title is valid JIS X 0201.
	for (int i = ARRAY_SIZE(romFooter->title)-2-1; i >= 0; i--) {
		if (!VirtualBoyPrivate::isJISX0201(romFooter->title[i])) {
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
	if (!VirtualBoyPrivate::isPublisherID(romFooter->publisher[0]) ||
	    !VirtualBoyPrivate::isPublisherID(romFooter->publisher[1]))
	{
		// Invalid publisher ID.
		return -1;
	}

	for (int i = ARRAY_SIZE(romFooter->gameid)-1; i >= 0; i--) {
		if (!VirtualBoyPrivate::isGameID(romFooter->gameid[i])) {
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

	// VirtualBoy has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"VirtualBoy::systemName() array index optimization needs to be updated.");
	
	static const char *const sysNames[4] = {
		"Nintendo Virtual Boy", "Virtual Boy", "VB", nullptr,
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int VirtualBoy::loadFieldData(void)
{
	RP_D(VirtualBoy);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Virtual Boy ROM footer, excluding the vector table.
	const VB_RomFooter *const romFooter = &d->romFooter;
	d->fields.reserve(5);	// Maximum of 5 fields.

	// Title
	d->fields.addField_string(C_("RomData", "Title"),
		cp1252_sjis_to_utf8(romFooter->title, sizeof(romFooter->title)));

	// Game ID and publisher.
	string id6(romFooter->gameid, sizeof(romFooter->gameid));
	id6.append(romFooter->publisher, sizeof(romFooter->publisher));
	d->fields.addField_string(C_("RomData", "Game ID"), latin1_to_utf8(id6));

	// Look up the publisher.
	const char *const publisher = NintendoPublishers::lookup(romFooter->publisher);
	string s_publisher;
	if (publisher) {
		s_publisher = publisher;
	} else {
		if (ISALNUM(romFooter->publisher[0]) &&
		    ISALNUM(romFooter->publisher[1]))
		{
			s_publisher = rp_sprintf(C_("RomData", "Unknown (%.2s)"),
				romFooter->publisher);
		} else {
			s_publisher = rp_sprintf(C_("RomData", "Unknown (%02X %02X)"),
				static_cast<uint8_t>(romFooter->publisher[0]),
				static_cast<uint8_t>(romFooter->publisher[1]));
		}
	}
	d->fields.addField_string(C_("RomData", "Publisher"), s_publisher);

	// Revision
	d->fields.addField_string_numeric(C_("RomData", "Revision"),
		romFooter->version, RomFields::Base::Dec, 2);

	// Region
	const char *s_region;
	switch (romFooter->gameid[3]) {
		case 'J':
			s_region = C_("Region", "Japan");
			break;
		case 'E':
			s_region = C_("Region", "USA");
			break;
		default:
			s_region = nullptr;
			break;
	}
	if (s_region) {
		d->fields.addField_string(C_("RomData", "Region Code"), s_region);
	} else {
		d->fields.addField_string(C_("RomData", "Region Code"),
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"),
				static_cast<uint8_t>(romFooter->gameid[3])));
	}

	return static_cast<int>(d->fields.count());
}

}
