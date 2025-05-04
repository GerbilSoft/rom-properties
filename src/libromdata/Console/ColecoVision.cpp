/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUPackage.hpp: Wii U NUS Package reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ColecoVision.hpp"
#include "cv_structs.h"

#include "ctypex.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;

namespace LibRomData {

class ColecoVisionPrivate final : public RomDataPrivate
{
public:
	explicit ColecoVisionPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(ColecoVisionPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 1+1> mimeTypes;
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

	/**
	 * Disassemble an interrupt vector field from the ROM header.
	 * This field is added to our RomFields object.
	 * @param title		[in] Field title
	 * @param pc		[in] PC where this vector is located
	 * @param p_ivec	[in] Pointer to the interrupt vector field (must be 3 bytes)
	 */
	void addField_z80vec(const char *title, uint16_t pc, const uint8_t *p_ivec);
};

ROMDATA_IMPL(ColecoVision)

/** ColecoVisionPrivate **/

/* RomDataInfo */
const array<const char*, 1+1> ColecoVisionPrivate::exts = {{
	".col",

	nullptr
}};
const array<const char*, 1+1> ColecoVisionPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-colecovision-rom",

	nullptr
}};
const RomDataInfo ColecoVisionPrivate::romDataInfo = {
	"ColecoVision", exts.data(), mimeTypes.data()
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
	// ROM header needs to have the "Show Logo" magic in order for
	// the Title field to be valid.
	if (le16_to_cpu(romHeader.magic) != COLECOVISION_MAGIC_SHOW_LOGO) {
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
			if (!isdigit_ascii(*p)) {
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

/**
 * Disassemble an interrupt vector field from the ROM header.
 * This field is added to our RomFields object.
 * @param title		[in] Field title
 * @param pc		[in] PC where this vector is located
 * @param p_ivec	[in] Pointer to the interrupt vector field (must be 3 bytes)
 */
void ColecoVisionPrivate::addField_z80vec(const char *title, uint16_t pc, const uint8_t *p_ivec)
{
	// Quick and dirty Z80 disassembly suitable for the
	// interrupt vector fields.
	const char *no_params = nullptr;
	bool ok = false;

	switch (p_ivec[0]) {
		default:
			// Not supported...
			break;

		case 0x00:
			// NOP
			no_params = "NOP";
			ok = true;
			break;

		case 0x18:
		case 0x20: {
			// NOTE: Ignoring flags...
			// 0x18: JR dd
			// 0x20: JR NZ, dd
			uint16_t addr = pc + p_ivec[1];
			fields.addField_string_numeric(title, addr, RomFields::Base::Hex, 4,
				RomFields::STRF_MONOSPACE);
			ok = true;
			break;
		}

		case 0xC3: {
			// JP nnnn
			uint16_t addr = p_ivec[1] | (p_ivec[2] << 8);
			fields.addField_string_numeric(title, addr, RomFields::Base::Hex, 4,
				RomFields::STRF_MONOSPACE);
			ok = true;
			break;
		}

		case 0xC9:
			// RET
			no_params = "RET";
			ok = true;
			break;

		case 0xED:
			switch (p_ivec[1]) {
				default:
					// Not supported...
					break;
				case 0x45:
					// RETN
					no_params = "RETN";
					ok = true;
					break;
				case 0x4D:
					// RETI
					no_params = "RETI";
					ok = true;
					break;
			}
			break;

		case 0xFF:
			// RST 38h
			no_params = "RST 38h";
			ok = true;
			break;
	}

	if (no_params) {
		// No-parameter opcode
		fields.addField_string(title, no_params);
	} else if (!ok) {
		// Not supported...
		fields.addField_string_hexdump(title, p_ivec, 3);
	}
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
	assert(info->header.pData != nullptr);
	if (!info || !info->ext || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(ColecoVision_ROMHeader))
	{
		// Either no detection information was specified,
		// or the header is too small.
		// Also, a file extension is needed.
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
	for (const char *const *ext = ColecoVisionPrivate::exts.data();
	     *ext != nullptr; ext++)
	{
		if (!strcasecmp(info->ext, *ext)) {
			// File extension is supported.
			// Also check for a valid magic number.
			const ColecoVision_ROMHeader *const romHeader =
				reinterpret_cast<const ColecoVision_ROMHeader*>(info->header.pData);
			switch (le16_to_cpu(romHeader->magic)) {
				case COLECOVISION_MAGIC_SHOW_LOGO:
				case COLECOVISION_MAGIC_SKIP_LOGO:
				case COLECOVISION_MAGIC_BIOS:
				case COLECOVISION_MAGIC_MONITOR_TEST:
					// Magic number is valid.
					return 0;
				default:
					break;
			}
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
		"ColecoVision::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"ColecoVision", "ColecoVision", "CV", nullptr
	}};

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

	// IRQ vector
	d->addField_z80vec(C_("ColecoVision", "IRQ Vector"), 0x801E, romHeader->irq_int_vect);

	// NMI vector
	d->addField_z80vec(C_("ColecoVision", "IRQ Vector"), 0x8021, romHeader->nmi_int_vect);

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
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	//const ColecoVision_ROMHeader *const romHeader = &d->romHeader;
	d->metaData.reserve(2);	// Maximum of 2 metadata properties.

	// Title
	int year = -1;
	const string title = d->getTitle(&year);
	if (!title.empty()) {
		d->metaData.addMetaData_string(Property::Title, title);
	}

	// Release year (actually copyright year)
	if (year >= 0) {
		d->metaData.addMetaData_uint(Property::ReleaseYear, static_cast<unsigned int>(year));
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData.count());
}

} // namespace LibRomData
