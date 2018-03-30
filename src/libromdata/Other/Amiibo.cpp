/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Amiibo.cpp: Nintendo amiibo NFC dump reader.                            *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#include "Amiibo.hpp"
#include "librpbase/RomData_p.hpp"

#include "nfp_structs.h"
#include "data/AmiiboData.hpp"

// librpbase
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(Amiibo)
ROMDATA_IMPL_IMG(Amiibo)

class AmiiboPrivate : public RomDataPrivate
{
	public:
		AmiiboPrivate(Amiibo *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(AmiiboPrivate)

	public:
		// NFC data.
		// TODO: Use nfpSize to determine an "nfpType" value?
		int nfpSize;		// NFP_File_Size
		NFP_Data_t nfpData;

		/**
		 * Calculate the check bytes from an NTAG215 serial number.
		 * @param serial	[in] NTAG215 serial number. (9 bytes)
		 * @param pCb0		[out] Check byte 0. (calculated)
		 * @param pCb1		[out] Check byte 1. (calculated)
		 * @return True if the serial number has valid check bytes; false if not.
		 */
		static bool calcCheckBytes(const uint8_t *serial, uint8_t *pCb0, uint8_t *pCb1);
};

/** AmiiboPrivate **/

AmiiboPrivate::AmiiboPrivate(Amiibo *q, IRpFile *file)
	: super(q, file)
	, nfpSize(0)
{
	// Clear the NFP data struct.
	memset(&nfpData, 0, sizeof(nfpData));
}

/**
 * Calculate the check bytes from an NTAG215 serial number.
 * @param serial	[in] NTAG215 serial number. (9 bytes)
 * @param pCb0		[out] Check byte 0. (calculated)
 * @param pCb1		[out] Check byte 1. (calculated)
 * @return True if the serial number has valid check bytes; false if not.
 */
bool AmiiboPrivate::calcCheckBytes(const uint8_t *serial, uint8_t *pCb0, uint8_t *pCb1)
{
	// Check Byte 0 = CT ^ SN0 ^ SN1 ^ SN2
	// Check Byte 1 = SN3 ^ SN4 ^ SN5 ^ SN6
	// NTAG215 uses Cascade Level 2, so CT = 0x88.
	*pCb0 = 0x88 ^ serial[0] ^ serial[1] ^ serial[2];
	*pCb1 = serial[4] ^ serial[5] ^ serial[6] ^ serial[7];
	return (*pCb0 == serial[3] && *pCb1 == serial[8]);
}

/** Amiibo **/

/**
 * Read a Nintendo amiibo NFC dump.
 *
 * An NFC dump must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the NFC dump.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open NFC dump.
 */
Amiibo::Amiibo(IRpFile *file)
	: super(new AmiiboPrivate(this, file))
{
	// This class handles NFC dumps.
	RP_D(Amiibo);
	d->className = "Amiibo";
	d->fileType = FTYPE_NFC_DUMP;

	if (!d->file) {
		// Could not dup() the file handle.
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
			d->nfpSize = (int)size;
			break;

		default:
			// Unsupported file size.
			return;
	}

	// Check if the NFC data is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->nfpData);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->nfpData);
	info.ext = nullptr;	// Not needed for NFP.
	info.szFile = d->file->size();
	d->isValid = (isRomSupported_static(&info) >= 0);
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

	const NFP_Data_t *nfpData = reinterpret_cast<const NFP_Data_t*>(info->header.pData);

	// UID must start with 0x04.
	if (nfpData->serial[0] != 0x04) {
		// Invalid UID.
		return -1;
	}

	// Validate the UID check bytes.
	uint8_t cb0, cb1;
	if (!AmiiboPrivate::calcCheckBytes(nfpData->serial, &cb0, &cb1)) {
		// Check bytes are invalid.
		// These are read-only, so something went wrong
		// when the tag was being dumped.
		return -1;
	}

	// Check the "must match" values.
	static const uint8_t lock_header[2] = {0x0F, 0xE0};
	static const uint8_t cap_container[4] = {0xF1, 0x10, 0xFF, 0xEE};
	static const uint8_t lock_footer[3] = {0x01, 0x00, 0x0F};
	static const uint8_t cfg0[4] = {0x00, 0x00, 0x00, 0x04};
	static const uint8_t cfg1[4] = {0x5F, 0x00, 0x00, 0x00};

	static_assert(sizeof(nfpData->lock_header)   == sizeof(lock_header),   "lock_header is the wrong size.");
	static_assert(sizeof(nfpData->cap_container) == sizeof(cap_container), "cap_container is the wrong size.");
	static_assert(sizeof(nfpData->lock_footer)   == sizeof(lock_footer)+1, "lock_footer is the wrong size.");
	static_assert(sizeof(nfpData->cfg0)          == sizeof(cfg0),          "cfg0 is the wrong size.");
	static_assert(sizeof(nfpData->cfg1)          == sizeof(cfg1),          "cfg1 is the wrong size.");

	if (memcmp(nfpData->lock_header,   lock_header,   sizeof(lock_header)) != 0 ||
	    memcmp(nfpData->cap_container, cap_container, sizeof(cap_container)) != 0 ||
	    memcmp(nfpData->lock_footer,   lock_footer,   sizeof(lock_footer)) != 0 ||
	    memcmp(nfpData->cfg0,          cfg0,          sizeof(cfg0)) != 0 ||
	    memcmp(nfpData->cfg1,          cfg1,          sizeof(cfg1)) != 0)
	{
		// Not an amiibo.
		return -1;
	}

	// Low byte of amiibo_id must be 0x02.
	if ((be32_to_cpu(nfpData->amiibo_id) & 0xFF) != 0x02) {
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
const char *const *Amiibo::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		// NOTE: These extensions may cause conflicts on
		// Windows if fallback handling isn't working.
		".bin",	// too generic

		// NOTE: The following extensions are listed
		// for testing purposes on Windows, and may
		// be removed later.
		".nfc", ".nfp",

		nullptr
	};
	return exts;
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
std::vector<RomData::ImageSizeDef> Amiibo::supportedImageSizes_static(ImageType imageType)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return std::vector<ImageSizeDef>();
	}

