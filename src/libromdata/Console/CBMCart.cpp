/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CBMCart.cpp: Commodore ROM cartridge reader.                            *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CBMCart.hpp"
#include "cbm_cart_structs.h"
#include "data/CBMData.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::string;
using std::unique_ptr;
using std::vector;

// zlib for crc32()
#include <zlib.h>
#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRomData {

#ifdef _MSC_VER
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#endif /* _MSC_VER */

class CBMCartPrivate final : public RomDataPrivate
{
public:
	CBMCartPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(CBMCartPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	enum class RomType {
		Unknown	= -1,

		C64	= 0,
		C128	= 1,
		CBM2	= 2,
		VIC20	= 3,
		Plus4	= 4,

		Max
	};
	RomType romType;

	// ROM header
	CBM_CRTHeader romHeader;

	// CRC32 of the first 16 KB of ROM data.
	// Used for the external image URL.
	// NOTE: Calculated by extURLs() on demand.
	uint32_t rom_16k_crc32;

public:
	/**
	 * Initialize zlib.
	 * @return 0 on success; non-zero on error.
	 */
	static int zlibInit(void);
};

ROMDATA_IMPL(CBMCart)
ROMDATA_IMPL_IMG(CBMCart)

/** CBMCartPrivate **/

/* RomDataInfo */
const char *const CBMCartPrivate::exts[] = {
	".crt",

	nullptr
};
const char *const CBMCartPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-c64-cartridge",
	"application/x-c128-cartridge",
	"application/x-cbm2-cartridge",
	"application/x-vic20-cartridge",
	"application/x-plus4-cartridge",

	nullptr
};
const RomDataInfo CBMCartPrivate::romDataInfo = {
	"CBMCart", exts, mimeTypes
};

CBMCartPrivate::CBMCartPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, romType(RomType::Unknown)
	, rom_16k_crc32(0)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/**
 * Initialize zlib.
 * @return 0 on success; non-zero on error.
 */
int CBMCartPrivate::zlibInit(void)
{
#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		// Can't calculate the CRC32.
		return -ENOENT;
	}
#else /* !defined(_MSC_VER) || !defined(ZLIB_IS_DLL) */
	// zlib isn't in a DLL, but we need to ensure that the
	// CRC table is initialized anyway.
	get_crc_table();
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// zlib initialized.
	return 0;
}

/** CBMCart **/

