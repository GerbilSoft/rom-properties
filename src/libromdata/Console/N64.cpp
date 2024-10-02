/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * N64.cpp: Nintendo 64 ROM image reader.                                  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "N64.hpp"
#include "n64_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;

namespace LibRomData {

class N64Private final : public RomDataPrivate
{
public:
	N64Private(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(N64Private)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// ROM image type
	enum class RomType {
		Unknown	= -1,

		Z64	= 0,	// Z64 format
		V64	= 1,	// V64 format
		SWAP2	= 2,	// swap2 format
		LE32	= 3,	// LE32 format

		Max
	};
	RomType romType;

public:
	// ROM header.
	// NOTE: Fields have been byteswapped in the constructor.
	N64_RomHeader romHeader;
};

ROMDATA_IMPL(N64)

/** N64Private **/

/* RomDataInfo */
const char *const N64Private::exts[] = {
	".z64", ".n64", ".v64",

	nullptr
};
const char *const N64Private::mimeTypes[] = {
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-n64-rom",

	nullptr
};
const RomDataInfo N64Private::romDataInfo = {
	"N64", exts, mimeTypes
};

N64Private::N64Private(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, romType(RomType::Unknown)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/** N64 **/

/**
 * Read a Nintendo 64 ROM image.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
N64::N64(const IRpFilePtr &file)
	: super(new N64Private(file))
{
	RP_D(N64);
	d->mimeType = "application/x-n64-rom";	// unofficial

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ROM image header.
	d->file->rewind();
	size_t size = d->file->read(&d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader)) {
		d->file.reset();
		return;
	}

	// Check if this ROM image is supported.
	const DetectInfo info = {
		{0, sizeof(d->romHeader), reinterpret_cast<const uint8_t*>(&d->romHeader)},
		nullptr,	// ext (not needed for N64)
		0		// szFile (not needed for N64)
	};
	d->romType = static_cast<N64Private::RomType>(isRomSupported_static(&info));

	switch (d->romType) {
		case N64Private::RomType::Z64:
			// Z64 format. Byteswapping will be done afterwards.
			break;

		case N64Private::RomType::V64:
			// V64 format. (16-bit byteswapped)
			// Convert the header to Z64 first.
			rp_byte_swap_16_array(d->romHeader.u16, sizeof(d->romHeader.u16));
			break;

		case N64Private::RomType::SWAP2:
			// swap2 format. (wordswapped)
			// Convert the header to Z64 first.
			#define UNSWAP2(x) (uint32_t)(((x) >> 16) | ((x) << 16))
			for (size_t i = 0; i < ARRAY_SIZE(d->romHeader.u32); i++) {
				d->romHeader.u32[i] = UNSWAP2(d->romHeader.u32[i]);
			}
			break;

		case N64Private::RomType::LE32:
			// LE32 format. (32-bit byteswapped)
			// Convert the header to Z64 first.
			// TODO: Optimize by not converting the non-text fields
			// if the host system is little-endian?
			// FIXME: Untested - ucon64 doesn't support it.
			rp_byte_swap_32_array(d->romHeader.u32, sizeof(d->romHeader.u32));
			break;

		default:
			// Unknown ROM type.
			d->romType = N64Private::RomType::Unknown;
			d->file.reset();
			return;
	}

	d->isValid = true;

	// Byteswap the header from Z64 format.
#if SYS_BYTEORDER != SYS_BIG_ENDIAN
	d->romHeader.init_pi	= be32_to_cpu(d->romHeader.init_pi);
	d->romHeader.clockrate	= be32_to_cpu(d->romHeader.clockrate);
	d->romHeader.entrypoint	= be32_to_cpu(d->romHeader.entrypoint);
	d->romHeader.crc[0]     = be32_to_cpu(d->romHeader.crc[0]);
	d->romHeader.crc[1]     = be32_to_cpu(d->romHeader.crc[1]);
#endif /* SYS_BYTEORDER != SYS_BIG_ENDIAN */
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int N64::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(N64_RomHeader))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(N64Private::RomType::Unknown);
	}

	const N64_RomHeader *const romHeader =
		reinterpret_cast<const N64_RomHeader*>(info->header.pData);

