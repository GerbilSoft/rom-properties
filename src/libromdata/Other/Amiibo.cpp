/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Amiibo.cpp: Nintendo amiibo NFC dump reader.                            *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "Amiibo.hpp"
#include "RomData_p.hpp"

#include "nfp_structs.h"
#include "data/AmiiboData.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

class AmiiboPrivate final : public RomDataPrivate
{
public:
	explicit AmiiboPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
public:
	RP_DISABLE_COPY(AmiiboPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 3+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// amiibo type
	enum class AmiiboType {
		Unknown	= -1,

		NTAG215		= 0,	// NTAG215 (original amiibo aka v2)
		NT3H2111	= 1,	// NT3H2111 (Super Nintendo World bands)
		NT3H2211	= 2,	// NT3H2211 (v3)

		Max
	};
	AmiiboType amiiboType;

	// NFP file size.
	// Can identify the tag type and/or if it's truncated.
	uint32_t nfpSize;

	// NFC data
	union {
		NFP_NTAG215_Data_t ntag215;
		NFP_NT3H2111_Data_t nt3h2111;
		uint8_t u8[NFP_MAX_SIZE];
	} nfpData;

	/**
	 * Verify the check bytes in an NTAG215 unique ID.
	 * @param uid	[in] NTAG215 unique ID (9 bytes)
	 * @return True if the unique ID has valid check bytes; false if not.
	 */
	static bool verifyNTAG215CheckBytes(const uint8_t *uid);
};

ROMDATA_IMPL(Amiibo)
ROMDATA_IMPL_IMG(Amiibo)

/** AmiiboPrivate **/

/* RomDataInfo */
const array<const char*, 3+1> AmiiboPrivate::exts = {{
	// NOTE: These extensions may cause conflicts on
	// Windows if fallback handling isn't working.
	".bin",	// too generic

	// NOTE: The following extensions are listed
	// for testing purposes on Windows, and may
	// be removed later.
	".nfc", ".nfp",

	nullptr
}};
const array<const char*, 1+1> AmiiboPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-nintendo-amiibo",

	nullptr
}};
const RomDataInfo AmiiboPrivate::romDataInfo = {
	"Amiibo", exts.data(), mimeTypes.data()
};

AmiiboPrivate::AmiiboPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, nfpSize(0)
{
	// Clear the NFP data struct.
	memset(&nfpData, 0, sizeof(nfpData));
}

/**
 * Verify the check bytes in an NTAG215 unique ID.
 * @param uid	[in] NTAG215 unique ID (9 bytes)
 * @return True if the unique ID number has valid check bytes; false if not.
 */