/**
 * Read a Commodore ROM cartridge image.
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
CBMCart::CBMCart(const IRpFilePtr &file)
	: super(new CBMCartPrivate(file))
{
	RP_D(CBMCart);

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
	const DetectInfo info = {
		{0, sizeof(d->romHeader), reinterpret_cast<const uint8_t*>(&d->romHeader)},
		nullptr,	// ext (not needed for CBMCart)
		0		// szFile (not needed for CBMCart)
	};
	d->romType = static_cast<CBMCartPrivate::RomType>(isRomSupported_static(&info));
	d->isValid = (d->romType > CBMCartPrivate::RomType::Unknown);

	if (!d->isValid) {
		d->file.reset();
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int CBMCart::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(CBM_CRTHeader))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(CBMCartPrivate::RomType::Unknown);
	}

	const CBM_CRTHeader *const romHeader =
		reinterpret_cast<const CBM_CRTHeader*>(info->header.pData);

	// Check the magic string.
	CBMCartPrivate::RomType romType = CBMCartPrivate::RomType::Unknown;
	if (!memcmp(romHeader->magic, CBM_C64_CRT_MAGIC, sizeof(romHeader->magic))) {
		romType = CBMCartPrivate::RomType::C64;
	} else if (!memcmp(romHeader->magic, CBM_C128_CRT_MAGIC, sizeof(romHeader->magic))) {
		romType = CBMCartPrivate::RomType::C128;
	} else if (!memcmp(romHeader->magic, CBM_CBM2_CRT_MAGIC, sizeof(romHeader->magic))) {
		romType = CBMCartPrivate::RomType::CBM2;
	} else if (!memcmp(romHeader->magic, CBM_VIC20_CRT_MAGIC, sizeof(romHeader->magic))) {
		romType = CBMCartPrivate::RomType::VIC20;
	} else if (!memcmp(romHeader->magic, CBM_PLUS4_CRT_MAGIC, sizeof(romHeader->magic))) {
		romType = CBMCartPrivate::RomType::Plus4;
	} else {
		// Not supported.
		return static_cast<int>(CBMCartPrivate::RomType::Unknown);
	}

	// CRT version number.
	const uint16_t version = be16_to_cpu(romHeader->version);

	/// Verify that certain features are not present in older versions.

	// Subtype requires v1.1.
	if (romHeader->subtype != 0) {
		if (version < 0x0101) {
			return static_cast<int>(CBMCartPrivate::RomType::Unknown);
		}
	}

	// Systems other than C64 require v2.0.
	if (romType > CBMCartPrivate::RomType::C64) {
		if (version < 0x0200) {
			return static_cast<int>(CBMCartPrivate::RomType::Unknown);
		}
	}

	// We're done here.
	return static_cast<int>(romType);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *CBMCart::systemName(unsigned int type) const
{
	RP_D(const CBMCart);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	static_assert(SYSNAME_TYPE_MASK == 3,
		"CBMCart::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[][4] = {
		{"Commodore 64", "C64", "C64", nullptr},
		{"Commodore 128", "C128", "C128", nullptr},
		{"Commodore CBM-II", "CBM-II", "CBM-II", nullptr},
		{"Commodore VIC-20", "VIC-20", "VIC-20", nullptr},
		{"Commodore Plus/4", "Plus/4", "Plus/4", nullptr},
	};
	static_assert(ARRAY_SIZE(sysNames) == static_cast<int>(CBMCartPrivate::RomType::Max),
		"CBMCart: sysNames[] is missing entries!");

	int i = static_cast<int>(d->romType);
	if (i < 0 || i >= ARRAY_SIZE_I(sysNames)) {
		// Invalid system ID. Default to C64.
		i = 0;
	}
	return sysNames[i][type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t CBMCart::supportedImageTypes_static(void)
{
	return IMGBF_EXT_TITLE_SCREEN;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> CBMCart::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			// FIXME: NTSC vs. PAL; proper rescaling.
			// Using VICE C64 NTSC image dimensions.
			return {{nullptr, 384, 247, 0}};
		default:
			break;
	}

	// Unsupported image type.
	return {};
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int CBMCart::loadFieldData(void)
{
	RP_D(CBMCart);
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

	// ROM header is read in the constructor.
	const CBM_CRTHeader *const romHeader = &d->romHeader;
	d->fields.reserve(2);	// Maximum of 2 fields.

	// Title
	if (romHeader->title[0] != '\0') {
		d->fields.addField_string(C_("RomData", "Title"),
			cp1252_to_utf8(romHeader->title, sizeof(romHeader->title)),
			RomFields::STRF_TRIM_END);
	}

	// Cartridge type
	const unsigned int type = be16_to_cpu(romHeader->type);
	bool b_noType = false;
	const char *s_type = nullptr;
	switch (d->romType) {
		case CBMCartPrivate::RomType::C64:
			switch (type) {
				case 0: {
					// Generic cartridge.
					// Identify the type based on the EXROM/GAME lines.
					const uint8_t id = (romHeader->c64_game  != 0) |
					                  ((romHeader->c64_exrom != 0) << 1);
					static const char crt_types_c64_generic[][16] = {
						"16 KB game", "8 KB game",
						"UltiMax mode", "RAM/disabled"
					};
					s_type = crt_types_c64_generic[id & 3];
					break;
				}
				case 36:
					s_type = (romHeader->subtype == 1 ? "Nordic Replay" : "Retro Replay");
					break;
				case 57:
					s_type = (romHeader->subtype == 1 ? "Hucky" : "RGCD");
					break;
				default:
					s_type = CBMData::lookup_C64_cart_type(type);
					break;
			}
			break;

		case CBMCartPrivate::RomType::C128:
			switch (type) {
				case 0:
					s_type = "generic cartridge";
					break;
				case 1:
					switch (romHeader->subtype) {
						default:
						case 0:
							s_type = "Warpspeed128";
							break;
						case 1:
							s_type = "Warpspeed128, REU support";
							break;
						case 2:
							s_type = "Warpspeed128, REU support, with I/O and ROM banking";
							break;
					}
					break;
				default:
					break;
			}
			break;

		case CBMCartPrivate::RomType::VIC20:
			s_type = CBMData::lookup_VIC20_cart_type(type);
			break;

		default:
			// Type is not supported for this platform.
			b_noType = true;
			break;
	}

	if (!b_noType) {
		const char *const s_type_title = C_("RomData", "Type");
		if (s_type != nullptr) {
			d->fields.addField_string(s_type_title, s_type);
		} else {
			d->fields.addField_string(s_type_title,
				rp_sprintf(C_("RomData", "Unknown (%u)"), type));
		}
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int CBMCart::loadMetaData(void)
{
	RP_D(CBMCart);
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
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// ROM header is read in the constructor.
	const CBM_CRTHeader *const romHeader = &d->romHeader;

	// Title
	if (romHeader->title[0] != '\0') {
		d->metaData->addMetaData_string(Property::Title,
			cp1252_to_utf8(romHeader->title, sizeof(romHeader->title)),
			RomMetaData::STRF_TRIM_END);
	}

	// Finished reading the metadata.
	return (d->metaData ? static_cast<int>(d->metaData->count()) : -ENOENT);
}

/**
 * Get a list of URLs for an external image type.
 *
 * A thumbnail size may be requested from the shell.
 * If the subclass supports multiple sizes, it should
 * try to get the size that most closely matches the
 * requested size.
 *
 * @param imageType	[in]     Image type.
 * @param pExtURLs	[out]    Output vector.
 * @param size		[in,opt] Requested image size. This may be a requested
 *                               thumbnail size in pixels, or an ImageSizeType
 *                               enum value.
 * @return 0 on success; negative POSIX error code on error.
 */
