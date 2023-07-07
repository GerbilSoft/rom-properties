/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Amiibo.cpp: Nintendo amiibo NFC dump reader.                            *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Amiibo.hpp"
#include "nfp_structs.h"
#include "data/AmiiboData.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;
using LibRpFile::IRpFile;

// C++ STL classes
using std::string;
using std::vector;

namespace LibRomData {

class AmiiboPrivate final : public RomDataPrivate
{
	public:
		AmiiboPrivate(IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(AmiiboPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// NFC data.
		// TODO: Use nfpSize to determine an "nfpType" value?
		int nfpSize;		// NFP_File_Size
		NFP_Data_t nfpData;

		/**
		 * Verify the check bytes in an NTAG215 serial number.
		 * @param serial	[in] NTAG215 serial number. (9 bytes)
		 * @return True if the serial number has valid check bytes; false if not.
		 */
		static bool verifyCheckBytes(const uint8_t *serial);
};

ROMDATA_IMPL(Amiibo)
ROMDATA_IMPL_IMG(Amiibo)

/** AmiiboPrivate **/

/* RomDataInfo */
const char *const AmiiboPrivate::exts[] = {
	// NOTE: These extensions may cause conflicts on
	// Windows if fallback handling isn't working.
	".bin",	// too generic

	// NOTE: The following extensions are listed
	// for testing purposes on Windows, and may
	// be removed later.
	".nfc", ".nfp",

	nullptr
};
const char *const AmiiboPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-nintendo-amiibo",

	nullptr
};
const RomDataInfo AmiiboPrivate::romDataInfo = {
	"Amiibo", exts, mimeTypes
};

AmiiboPrivate::AmiiboPrivate(IRpFile *file)
	: super(file, &romDataInfo)
	, nfpSize(0)
{
	// Clear the NFP data struct.
	memset(&nfpData, 0, sizeof(nfpData));
}

/**
 * Verify the check bytes in an NTAG215 serial number.
 * @param serial	[in] NTAG215 serial number. (9 bytes)
 * @param pCb0		[out] Check byte 0. (calculated)
 * @param pCb1		[out] Check byte 1. (calculated)
 * @return True if the serial number has valid check bytes; false if not.
 */
bool AmiiboPrivate::verifyCheckBytes(const uint8_t *serial)
{
	// Check Byte 0 = CT ^ SN0 ^ SN1 ^ SN2
	// Check Byte 1 = SN3 ^ SN4 ^ SN5 ^ SN6
	// NTAG215 uses Cascade Level 2, so CT = 0x88.
	const uint8_t cb0 = 0x88 ^ serial[0] ^ serial[1] ^ serial[2];
	const uint8_t cb1 = serial[4] ^ serial[5] ^ serial[6] ^ serial[7];
	return (cb0 == serial[3] && cb1 == serial[8]);
}

/** Amiibo **/

/**
 * Read a Nintendo amiibo NFC dump.
 *
 * An NFC dump must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the NFC dump.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open NFC dump.
 */
Amiibo::Amiibo(IRpFile *file)
	: super(new AmiiboPrivate(file))
{
	// This class handles NFC dumps.
	RP_D(Amiibo);
	d->mimeType = "application/x-nintendo-amiibo";	// unofficial, not on fd.o
	d->fileType = FileType::NFC_Dump;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the NFC data.
	d->file->rewind();
	size_t size = d->file->read(&d->nfpData, sizeof(d->nfpData));
	switch (size) {
		case NFP_FILE_NO_PW:	// Missing password bytes.
			// Zero out the password bytes.
			memset(d->nfpData.pwd, 0, sizeof(d->nfpData.pwd));
			memset(d->nfpData.pack, 0, sizeof(d->nfpData.pack));
			memset(d->nfpData.rfui, 0, sizeof(d->nfpData.rfui));

			// fall-through
		case NFP_FILE_STANDARD:	// Standard dump.
			// Zero out the extended dump section.
			memset(d->nfpData.extended, 0, sizeof(d->nfpData.extended));

			// fall-through
		case NFP_FILE_EXTENDED:	// Extended dump.
			// Size is valid.
			d->nfpSize = static_cast<int>(size);
			break;

		default:
			// Unsupported file size.
			UNREF_AND_NULL_NOCHK(d->file);
			return;
	}

	// Check if the NFC data is supported.
	const DetectInfo info = {
		{0, sizeof(d->nfpData), reinterpret_cast<const uint8_t*>(&d->nfpData)},
		nullptr,	// ext (not needed for Amiibo)
		d->file->size()	// szFile
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		UNREF_AND_NULL_NOCHK(d->file);
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Amiibo::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0)
	{
		// Either no detection information was specified,
		// the header is too small, or the file is the
		// wrong size.
		return -1;
	}

	// Check the file size.
	// Three file sizes are possible.
	switch (info->szFile) {
		case NFP_FILE_NO_PW:	// Missing password bytes.
		case NFP_FILE_STANDARD:	// Standard dump.
		case NFP_FILE_EXTENDED:	// Extended dump.
			if (info->header.size < NFP_FILE_NO_PW) {
				// Not enough data is available.
				return -1;
			}
			break;

		default:
			// Unsupported file size.
			return -1;
	}

	const NFP_Data_t *const nfpData = reinterpret_cast<const NFP_Data_t*>(info->header.pData);

	// UID must start with 0x04.
	if (nfpData->serial[0] != 0x04) {
		// Invalid UID.
		return -1;
	}

	// Validate the UID check bytes.
	if (!AmiiboPrivate::verifyCheckBytes(nfpData->serial)) {
		// Check bytes are invalid.
		// These are read-only, so something went wrong
		// when the tag was being dumped.

		// NOTE: Some Super Nintendo World power-up bands, e.g.
		// the Gold Mario Power-Up Band, have incorrect check bytes.
		// Not sure why.
		const uint32_t char_id = be32_to_cpu(nfpData->char_id);
		if ((char_id & 0xFF) != NFP_TYPE_BAND) {
			return -1;
		}
	}

	// Check the "must match" values.
	static const uint8_t lock_footer[3] = {0x01, 0x00, 0x0F};
	static_assert(sizeof(nfpData->lock_footer) == sizeof(lock_footer)+1, "lock_footer is the wrong size.");

	if (nfpData->lock_header != cpu_to_be16(NFP_LOCK_HEADER) ||
	    nfpData->cap_container != cpu_to_be32(NFP_CAP_CONTAINER) ||
	    memcmp(nfpData->lock_footer, lock_footer, sizeof(lock_footer)) != 0 ||
	    nfpData->cfg0 != cpu_to_be32(NFP_CFG0) ||
	    nfpData->cfg1 != cpu_to_be32(NFP_CFG1))
	{
		// Not an amiibo.
		return -1;
	}

	// Low byte of amiibo_id must be 0x02.
	if (nfpData->amiibo_id.u8[3] != 0x02) {
		// Incorrect amiibo ID.
		return -1;
	}

	// This is an amiibo.
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *Amiibo::systemName(unsigned int type) const
{
	RP_D(const Amiibo);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// The "correct" name is "Nintendo Figurine Platform".
	// It's unknown whether or not Nintendo will release
	// NFC-enabled figurines that aren't amiibo.

	// NFP has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Amiibo::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Nintendo Figurine Platform",
		"Nintendo Figurine Platform",
		"NFP",
		nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Amiibo::supportedImageTypes_static(void)
{
	return IMGBF_EXT_MEDIA;
}

/**
 * Get a list of all available image sizes for the specified image type.
 *
 * The first item in the returned vector is the "default" size.
 * If the width/height is 0, then an image exists, but the size is unknown.
 *
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> Amiibo::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_EXT_MEDIA) {
		// Only media scans are supported.
		return vector<ImageSizeDef>();
	}

	// Amiibo scan sizes may vary, but there's always one.
	static const ImageSizeDef sz_EXT_MEDIA[] = {
		{nullptr, 0, 0, 0},
	};
	return vector<ImageSizeDef>(sz_EXT_MEDIA,
		sz_EXT_MEDIA + ARRAY_SIZE(sz_EXT_MEDIA));
}

/**
 * Get image processing flags.
 *
 * These specify post-processing operations for images,
 * e.g. applying transparency masks.
 *
 * @param imageType Image type.
 * @return Bitfield of ImageProcessingBF operations to perform.
 */
uint32_t Amiibo::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	// NOTE: amiibo.life's amiibo images have alpha transparency.
	// Hence, no image processing is required.
	return 0;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Amiibo::loadFieldData(void)
{
	RP_D(Amiibo);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// NTAG215 data
	d->fields.reserve(10);	// Maximum of 10 fields.

	// Serial number

	// Convert the 7-byte serial number to ASCII.
	static const char hex_lookup[16] = {
		'0','1','2','3','4','5','6','7',
		'8','9','A','B','C','D','E','F'
	};
	char buf[32]; char *pBuf = buf;
	for (unsigned int i = 0; i < 8; i++, pBuf += 2) {
		if (i == 3) {
			// Byte 3 is CB0.
			i++;
		}
		pBuf[0] = hex_lookup[d->nfpData.serial[i] >> 4];
		pBuf[1] = hex_lookup[d->nfpData.serial[i] & 0x0F];
	}
	*pBuf = 0;

	d->fields.addField_string(C_("Amiibo", "NTAG215 Serial"),
		latin1_to_utf8(buf, 7*2),
		RomFields::STRF_MONOSPACE);

	// NFP data
	const uint32_t char_id = be32_to_cpu(d->nfpData.char_id);
	const uint32_t amiibo_id = be32_to_cpu(d->nfpData.amiibo_id.u32);

	// tr: amiibo ID. Represents the character and amiibo series.
	// TODO: Link to https://amiibo.life/nfc/%08X-%08X
	d->fields.addField_string(C_("Amiibo", "amiibo ID"),
		rp_sprintf("%08X-%08X", char_id, amiibo_id),
		RomFields::STRF_MONOSPACE);

	// tr: amiibo type.
	static const char *const amiibo_type_tbl[4] = {
		// tr: NFP_TYPE_FIGURINE == standard amiibo
		NOP_C_("Amiibo|Type", "Figurine"),
		// tr: NFP_TYPE_CARD == amiibo card
		NOP_C_("Amiibo|Type", "Card"),
		// tr: NFP_TYPE_YARN == yarn amiibo
		NOP_C_("Amiibo|Type", "Yarn"),
		// tr: NFP_TYPE_BAND == Power-Up Band
		NOP_C_("Amiibo|Type", "Power-Up Band"),
	};
	const char *const amiibo_type_title = C_("Amiibo", "amiibo Type");
	if ((char_id & 0xFF) < ARRAY_SIZE(amiibo_type_tbl)) {
		d->fields.addField_string(amiibo_type_title,
			dpgettext_expr(RP_I18N_DOMAIN, "Amiibo|Type", amiibo_type_tbl[char_id & 0xFF]));
	} else {
		// Invalid amiibo type.
		d->fields.addField_string(amiibo_type_title,
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"), (char_id & 0xFF)));
	}

	// Get the AmiiboData instance.
	const AmiiboData *const pAmiiboData = AmiiboData::instance();

	// Character series
	const char *const char_series = pAmiiboData->lookup_char_series_name(char_id);
	d->fields.addField_string(C_("Amiibo", "Character Series"),
		char_series ? char_series : C_("RomData", "Unknown"));

	// Character name
	const char *const char_name = pAmiiboData->lookup_char_name(char_id);
	d->fields.addField_string(C_("Amiibo", "Character Name"),
		char_name ? char_name : C_("RomData", "Unknown"));

	// amiibo series
	const char *const amiibo_series = pAmiiboData->lookup_amiibo_series_name(amiibo_id);
	d->fields.addField_string(C_("Amiibo", "amiibo Series"),
		amiibo_series ? amiibo_series : C_("RomData", "Unknown"));

	// amiibo name, wave number, and release number.
	int wave_no, release_no;
	const char *const amiibo_name_title = C_("Amiibo", "amiibo Name");
	const char *const amiibo_name = pAmiiboData->lookup_amiibo_series_data(amiibo_id, &release_no, &wave_no);
	if (amiibo_name) {
		d->fields.addField_string(amiibo_name_title, amiibo_name);
		if (wave_no != 0) {
			d->fields.addField_string_numeric(C_("Amiibo", "amiibo Wave #"), wave_no);
		}
		if (release_no != 0) {
			d->fields.addField_string_numeric(C_("Amiibo", "amiibo Release #"), release_no);
		}
	} else {
		d->fields.addField_string(amiibo_name_title, C_("RomData", "Unknown"));
	}

	// tr: Credits for amiibo image downloads.
	const string credits = rp_sprintf(
		C_("Amiibo", "amiibo images provided by %s,\nthe Unofficial amiibo Database."),
		"<a href=\"https://amiibo.life/\">amiibo.life</a>");
	d->fields.addField_string(C_("Amiibo", "Credits"), credits, RomFields::STRF_CREDITS);

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
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
int Amiibo::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	// Only one size is available.
	RP_UNUSED(size);

	RP_D(const Amiibo);
	if (!d->isValid) {
		// Invalid file.
		return -EIO;
	}

	// Only the "media" scan is supported.
	// Note that "media" refers to a photo of
	// the figure and/or card.
	if (imageType != IMG_EXT_MEDIA) {
		// Unsupported image type.
		return -ENOENT;
	}

	// Only one URL.
	pExtURLs->resize(1);
	auto &extURL = pExtURLs->at(0);

	// Amiibo ID.
	char amiibo_id[20];
	snprintf(amiibo_id, sizeof(amiibo_id), "%08X-%08X",
		be32_to_cpu(d->nfpData.char_id), be32_to_cpu(d->nfpData.amiibo_id.u32));

	// Cache key. (amiibo ID)
	extURL.cache_key.reserve(32);
	extURL.cache_key = "amiibo/";
	extURL.cache_key += amiibo_id;
	extURL.cache_key += ".png";

	// URL.
	// Format: https://amiibo.life/nfc/[Page21]-[Page22]/image
	extURL.url.reserve(48);
	extURL.url = "https://amiibo.life/nfc/";
	extURL.url += amiibo_id;
	extURL.url += "/image";

	// Size may vary depending on amiibo.
	extURL.width = 0;
	extURL.height = 0;
	extURL.high_res = false;	// Only one size is available.

	// We're done here.
	return 0;
}

}
