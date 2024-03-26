/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Intellivision.cpp: Intellivision ROM reader.                            *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Intellivision.hpp"
#include "intv_structs.h"

#include "ctypex.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::string;
using std::vector;

namespace LibRomData {

class IntellivisionPrivate final : public RomDataPrivate
{
public:
	IntellivisionPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(IntellivisionPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// ROM header
	Intellivision_ROMHeader romHeader;

public:
	/**
	 * Get the title from the ROM header.
	 * @param pOutYear	[out,opt] Release year, if set; otherwise, -1.
	 * @return Title screen lines, or empty string on error.
	 */
	string getTitle(int *pOutYear = nullptr) const;
};

ROMDATA_IMPL(Intellivision)

/** IntellivisionPrivate **/

/* RomDataInfo */
const char *const IntellivisionPrivate::exts[] = {
	".int", ".itv",

	//".bin",	// NOTE: Too generic...

	nullptr
};
const char *const IntellivisionPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-intellivision-rom",

	nullptr
};
const RomDataInfo IntellivisionPrivate::romDataInfo = {
	"Intellivision", exts, mimeTypes
};

IntellivisionPrivate::IntellivisionPrivate(const IRpFilePtr &file)
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
string IntellivisionPrivate::getTitle(int *pOutYear) const
{
	// NOTE: The cartridge ROM is mapped to 0x5000.

	// Title/date address must be between 0x5010 and 0x50FF.
	uint16_t title_addr = romHeader.title_date.get_real_value();
	if (title_addr < 0x5010 || title_addr >= (0x5000 + ARRAY_SIZE(romHeader.u16))) {
		// Out of range.
		return {};
	}
	// Convert to an absolute address.
	title_addr -= 0x5000;

	// First word has the year, minus 1900.
	// NOTE: ROMs that don't have a valid title/date field may have 0 (1900) here.
	// Some homebrew titles have weird values, e.g. 2 (1902) or 4 (1904), so
	// we'll allow any year as long as it's not 0 (1900).
	if (pOutYear) {
		unsigned int year_raw = be16_to_cpu(romHeader.u16[title_addr]);
		if (year_raw != 0) {
			*pOutYear = year_raw + 1900;
		}
	}

	// Title is a NULL-terminated ASCII string, but it's 16-bit words.
	// Convert it to 8-bit ASCII.
	// NOTE: Removing the high bit to ensure UTF-8 compatibility.
	// TODO: Verify the whole EXEC character set.
	// FIXME: 0x5E and 0x5F are arrows, similar to PETSCII.
	string title;
	title.reserve(32);
	const uint16_t *const p_end = &romHeader.u16[ARRAY_SIZE(romHeader.u16)];
	for (const uint16_t *p = &romHeader.u16[title_addr+1]; p < p_end; p++) {
		const uint16_t chr = *p;
		if (chr == 0) {
			break;
		}

		title += static_cast<char>(static_cast<uint8_t>(be16_to_cpu(chr)));
	}

	// Trim the title.
	// NOTE: Games that don't use EXEC don't necessarily have a valid title.
	// The title field is usually single space in that case, and we should
	// ignore it. (date is 0 aka 1900; maybe we should skip if that's the case?)
	while (!title.empty() && ISSPACE(title[title.size()-1])) {
		title.resize(title.size()-1);
	}

	return title;
}

/** Intellivision **/

/**
 * Read a Intellivision ROM image.
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
Intellivision::Intellivision(const IRpFilePtr &file)
	: super(new IntellivisionPrivate(file))
{
	RP_D(Intellivision);

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
		0		// szFile (not needed for Intellivision)
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
int Intellivision::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	if (!info || !info->ext) {
		// Needs the file extension...
		return -1;
	}

	// File extension is required.
	if (info->ext[0] == '\0') {
		// Empty file extension...
		return -1;
	}

	// The Intellivision ROM header doesn't have enough magic
	// to conclusively determine if it's a Intellivision ROM,
	// so check the file extension.
	for (const char *const *ext = IntellivisionPrivate::exts;
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
const char *Intellivision::systemName(unsigned int type) const
{
	RP_D(const Intellivision);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Intellivision has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Intellivision::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static constexpr const char *const sysNames[4] = {
		"Intellivision", "Intellivision", "INTV", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Intellivision::loadFieldData(void)
{
	RP_D(Intellivision);
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

	const Intellivision_ROMHeader *const romHeader = &d->romHeader;
	d->fields.reserve(3);	// Maximum of 3 fields.

	// Title
	int year = -1;
	const string title = d->getTitle(&year);
	if (!title.empty()) {
		d->fields.addField_string(C_("RomData", "Title"), title);
	}

	// Copyright year
	if (year >= 0) {
		d->fields.addField_string_numeric(C_("Intellivision", "Copyright Year"), year);
	}

	// Flags
	unsigned int flags = be16_to_cpu(romHeader->flags);

	// If both "Skip ECS" bits aren't set, clear both to prevent issues.
	if ((flags & INTV_SKIP_ECS) != INTV_SKIP_ECS) {
		flags &= ~INTV_SKIP_ECS;
	}

	static const char *const flags_bitfield_names[] = {
		// Bits 0-5: Keyclick bits (TODO)
		nullptr, nullptr, nullptr, nullptr, nullptr,

		// Bits 6-8
		NOP_C_("Intellivision|Flags", "Intellivision 2"),
		NOP_C_("Intellivision|Flags", "Run code after title string"),
		NOP_C_("Intellivision|Flags", "Skip ECS title screen"),
	};
	vector<string> *const v_flags_bitfield_names = RomFields::strArrayToVector_i18n(
		"Region", flags_bitfield_names, ARRAY_SIZE(flags_bitfield_names));
	d->fields.addField_bitfield(C_("Intellivision", "Flags"),
		v_flags_bitfield_names, 2, flags);

	// TODO: Entry point (differs if EXEC is used or not)

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int Intellivision::loadMetaData(void)
{
	RP_D(Intellivision);
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

	//const Intellivision_ROMHeader *const romHeader = &d->romHeader;

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
