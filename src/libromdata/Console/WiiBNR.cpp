/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiBNR.cpp: Nintendo Wii banner reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WiiBNR.hpp"
#include "data/NintendoLanguage.hpp"

#include "WiiCommon.hpp"
#include "wii_banner.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;

// Uninitialized vector class
#include "uvector.h"

namespace LibRomData {

class WiiBNRPrivate final : public RomDataPrivate
{
public:
	explicit WiiBNRPrivate(const IRpFilePtr &file, uint32_t gcnRegion = ~0U, char id4_region = 'A');

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(WiiBNRPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// IMET struct
	// This contains all of the text data.
	Wii_IMET_t imet;

	// Region data to distinguish certain titles
	uint32_t gcnRegion;
	char id4_region;
};

ROMDATA_IMPL(WiiBNR)

/** WiiBNRPrivate **/

/* RomDataInfo */
// NOTE: This will be handled using the same
// settings as GameCube.
const array<const char*, 1+1> WiiBNRPrivate::exts = {{
	".bnr",

	nullptr
}};
const array<const char*, 1+1> WiiBNRPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-wii-bnr",	// .bnr

	nullptr
}};
const RomDataInfo WiiBNRPrivate::romDataInfo = {
	"GameCube", exts.data(), mimeTypes.data()
};

WiiBNRPrivate::WiiBNRPrivate(const IRpFilePtr &file, uint32_t gcnRegion, char id4_region)
	: super(file, &romDataInfo)
	, gcnRegion(gcnRegion)
	, id4_region(id4_region)
{}

/** WiiBNR **/

/**
 * Read a Nintendo Wii banner file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file		[in] Open banner file
 * @param gcnRegion	[in] GameCube region code from the boot block
 * @param id4_region	[in] ID4 region
 */
WiiBNR::WiiBNR(const LibRpFile::IRpFilePtr &file, uint32_t gcnRegion, char id4_region)
	: super(new WiiBNRPrivate(file, gcnRegion, id4_region))
{
	// This class handles banner files.
	// NOTE: This will be handled using the same
	// settings as GameCube.
	RP_D(WiiBNR);
	d->mimeType = "application/x-wii-bnr";	// unofficial, not on fd.o
	d->fileType = FileType::BannerFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Check the magic number.
	// This is usually located at one of the following offsets:
	// - 64: Retail games
	// - 128: Some homebrew
	unsigned int addr = 0;
	static constexpr array<uint8_t, 2> addrs = {{64, 128}};
	for (unsigned int p : addrs) {
		uint32_t bnr_magic;
		size_t size = d->file->seekAndRead(p, &bnr_magic, sizeof(bnr_magic));
		if (size != sizeof(bnr_magic)) {
			// Seek and/or read error.
			d->file.reset();
			return;
		}
		if (bnr_magic == cpu_to_be32(WII_IMET_MAGIC)) {
			// Found it!
			addr = p;
			break;
		}
	}
	if (addr == 0) {
		// Not found.
		d->file.reset();
		return;
	}

	// Load the full IMET data.
	// NOTE: Wii_IMET_t includes 64 zero bytes *before* the IMET data,
	// so we need to subtract 64 from addr.
	size_t size = d->file->seekAndRead(addr-64, &d->imet, sizeof(d->imet));
	if (size != sizeof(d->imet)) {
		// Seek and/or read error.
		d->file.reset();
		return;
	}

	d->isValid = true;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiBNR::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const uint32_t *const pData32 = reinterpret_cast<const uint32_t*>(info->header.pData);

	// Check the magic number.
	// This is usually located at one of the following offsets:
	// - 64: Retail games
	// - 128: Some homebrew
	static constexpr array<uint8_t, 2> addrs = {{64, 128}};
	for (unsigned int p : addrs) {
		// NOTE: Wii_IMET_t includes 64 zero bytes *before* the IMET data,
		// so we need to subtract 64 from p.
		if ((p + sizeof(Wii_IMET_t) - 64) >= info->header.size)
			break;

		if (pData32[p/sizeof(uint32_t)] == cpu_to_be32(WII_IMET_MAGIC)) {
			// Found the IMET magic number.
			return 0;
		}
	}

	// Not found...
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *WiiBNR::systemName(unsigned int type) const
{
	RP_D(const WiiBNR);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Wii has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"WiiBNR::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"Nintendo Wii", "Wii", "Wii", nullptr,
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int WiiBNR::loadFieldData(void)
{
	RP_D(WiiBNR);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// No file.
		// A closed file is OK, since we already loaded the IMET data.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown banner file type.
		return -EIO;
	}

	d->fields.reserve(1);	// Maximum of 1 field.

	// Using WiiCommon to get an RFT_STRING_MULTI field.
	RomFields::StringMultiMap_t *const pMap_bannerName = WiiCommon::getWiiBannerStrings(
		&d->imet, d->gcnRegion, d->id4_region);
	if (pMap_bannerName) {
		// Add the field.
		const uint32_t def_lc = NintendoLanguage::getWiiLanguageCode(
			NintendoLanguage::getWiiLanguage());
		d->fields.addField_string_multi(C_("GameCube", "Game Info"), pMap_bannerName, def_lc);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int WiiBNR::loadMetaData(void)
{
	RP_D(WiiBNR);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// No file.
		// A closed file is OK, since we already loaded the IMET data.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown banner file type.
		return -EIO;
	}

	d->metaData.reserve(1);	// Maximum of 1 metadata property.

	// Using WiiCommon to get an RFT_STRING field.
	d->metaData.addMetaData_string(Property::Description,
		WiiCommon::getWiiBannerStringForSysLC(
			&d->imet, d->gcnRegion, d->id4_region));

	// Finished reading the metadata.
	return static_cast<int>(d->metaData.count());
}

} // namespace LibRomData