	if (imageType != IMG_EXT_MEDIA) {
		// Only media scans are supported.
		return std::vector<ImageSizeDef>();
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
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}

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
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// NTAG215 data.
	d->fields->reserve(10);	// Maximum of 10 fields.

	// Serial number.

	// Convert the 7-byte serial number to ASCII.
	static const char hex_lookup[16] = {
		'0','1','2','3','4','5','6','7',
		'8','9','A','B','C','D','E','F'
	};
	char buf[64]; char *pBuf = buf;
	for (int i = 0; i < 8; i++, pBuf += 2) {
		if (i == 3) {
			// Byte 3 is CB0.
			i++;
		}
		pBuf[0] = hex_lookup[d->nfpData.serial[i] >> 4];
		pBuf[1] = hex_lookup[d->nfpData.serial[i] & 0x0F];
	}

	// Verify the check bytes.
	// TODO: Show calculated check bytes?
	uint8_t cb0, cb1;
	int len;
	if (d->calcCheckBytes(d->nfpData.serial, &cb0, &cb1)) {
		// Check bytes are valid.
		len = snprintf(pBuf, sizeof(buf) - (7*2), " (check: %02X %02X)",
			d->nfpData.serial[3], d->nfpData.serial[8]);
	} else {
		// Check bytes are NOT valid.
		// NOTE: Shouldn't show up anymore because invalid
		// serial numbers are discarded in isRomSupported_static().
		len = snprintf(pBuf, sizeof(buf) - (7*2), " (check ERR: %02X %02X)",
			d->nfpData.serial[3], d->nfpData.serial[8]);
	}

	len += (7*2);
	if (len > (int)sizeof(buf))
		len = (int)sizeof(buf);
	d->fields->addField_string(C_("Amiibo", "NTAG215 Serial"),
		len > 0 ? latin1_to_utf8(buf, len) : "",
		RomFields::STRF_MONOSPACE);

