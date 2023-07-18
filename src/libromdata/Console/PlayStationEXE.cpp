/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PlayStationEXE.cpp: PlayStation PS-X executable reader.                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "PlayStationEXE.hpp"
#include "ps1_exe_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;
using LibRpFile::IRpFile;

// C++ STL classes.
using std::string;

namespace LibRomData {

class PlayStationEXEPrivate final : public RomDataPrivate
{
	public:
		PlayStationEXEPrivate(IRpFile *file, uint32_t sp_override);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(PlayStationEXEPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// PS-X EXE header.
		// NOTE: **NOT** byteswapped.
		PS1_EXE_Header psxHeader;

		// Stack pointer override
		uint32_t sp_override;
};

ROMDATA_IMPL(PlayStationEXE)

/** PlayStationEXEPrivate **/

/* RomDataInfo */
const char *const PlayStationEXEPrivate::exts[] = {
	".exe",	// NOTE: Conflicts with Windows executables.

	nullptr
};
const char *const PlayStationEXEPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-ps1-executable",

	nullptr
};
const RomDataInfo PlayStationEXEPrivate::romDataInfo = {
	"PlayStationEXE", exts, mimeTypes
};

PlayStationEXEPrivate::PlayStationEXEPrivate(IRpFile *file, uint32_t sp_override)
	: super(file, &romDataInfo)
	, sp_override(sp_override)
{
	// Clear the structs.
	memset(&psxHeader, 0, sizeof(psxHeader));
}

/** PlayStationEXE **/

/**
 * Read a PlayStation PS-X executable file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open PS-X executable file.
 */
PlayStationEXE::PlayStationEXE(IRpFile *file)
	: super(new PlayStationEXEPrivate(file, 0))
{
	// This class handles executables.
	RP_D(PlayStationEXE);
	d->mimeType = "application/x-ps1-executable";	// unofficial, not on fd.o
	d->fileType = FileType::Executable;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	init();
}

/**
 * Read a PlayStation PS-X executable file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open PS-X executable file.
 * @param sp_override Stack pointer override.
 */
PlayStationEXE::PlayStationEXE(IRpFile *file, uint32_t sp_override)
	: super(new PlayStationEXEPrivate(file, sp_override))
{
	// This class handles executables.
	RP_D(PlayStationEXE);
	d->mimeType = "application/x-ps1-executable";	// unofficial, not on fd.o
	d->fileType = FileType::Executable;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	init();
}

/**
 * Common initialization function for the constructors.
 */
void PlayStationEXE::init(void)
{
	RP_D(PlayStationEXE);

	// Read the PS-X EXE header.
	d->file->rewind();
	size_t size = d->file->read(&d->psxHeader, sizeof(d->psxHeader));
	if (size != sizeof(d->psxHeader)) {
		d->psxHeader.magic[0] = 0;
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(d->psxHeader), reinterpret_cast<const uint8_t*>(&d->psxHeader)},
		nullptr,	// ext (not needed for PlayStationEXE)
		0		// szFile (not needed for PlayStationEXE)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->psxHeader.magic[0] = 0;
		UNREF_AND_NULL_NOCHK(d->file);
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int PlayStationEXE::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(PS1_EXE_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for PS-X.
	const PS1_EXE_Header *const psxHeader =
		reinterpret_cast<const PS1_EXE_Header*>(info->header.pData);
	if (!memcmp(psxHeader->magic, PS1_EXE_MAGIC, sizeof(psxHeader->magic))) {
		// We have a PS-X executable.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *PlayStationEXE::systemName(unsigned int type) const
{
	RP_D(const PlayStationEXE);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Xbox 360 has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"PlayStationEXE::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Sony PlayStation", "PlayStation", "PS1", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int PlayStationEXE::loadFieldData(void)
{
	RP_D(PlayStationEXE);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// XBE file isn't valid.
		return -EIO;
	}

	// Parse the PS-X executable.
	const PS1_EXE_Header *const psxHeader = &d->psxHeader;

	d->fields.reserve(6);	// Maximum of 6 fields.
	d->fields.setTabName(0, "PS1 EXE");

	// RAM Address
	d->fields.addField_string_numeric(C_("PlayStationEXE", "RAM Address"),
		le32_to_cpu(psxHeader->ram_addr), RomFields::Base::Hex, 8,
		RomFields::STRF_MONOSPACE);

	// Initial PC
	d->fields.addField_string_numeric(C_("PlayStationEXE", "Initial PC"),
		le32_to_cpu(psxHeader->initial_pc), RomFields::Base::Hex, 8,
		RomFields::STRF_MONOSPACE);

	// Initial GP
	d->fields.addField_string_numeric(C_("PlayStationEXE", "Initial GP"),
		le32_to_cpu(psxHeader->initial_gp), RomFields::Base::Hex, 8,
		RomFields::STRF_MONOSPACE);

	// Initial SP
	const uint32_t initial_sp = (d->sp_override != 0 ? d->sp_override : le32_to_cpu(psxHeader->initial_sp));
	d->fields.addField_string_numeric(C_("PlayStationEXE", "Initial SP/FP"),
		initial_sp, RomFields::Base::Hex, 8,
		RomFields::STRF_MONOSPACE);

	// Initial SP offset
	d->fields.addField_string_numeric(C_("PlayStationEXE", "Initial SP Offset"),
		le32_to_cpu(psxHeader->initial_sp_off), RomFields::Base::Hex, 8,
		RomFields::STRF_MONOSPACE);

	// Region
	// To avoid shenanigans, we'll do a 16-bit XOR of the first part.
	const char *s_region = nullptr;
	const uint16_t *pu16 = reinterpret_cast<const uint16_t*>(psxHeader->region_id);
	const uint16_t *const pu16_end = pu16 + (36/2);
	uint16_t xor_result = 0;
	for (; pu16 < pu16_end; pu16++) {
		xor_result ^= le16_to_cpu(*pu16);
	}
	if (xor_result == 0x693C && psxHeader->region_id[36] == ' ') {
		// Check the region name.
		if (!strncmp(&psxHeader->region_id[37], "North America area", 19)) {
			s_region = C_("Region", "North America");
		} else if (!strncmp(&psxHeader->region_id[37], "Japan area", 11)) {
			s_region = C_("Region", "Japan");
		} else if (!strncmp(&psxHeader->region_id[37], "Europe area", 12)) {
			s_region = C_("Region", "Europe");
		}
	}
	const char *const s_region_title = C_("RomData", "Region");
	if (s_region) {
		d->fields.addField_string(s_region_title, s_region);
	} else {
		d->fields.addField_string(s_region_title,
			latin1_to_utf8(psxHeader->region_id, sizeof(psxHeader->region_id)));
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

}