	// Check the magic number.
	// NOTE: This technically isn't a "magic number",
	// but it appears to be the same for all N64 ROMs.
	N64Private::RomType romType = N64Private::RomType::Unknown;
	if ((romHeader->magic64 & cpu_to_be64(N64_Z64_MAGIC_MASK)) == cpu_to_be64(N64_Z64_MAGIC)) {
		romType = N64Private::RomType::Z64;
	} else if ((romHeader->magic64 & cpu_to_be64(N64_V64_MAGIC_MASK)) == cpu_to_be64(N64_V64_MAGIC)) {
		romType = N64Private::RomType::V64;
	} else if ((romHeader->magic64 & cpu_to_be64(N64_SWAP2_MAGIC_MASK)) == cpu_to_be64(N64_SWAP2_MAGIC)) {
		romType = N64Private::RomType::SWAP2;
	} else if ((romHeader->magic64 & cpu_to_be64(N64_LE32_MAGIC_MASK)) == cpu_to_be64(N64_LE32_MAGIC)) {
		romType = N64Private::RomType::LE32;
	}

	return static_cast<int>(romType);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *N64::systemName(unsigned int type) const
{
	RP_D(const N64);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// N64 has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"N64::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"Nintendo 64", "Nintendo 64", "N64", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int N64::loadFieldData(void)
{
	RP_D(N64);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->romType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// snprintf() buffer.
	char buf[32];

	// ROM file header is read and byteswapped in the constructor.
	// TODO: Indicate the byteswapping format?
	const N64_RomHeader *const romHeader = &d->romHeader;
	d->fields.reserve(7);	// Maximum of 7 fields.

	// Title.
	// TODO: Space elimination.
	d->fields.addField_string(C_("RomData", "Title"),
		cp1252_sjis_to_utf8(romHeader->title, sizeof(romHeader->title)),
		RomFields::STRF_TRIM_END);

	// Game ID.
	// Replace any non-printable characters with underscores.
	char id4[5];
	for (int i = 0; i < 4; i++) {
		id4[i] = (ISPRINT(romHeader->id4[i])
			? romHeader->id4[i]
			: '_');
	}
	id4[4] = 0;
	d->fields.addField_string(C_("N64", "Game ID"),
		latin1_to_utf8(id4, 4));

	// Revision.
	d->fields.addField_string_numeric(C_("RomData", "Revision"),
		romHeader->revision, RomFields::Base::Dec, 2);

	// Entry point.
	d->fields.addField_string_numeric(C_("RomData", "Entry Point"),
		romHeader->entrypoint, RomFields::Base::Hex, 8, RomFields::STRF_MONOSPACE);

	// OS version.
	// TODO: ISALPHA(), or ISUPPER()?
	const char *const os_version_title = C_("RomData", "OS Version");
	if (romHeader->os_version[0] == 0x00 &&
	    romHeader->os_version[1] == 0x00 &&
	    ISALPHA(romHeader->os_version[3]))
	{
		snprintf(buf, sizeof(buf), "OS%u.%u%c",
			romHeader->os_version[2] / 10,
			romHeader->os_version[2] % 10,
			romHeader->os_version[3]);
		d->fields.addField_string(os_version_title, buf);
	} else {
		// Unrecognized Release field.
		d->fields.addField_string_hexdump(os_version_title,
			romHeader->os_version, sizeof(romHeader->os_version),
			RomFields::STRF_MONOSPACE);
	}

	// Clock rate.
	// NOTE: Lower 0xF is masked.
	const char *clockrate_title = C_("N64", "Clock Rate");
	const uint32_t clockrate = (romHeader->clockrate & ~0xFU);
	if (clockrate == 0) {
		d->fields.addField_string(clockrate_title,
			C_("N64|ClockRate", "0 (default)"));
	} else {
		d->fields.addField_string(clockrate_title,
			LibRpText::formatFrequency(clockrate));
	}

	// CRCs.
	snprintf(buf, sizeof(buf), "0x%08X 0x%08X",
		romHeader->crc[0], romHeader->crc[1]);
	d->fields.addField_string(C_("N64", "CRCs"),
		buf, RomFields::STRF_MONOSPACE);

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int N64::loadMetaData(void)
{
	RP_D(N64);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->romType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// ROM file header is read and byteswapped in the constructor.
	// TODO: Indicate the byteswapping format?
	const N64_RomHeader *const romHeader = &d->romHeader;

	// Title.
	// TODO: Space elimination.
	d->metaData->addMetaData_string(Property::Title,
		cp1252_sjis_to_utf8(romHeader->title, sizeof(romHeader->title)),
		RomMetaData::STRF_TRIM_END);

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

} // namespace LibRomData
