/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ColecoVision.cpp: ColecoVisoin ROM reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ColecoVision.hpp"
#include "colecovision_structs.h"

#include "ctypex.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::string;

namespace LibRomData {

class ColecoVisionPrivate final : public RomDataPrivate
{
public:
	ColecoVisionPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(ColecoVisionPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// ROM header
	ColecoVision_ROMHeader romHeader;

public:
	/**
	 * Get the title from the ROM header.
	 * @param pOutYear	[out,opt] Release year, if set; otherwise, -1.
	 * @return Title screen lines, or empty string on error.
	 */
	string getTitle(int *pOutYear = nullptr) const;
};

ROMDATA_IMPL(ColecoVision)

/** ColecoVisionPrivate **/

/* RomDataInfo */
const char *const ColecoVisionPrivate::exts[] = {
	".col",

	nullptr
};
const char *const ColecoVisionPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-colecovision-rom",

	nullptr
};
const RomDataInfo ColecoVisionPrivate::romDataInfo = {
	"ColecoVision", exts, mimeTypes
};

ColecoVisionPrivate::ColecoVisionPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/**
 * Get the title from the ROM header.
 * @param pOutYear	[out,opt] Release year, if set; otherwise, -1.
 * @return Title screen lines, or empty string on error.
 */
string ColecoVisionPrivate::getTitle(int *pOutYear) const
{
	static const uint8_t magic_has_logo[2] = {0xAA, 0x55};
	if (memcmp(romHeader.magic, magic_has_logo, sizeof(romHeader.magic)) != 0) {
		// Not the correct magic. No title.
		if (pOutYear) {
			*pOutYear = -1;
		}
		return {};
	}

	// Title has multiple lines, slash-separated.
	// "\x1E\x1F" == trademark symbol; replace it with '™'.
	// TODO: Up to three lines:
	// - Line 1: Copyright (usually)
	// - Line 2: Game name (usually)
	// - Line 3: Release year (only has digits) [FIXME: Limit to 4 chars]
	// On the startup screen, it's displayed in the following order:
	// - Line 2: Game name (usually)
	// - Line 1: Copyright (usually)
	// - "©[release year] Coleco"
	// Note that some games don't always use Line 1 and 2 exactly as described.

	// TODO: Return an "onscreen text" string (remove empty lines) and,
	// if present, a copyright year.
	string lines[2];
	lines[0].reserve(32);
	lines[1].reserve(64+1);

	// Game name should be ASCII, not Latin-1 or cp1252.
	// Any bytes with the high bit set will be stripped for now.
	// TODO: Check the rest of the ColecoVision system ROM font.
	unsigned int line = 0;
	const char *p = romHeader.game_name;
	const char *const p_end = &romHeader.game_name[ARRAY_SIZE(romHeader.game_name)];
	for (; line < 2 && p < p_end; p++) {
		const char chr = *p;
		switch (chr) {
			case '\x00':
				// Skip NULL bytes.
				break;
			case '/':
				// Next line
				line++;
				break;
			case '\x1D':
				lines[line] += "©";
				break;
			case '\x1E':
				// The next character should be '\x1F'.
				if (p + 1 < p_end && p[1] == '\x1F') {
					lines[line] += "™";
				}
				break;
			case '\x1F':
				// '\x1F' is ignored by itself.
				break;
			case ' ':
				// Skip leading spaces.
				if (!lines[line].empty())
					lines[line] += ' ';
				break;
			default:
				if (!(chr & 0x80)) {
					lines[line] += chr;
				}
				break;
		}
	}

	if (pOutYear && (p + 4 <= p_end)) {
		// Check if the next four characters are a release year.
		int year = 0;
		for (unsigned int i = 0; i < 4; i++, p++) {
			if (!ISDIGIT(*p)) {
				// Not a digit.
				year = -1;
				break;
			}

			year *= 10;
			year += (*p - '0');
		}

		*pOutYear = year;
	}

	// Trim the lines.
	for (auto &p : lines) {
		while (!p.empty() && ISSPACE(p[p.size()-1])) {
			p.resize(p.size()-1);
		}
	}

	// Combine the lines.
	// NOTE: Second line is used as the top line.
	if (lines[1].empty() && !lines[0].empty()) {
		return lines[0];
	} else if (!lines[1].empty() && !lines[0].empty()) {
		lines[1] += '\n';
		lines[1] += lines[0];
	}

	return lines[1];
}

/** ColecoVision **/

/**
 * Read a ColecoVision ROM image.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
ColecoVision::ColecoVision(const IRpFilePtr &file)
	: super(new ColecoVisionPrivate(file))
{
	RP_D(ColecoVision);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ROM header.
	d->file->rewind();
	size_t size = d->file->read(&d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader)) {
		// Seek and/or read error.
		d->file.reset();
		return;
	}

	// Check if this ROM image is supported.
	const char *const filename = file->filename();
	const DetectInfo info = {
		{0, sizeof(d->romHeader), reinterpret_cast<const uint8_t*>(&d->romHeader)},
		FileSystem::file_ext(filename),	// ext
		0		// szFile (not needed for ColecoVision)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int ColecoVision::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->ext != nullptr);
	if (!info || !info->ext) {
		// Needs the file extension...
		return -1;
	}

	// File extension is required.
	if (info->ext[0] == '\0') {
		// Empty file extension...
		return -1;
	}

	// The ColecoVision ROM header doesn't have enough magic
	// to conclusively determine if it's a ColecoVision ROM,
	// so check the file extension.
	// TODO: Also check for AA55/55AA?
	for (const char *const *ext = ColecoVisionPrivate::exts;
	     *ext != nullptr; ext++)
	{
		if (!strcasecmp(info->ext, *ext)) {
			// File extension is supported.
			return 0;
		}
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *ColecoVision::systemName(unsigned int type) const
{
	RP_D(const ColecoVision);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// ColecoVision has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"N64::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"ColecoVision", "ColecoVision", "CV", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int ColecoVision::loadFieldData(void)
{
	RP_D(ColecoVision);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	const ColecoVision_ROMHeader *const romHeader = &d->romHeader;
	d->fields.reserve(4);	// Maximum of 4 fields.

	// Title
	int year = -1;
	const string title = d->getTitle(&year);
	if (!title.empty()) {
		d->fields.addField_string(C_("RomData", "Title"), title);
	}

	// Copyright year
	if (year >= 0) {
		d->fields.addField_string_numeric(C_("ColecoVision", "Copyright Year"), year);
	}

	// TODO: Various table addresses?

	// Entry point
	d->fields.addField_string_numeric(C_("ColecoVision", "Entry Point"),
		le16_to_cpu(romHeader->entry_point), RomFields::Base::Hex, 4,
		RomFields::STRF_MONOSPACE);

	// Interrupt vectors use Z80 assembly.
	// We'll just decode absolute addresses and RETI.
	// Anything else will be shown as a hexdump.
	// TODO: Split into a separate function?
	static const uint8_t Z80_RETI[2] = {0xED, 0x4D};

	// IRQ vector
	const char *s_title = C_("ColecoVision", "IRQ Vector");
	if (romHeader->irq_int_vect[0] == 0xC3) {
		// JP nnnn
		uint16_t addr = romHeader->irq_int_vect[1] | (romHeader->irq_int_vect[2] << 8);
		d->fields.addField_string_numeric(s_title, addr, RomFields::Base::Hex, 4,
			RomFields::STRF_MONOSPACE);
	} else if (!memcmp(romHeader->irq_int_vect, Z80_RETI, 2)) {
		// RETI
		d->fields.addField_string(s_title, "RETI");
	} else {
		// Something else
		d->fields.addField_string_hexdump(s_title, romHeader->irq_int_vect, sizeof(romHeader->irq_int_vect));
	}

	// NMI vector
	s_title = C_("ColecoVision", "NMI Vector");
	if (romHeader->nmi_int_vect[0] == 0xC3) {
		// JP nnnn
		uint16_t addr = romHeader->nmi_int_vect[1] | (romHeader->nmi_int_vect[2] << 8);
		d->fields.addField_string_numeric(s_title, addr, RomFields::Base::Hex, 4,
			RomFields::STRF_MONOSPACE);
	} else if (!memcmp(romHeader->nmi_int_vect, Z80_RETI, 2)) {
		// RETI
		d->fields.addField_string(s_title, "RETI");
	} else {
		// Something else
		d->fields.addField_string_hexdump(s_title, romHeader->nmi_int_vect, sizeof(romHeader->nmi_int_vect));
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int ColecoVision::loadMetaData(void)
{
	RP_D(ColecoVision);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(2);	// Maximum of 2 metadata properties.

	//const ColecoVision_ROMHeader *const romHeader = &d->romHeader;

	// Title
	int year = -1;
	const string title = d->getTitle(&year);
	if (!title.empty()) {
		d->metaData->addMetaData_string(Property::Title, title);
	}

	// Release year (actually copyright year)
	if (year >= 0) {
		d->metaData->addMetaData_uint(Property::ReleaseYear, static_cast<unsigned int>(year));
	}

	// Finished reading the metadata.
	return (d->metaData ? static_cast<int>(d->metaData->count()) : -ENOENT);
}

}