bool AmiiboPrivate::verifyNTAG215CheckBytes(const uint8_t *uid)
{
	// Check Byte 0 (BCC0) = CT ^ SN0 ^ SN1 ^ SN2
	// Check Byte 1 (BCC1) = SN3 ^ SN4 ^ SN5 ^ SN6
	// NTAG215 uses Cascade Level 2, so CT = 0x88.
	const uint8_t bcc0 = 0x88 ^ uid[0] ^ uid[1] ^ uid[2];
	const uint8_t bcc1 = uid[4] ^ uid[5] ^ uid[6] ^ uid[7];
	return (bcc0 == uid[3] && bcc1 == uid[8]);
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
Amiibo::Amiibo(const IRpFilePtr &file)
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
	d->nfpSize = static_cast<uint32_t>(size);

	// Check if the NFC data is supported.
	const DetectInfo info = {
		{0, d->nfpSize, reinterpret_cast<const uint8_t*>(&d->nfpData)},
		nullptr,	// ext (not needed for Amiibo)
		d->file->size()	// szFile
	};
	d->amiiboType = static_cast<AmiiboPrivate::AmiiboType>(isRomSupported_static(&info));
	d->isValid = (static_cast<int>(d->amiiboType) >= 0);

	if (!d->isValid) {
		d->file.reset();
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
		return static_cast<int>(AmiiboPrivate::AmiiboType::Unknown);
	}

	// Check the file size.
	switch (info->szFile) {
		case NFP_FILE_NO_PW:	// Missing password bytes.

		case NFP_FILE_STANDARD:	// Standard dump
		case NFP_FILE_EXTENDED:	// Extended dump

		case NFP_SNW_STANDARD:	// Super Nintendo World band (standard)
		case NFP_SNW_EXTENDED:	// Super Nintendo World band (extended)

		case NFP_V3_STANDARD:	// amiibo v3
		case NFP_V3_EXTENDED:	// amiibo v3
			if (info->header.size < NFP_FILE_NO_PW) {
				// Not enough data is available.
				return static_cast<int>(AmiiboPrivate::AmiiboType::Unknown);
			}
			break;

		default:
			// Unsupported file size.
			return static_cast<int>(AmiiboPrivate::AmiiboType::Unknown);
	}

	// Check if this is a valid NTAG215.
	const NFP_NTAG215_Data_t *const pNtag215 =
		reinterpret_cast<const NFP_NTAG215_Data_t*>(info->header.pData);

	do {
		// UID must start with 0x04.
		if (pNtag215->uid[0] != 0x04) {
			// Invalid UID.
			break;
		}

		// Validate the UID check bytes.
		if (!AmiiboPrivate::verifyNTAG215CheckBytes(pNtag215->uid)) {
			// Check bytes are invalid.
			// These are read-only, so something went wrong
			// when the tag was being dumped.
			break;
		}

		// Check the "must match" values.
		static constexpr array<uint8_t, 3> lock_footer = {{0x01, 0x00, 0x0F}};
		static_assert(sizeof(pNtag215->lock_footer) == sizeof(lock_footer)+1, "lock_footer is the wrong size.");

		if (pNtag215->lock_header != cpu_to_be16(NFP_NTAG215_LOCK_HEADER) ||
		    pNtag215->cap_container != cpu_to_be32(NFP_NTAG215_CAP_CONTAINER) ||
		    memcmp(pNtag215->lock_footer, lock_footer.data(), lock_footer.size()) != 0 ||
		    pNtag215->cfg0 != cpu_to_be32(NFP_NTAG215_CFG0) ||
		    pNtag215->cfg1 != cpu_to_be32(NFP_NTAG215_CFG1))
		{
			// Not an NTAG215 amiibo.
			break;
		}

		// Low byte of amiibo_id indicates the version.
		// It should be 0x02 for NTAG215.
		if (pNtag215->user_data.amiibo_id.u8[3] == 0x02) {
			// This is an NTAG215.
			return static_cast<int>(AmiiboPrivate::AmiiboType::NTAG215);
		}
	} while (0);

	// Not a valid NTAG215.
	// Check if it's a valid NT3H2111.
	const NFP_NT3H2111_Data_t *const pNt3H2111 =
		reinterpret_cast<const NFP_NT3H2111_Data_t*>(info->header.pData);

	do {
		// UID must start with 0x04.
		if (pNt3H2111->uid[0] != 0x04) {
			// Invalid UID.
			break;
		}

		// Check the lock header and cap container.
		// NOTE: These are the same values as NTAG215.
		if (pNt3H2111->lock_header != cpu_to_be16(NFP_NT3H2111_LOCK_HEADER) ||
		    pNt3H2111->cap_container != cpu_to_be32(NFP_NT3H2111_CAP_CONTAINER))
		{
			// Not an NT3H2111 amiibo.
			break;
		}

		// If the file is large enough, check the footer values.
		if (info->header.size >= NFP_SNW_STANDARD) {
			// TODO: Check rfu_pwd?
			if (pNt3H2111->cfg0 != cpu_to_be32(NFP_NT3H2111_CFG0) ||
			    pNt3H2111->cfg1 != cpu_to_be32(NFP_NT3H2111_CFG1))
			{
				// Not an NT3H2111 amiibo.
				break;
			}
		}

		// Low byte of amiibo_id indicates the version.
		// It should be 0x02 for NT3H2111, or 0x03 for NT3H2211.
		switch (pNt3H2111->user_data.amiibo_id.u8[3]) {
			case 0x02:
				// NT3H2111
				return static_cast<int>(AmiiboPrivate::AmiiboType::NT3H2111);
			case 0x03:
				// NT3H2211
				return static_cast<int>(AmiiboPrivate::AmiiboType::NT3H2211);
			default:
				// Not valid...
				break;
		}
	} while (0);

	// Not a supported amiibo NFC tag...
	return static_cast<int>(AmiiboPrivate::AmiiboType::Unknown);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *Amiibo::systemName(unsigned int type) const
{
	RP_D(const Amiibo);
	if (!d->isValid || !isSystemNameTypeValid(type)) {
		return nullptr;
	}

	// The "correct" name is "Nintendo Figurine Platform".
	// It's unknown whether or not Nintendo will release
	// NFC-enabled figurines that aren't amiibo.

	// NFP has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Amiibo::systemName() array index optimization needs to be updated.");

	static const array<const char*, 4> sysNames = {{
		"Nintendo Figurine Platform", "Nintendo Figurine Platform", "NFP", nullptr
	}};

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
		return {};
	}

	// Amiibo scan sizes may vary, but there's always one.
	return {{nullptr, 0, 0, 0}};
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
		// amiibo file isn't valid.
		return -EIO;
	}

	// NTAG215 data
	d->fields.reserve(10);	// Maximum of 10 fields.

	// Unique ID
	static constexpr array<char, 16> hex_lookup = {{
		'0','1','2','3','4','5','6','7',
		'8','9','A','B','C','D','E','F'
	}};

	// Convert the 7-byte unique ID number to ASCII.
	char buf[16]; char *pBuf = buf;
	// FIXME: Need to make a separate "type" field because some NT3H2111/NT3H2211
	// dumps are truncated to NTAG215 sizes. (Check the fixed fields to distinguish
	// them, since there's no check digits in the unique ID.)
	if (d->amiiboType == AmiiboPrivate::AmiiboType::NTAG215) {
		// NTAG215: Need to skip BCC0.
		for (unsigned int i = 0; i < 8; i++, pBuf += 2) {
			if (i == 3) {
				// Byte 3 is BCC0.
				i++;
			}
			pBuf[0] = hex_lookup[d->nfpData.ntag215.uid[i] >> 4];
			pBuf[1] = hex_lookup[d->nfpData.ntag215.uid[i] & 0x0F];
		}
	} else {
		// NT3H2111/NT3H2211: No check bytes to skip.
		for (unsigned int i = 0; i < sizeof(d->nfpData.nt3h2111.uid); i++, pBuf += 2) {
			pBuf[0] = hex_lookup[d->nfpData.nt3h2111.uid[i] >> 4];
			pBuf[1] = hex_lookup[d->nfpData.nt3h2111.uid[i] & 0x0F];
		}
	}
	*pBuf = 0;

	d->fields.addField_string(C_("Amiibo", "Unique ID"),
		latin1_to_utf8(buf, 7*2),
		RomFields::STRF_MONOSPACE);

	// NFP data
	const uint32_t char_id = be32_to_cpu(d->nfpData.ntag215.user_data.char_id);
	const uint32_t amiibo_id = be32_to_cpu(d->nfpData.ntag215.user_data.amiibo_id.u32);

	// tr: amiibo ID. Represents the character and amiibo series.
	// TODO: Link to https://amiibo.life/nfc/{:0>8X}-{:0>8X}
	d->fields.addField_string(C_("Amiibo", "amiibo ID"),
		fmt::format(FSTR("{:0>8X}-{:0>8X}"), char_id, amiibo_id),
		RomFields::STRF_MONOSPACE);

	// tr: amiibo type
	static const array<const char*, 5> amiibo_type_tbl = {{
		// tr: NFP_TYPE_FIGURINE == standard amiibo
		NOP_C_("Amiibo|Type", "Figurine"),
		// tr: NFP_TYPE_CARD == amiibo card
		NOP_C_("Amiibo|Type", "Card"),
		// tr: NFP_TYPE_YARN == yarn amiibo
		NOP_C_("Amiibo|Type", "Yarn"),
		// tr: NFP_TYPE_BAND == Power-Up Band
		NOP_C_("Amiibo|Type", "Power-Up Band"),
		// tr: NFP_TYPE_WOODEN_BLOCK = wooden blocks
		NOP_C_("Amiibo|Type", "Wooden Block"),
	}};
	const char *const amiibo_type_title = C_("Amiibo", "amiibo Type");
	if ((char_id & 0xFF) < amiibo_type_tbl.size()) {
		d->fields.addField_string(amiibo_type_title,
			pgettext_expr("Amiibo|Type", amiibo_type_tbl[char_id & 0xFF]));
	} else {
		// Invalid amiibo type.
		d->fields.addField_string(amiibo_type_title,
			fmt::format(FRUN(C_("RomData", "Unknown (0x{:0>2X})")), (char_id & 0xFF)));
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
	int wave_no = 0, release_no = 0;
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
	const string credits = fmt::format(
		FRUN(C_("Amiibo", "amiibo images provided by {:s},\nthe Unofficial amiibo Database.")),
		"<a href=\"https://amiibo.life/\">amiibo.life</a>");
	d->fields.addField_string(C_("Amiibo", "Credits"), credits,
		RomFields::STRF_CREDITS | RomFields::STRF_PARSE_LINKS);

	// Finished reading the field data.
	return d->fields.count();
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int Amiibo::loadMetaData(void)
{
	RP_D(Amiibo);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// amiibo file isn't valid.
		return -EIO;
	}

	d->metaData.reserve(2);	// Maximum of 2 metadata properties.

	// NFP data
	const uint32_t char_id = be32_to_cpu(d->nfpData.ntag215.user_data.char_id);
	const uint32_t amiibo_id = be32_to_cpu(d->nfpData.ntag215.user_data.amiibo_id.u32);

	// Get the AmiiboData instance.
	const AmiiboData *const pAmiiboData = AmiiboData::instance();

	// amiibo Name (as Title)
	const char *const amiibo_name = pAmiiboData->lookup_amiibo_series_data(amiibo_id);
	if (amiibo_name) {
		d->metaData.addMetaData_string(Property::Title, amiibo_name);
	}

	/** Custom properties! **/

	// amiibo ID (as Title ID)
	d->metaData.addMetaData_string(Property::TitleID,
		fmt::format(FSTR("{:0>8X}-{:0>8X}"), char_id, amiibo_id));

	// Finished reading the metadata.
	return d->metaData.count();
}