int CBMCart::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	RP_D(CBMCart);
	if (!d->isValid || (int)d->romType < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

	// System IDs.
	static const char sys_tbl[][8] = {
		"c64", "c128", "cbmII", "vic20", "plus4"
	};
	if (static_cast<int>(d->romType) >= ARRAY_SIZE_I(sys_tbl))
		return -ENOENT;
	const char *const sys = sys_tbl[(int)d->romType];

	// ROM header is read in the constructor.
	const CBM_CRTHeader *const romHeader = &d->romHeader;

	// Image URL is the CRC32 of the first 16 KB of actual ROM data
	// in the cartridge. If the cartridge has less than 16 KB ROM,
	// then it's the CRC32 of whatever's available.
	if (d->rom_16k_crc32 == 0) {
		// CRC32 hasn't been calculated yet.
		if (d->zlibInit() != 0) {
			// zlib could not be initialized.
			return -EIO;
		}

		if (!d->file || !d->file->isOpen()) {
			// File isn't open. Can't calculate the CRC32.
			return -EBADF;
		}

		// Read CHIP packets until we've read up to 16 KB of ROM data.
		size_t sz_rd_total = 0;
		const off64_t addr = static_cast<off64_t>(be32_to_cpu(romHeader->hdr_len));
		int ret = d->file->seek(addr);
		if (ret != 0) {
			// Seek error.
			int err = -d->file->lastError();
			if (err == 0) {
				err = -EIO;
			}
			return err;
		}

#define CBM_ROM_BUF_SIZ (16*1024)
		unique_ptr<uint8_t[]> buf(new uint8_t[CBM_ROM_BUF_SIZ]);
		while (sz_rd_total < CBM_ROM_BUF_SIZ) {
			CBM_CRT_CHIPHeader chipHeader;
			size_t sz_rd = d->file->read(&chipHeader, sizeof(chipHeader));
			if (sz_rd != sizeof(chipHeader)) {
				// Read error.
				break;
			}

			// Check the CHIP magic.
			if (chipHeader.magic != cpu_to_be32(CBM_CRT_CHIP_MAGIC)) {
				// Invalid magic.
				break;
			}

			// Determine how much data to read.
			uint16_t sz_to_read = be16_to_cpu(chipHeader.rom_size);
			if (sz_to_read == 0) {
				// No data... Bank is invalid.
				break;
			} else if (sz_rd_total + sz_to_read > CBM_ROM_BUF_SIZ) {
				// Too much data. Trim it.
				sz_to_read = static_cast<uint16_t>(CBM_ROM_BUF_SIZ - sz_rd_total);
			}

			// Read the data.
			sz_rd = d->file->read(&buf[sz_rd_total], sz_to_read);
			sz_rd_total += sz_rd;
			if (sz_rd != sz_to_read) {
				// Read error. We'll at least process whatever was read,
				// and then stop here.
				// "Fraction Fever (USA, Europe)" is 8,272 bytes, but the
				// CHIP header says 16 KB.
				break;
			}
		}

		if (sz_rd_total == 0) {
			// Unable to read *any* data.
			return -EIO;
		}

		// Calculate the CRC32 of whatever data we could read.
		d->rom_16k_crc32 = crc32(0, buf.get(), static_cast<uInt>(sz_rd_total));
	}

	// Lowercase hex CRC32s are used.
	char s_crc32[16];
	snprintf(s_crc32, sizeof(s_crc32), "%08x", d->rom_16k_crc32);

	// NOTE: We only have one size for CBMCart right now.
	// TODO: Determine the actual image size.
	RP_UNUSED(size);
	vector<ImageSizeDef> sizeDefs = supportedImageSizes(imageType);
	assert(sizeDefs.size() == 1);
	if (sizeDefs.empty()) {
		// No image sizes.
		return -ENOENT;
	}

	// NOTE: RPDB's title screen database only has one size.
	// There's no need to check image sizes, but we need to
	// get the image size for the extURLs struct.

	// Determine the image type name.
	const char *imageTypeName;
	const char *ext;
	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			imageTypeName = "title";
			ext = ".png";
			break;
		default:
			// Unsupported image type.
			return -ENOENT;
	}

	// FIXME: Use a better subdirectory scheme instead of just "crt" for cartridge?
	// NOTE: For C64 cartridges, using a second level subdirectory
	// for the cartridge type.
	char s_subdir[16];
	if (d->romType == CBMCartPrivate::RomType::C64) {
		// TODO: Separate dir for UltiMax?
		snprintf(s_subdir, sizeof(s_subdir), "crt/%u", be16_to_cpu(romHeader->type));
	} else {
		memcpy(s_subdir, "crt", 4);
	}

	// Add the URLs.
	pExtURLs->resize(1);
	auto extURL_iter = pExtURLs->begin();
	extURL_iter->url = d->getURL_RPDB(sys, imageTypeName, s_subdir, s_crc32, ext);
	extURL_iter->cache_key = d->getCacheKey_RPDB(sys, imageTypeName, s_subdir, s_crc32, ext);
	extURL_iter->width = sizeDefs[0].width;
	extURL_iter->height = sizeDefs[0].height;
	extURL_iter->high_res = (sizeDefs[0].index >= 2);

	// All URLs added.
	return 0;
}

}