	// NFP data.
	const uint32_t char_id = be32_to_cpu(d->nfpData.char_id);
	const uint32_t amiibo_id = be32_to_cpu(d->nfpData.amiibo_id);

	// tr: amiibo ID. Represents the character and amiibo series.
	// TODO: Link to https://amiibo.life/nfc/%08X-%08X
	d->fields->addField_string(C_("Amiibo", "amiibo ID"),
		rp_sprintf("%08X-%08X", char_id, amiibo_id),
		RomFields::STRF_MONOSPACE);

	// tr: amiibo type.
	static const char *const amiibo_type_tbl[3] = {
		// tr: NFP_TYPE_FIGURINE == standard amiibo
		NOP_C_("Amiibo|Type", "Figurine"),
		// tr: NFP_TYPE_CARD == amiibo card
		NOP_C_("Amiibo|Type", "Card"),
		// tr: NFP_TYPE_YARN == yarn amiibo
		NOP_C_("Amiibo|Type", "Yarn"),
	};
	if ((char_id & 0xFF) < ARRAY_SIZE(amiibo_type_tbl)) {
		d->fields->addField_string(C_("Amiibo", "amiibo Type"),
			dpgettext_expr(RP_I18N_DOMAIN, "Amiibo|Type", amiibo_type_tbl[char_id & 0xFF]));
	} else {
		// Invalid amiibo type.
		d->fields->addField_string(C_("Amiibo", "amiibo Type"),
			rp_sprintf(C_("Amiibo", "Unknown (0x%02X)"), (char_id & 0xFF)));
	}

	// Character series.
	const char *const char_series = AmiiboData::lookup_char_series_name(char_id);
	d->fields->addField_string(C_("Amiibo", "Character Series"),
		char_series ? char_series : C_("Amiibo", "Unknown"));

	// Character name.
	const char *const char_name = AmiiboData::lookup_char_name(char_id);
	d->fields->addField_string(C_("Amiibo", "Character Name"),
		char_name ? char_name : C_("Amiibo", "Unknown"));

	// amiibo series.
	const char *const amiibo_series = AmiiboData::lookup_amiibo_series_name(amiibo_id);
	d->fields->addField_string(C_("Amiibo", "amiibo Series"),
		amiibo_series ? amiibo_series : C_("Amiibo", "Unknown"));

	// amiibo name, wave number, and release number.
	int wave_no, release_no;
	const char *const amiibo_name = AmiiboData::lookup_amiibo_series_data(amiibo_id, &release_no, &wave_no);
	if (amiibo_name) {
		d->fields->addField_string(C_("Amiibo", "amiibo Name"), amiibo_name);
		if (wave_no != 0) {
			d->fields->addField_string_numeric(C_("Amiibo", "amiibo Wave #"), wave_no);
		}
		if (release_no != 0) {
			d->fields->addField_string_numeric(C_("Amiibo", "amiibo Release #"), release_no);
		}
	}

	// tr: Credits for amiibo image downloads.
	const string credits = rp_sprintf(
		C_("Amiibo", "amiibo images provided by %s,\nthe Unofficial amiibo Database."),
		"<a href=\"http://amiibo.life/\">amiibo.life</a>");
	d->fields->addField_string(C_("Amiibo", "Credits"), credits, RomFields::STRF_CREDITS);

	// Finished reading the field data.
	return (int)d->fields->count();
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
int Amiibo::extURLs(ImageType imageType, std::vector<ExtURL> *pExtURLs, int size) const
{
	assert(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_EXT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}
	assert(pExtURLs != nullptr);
	if (!pExtURLs) {
		// No vector.
		return -EINVAL;
	}
	pExtURLs->clear();

	// Only one size is available.
	RP_UNUSED(size);

	RP_D(Amiibo);
	if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
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
	const string amiibo_id = rp_sprintf("%08X-%08X",
		be32_to_cpu(d->nfpData.char_id), be32_to_cpu(d->nfpData.amiibo_id));

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