/**
 * Get a list of URLs for an external image type.
 *
 * A thumbnail size may be requested from the shell.
 * If the subclass supports multiple sizes, it should
 * try to get the size that most closely matches the
 * requested size.
 *
 * @param imageType	[in]     Image type
 * @param extURLs	[out]    Output vector
 * @param size		[in,opt] Requested image size. This may be a requested
 *                               thumbnail size in pixels, or an ImageSizeType
 *                               enum value.
 * @return 0 on success; negative POSIX error code on error.
 */
int Amiibo::extURLs(ImageType imageType, vector<ExtURL> &extURLs, int size) const
{
	extURLs.clear();
	ASSERT_extURLs(imageType);

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
	extURLs.resize(1);
	ExtURL &extURL = extURLs[0];

	// Amiibo ID
	const string amiibo_id = fmt::format(FSTR("{:0>8X}-{:0>8X}"),
		be32_to_cpu(d->nfpData.ntag215.user_data.char_id),
		be32_to_cpu(d->nfpData.ntag215.user_data.amiibo_id.u32));

	// Cache key (amiibo ID)
	extURL.cache_key.reserve(32);
	extURL.cache_key = "amiibo/";
	extURL.cache_key += amiibo_id;
	extURL.cache_key += ".png";

	// URL
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

} // namespace LibRomData
