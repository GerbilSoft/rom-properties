/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiU.cpp: Nintendo Wii U disc image reader.                             *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"

#include "WiiU.hpp"
#include "data/NintendoPublishers.hpp"
#include "wiiu_structs.h"
#include "gcn_structs.h"
#include "data/WiiUData.hpp"
#include "GameCubeRegions.hpp"

// Other rom-properties libraries
#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;
using namespace LibRpText;
using LibRpFile::IRpFile;

// DiscReader
#include "librpbase/disc/DiscReader.hpp"
#include "disc/WuxReader.hpp"

// C++ STL classes
using std::shared_ptr;
using std::string;
using std::vector;

namespace LibRomData {

class WiiUPrivate final : public RomDataPrivate
{
	public:
		WiiUPrivate(const shared_ptr<IRpFile> &file);
		~WiiUPrivate() final;

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(WiiUPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		enum class DiscType {
			Unknown	= -1,

			WUD	= 0,	// Wii U disc image (uncompressed)
			WUX	= 1,	// WUX (compressed)

			Max
		};
		DiscType discType;

		// Disc reader
		IDiscReader *discReader;

		// Disc header
		WiiU_DiscHeader discHeader;
};

ROMDATA_IMPL(WiiU)
ROMDATA_IMPL_IMG(WiiU)

/** WiiUPrivate **/

/* RomDataInfo */
const char *const WiiUPrivate::exts[] = {
	".wud", ".wux",

	// NOTE: May cause conflicts on Windows
	// if fallback handling isn't working.
	".iso",

	nullptr
};
const char *const WiiUPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-wii-u-rom",

	nullptr
};
const RomDataInfo WiiUPrivate::romDataInfo = {
	"WiiU", exts, mimeTypes
};

WiiUPrivate::WiiUPrivate(const shared_ptr<IRpFile> &file)
	: super(file, &romDataInfo)
	, discType(DiscType::Unknown)
	, discReader(nullptr)
{
	// Clear the discHeader struct.
	memset(&discHeader, 0, sizeof(discHeader));
}

WiiUPrivate::~WiiUPrivate()
{
	UNREF(discReader);
}

/** WiiU **/

/**
 * Read a Nintendo Wii U disc image.
 *
 * A disc image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open disc image.
 */
WiiU::WiiU(const shared_ptr<IRpFile> &file)
	: super(new WiiUPrivate(file))
{
	// This class handles disc images.
	RP_D(WiiU);
	d->mimeType = "application/x-wii-u-rom";	// unofficial, not on fd.o
	d->fileType = FileType::DiscImage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the disc header.
	// NOTE: Using sizeof(GCN_DiscHeader) so we can verify that
	// GCN/Wii magic numbers are not present.
	static_assert(sizeof(GCN_DiscHeader) >= sizeof(WiiU_DiscHeader),
		"GCN_DiscHeader is smaller than WiiU_DiscHeader.");
	uint8_t header[sizeof(GCN_DiscHeader)];
	d->file->rewind();
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header)) {
		d->file.reset();
		return;
	}

	// Check if this disc image is supported.
	const DetectInfo info = {
		{0, sizeof(header), header},
		nullptr,	// ext (not needed for WiiU)
		d->file->size()	// szFile
	};
	d->discType = static_cast<WiiUPrivate::DiscType>(isRomSupported_static(&info));

	// Create an IDiscReader.
	switch (d->discType) {
		case WiiUPrivate::DiscType::WUD:
			d->discReader = new DiscReader(d->file);
			break;
		case WiiUPrivate::DiscType::WUX:
			d->discReader = new WuxReader(d->file);
			break;
		case WiiUPrivate::DiscType::Unknown:
		default:
			// Disc image is invalid.
			d->discReader = nullptr;
			d->fileType = FileType::Unknown;
			d->discType = WiiUPrivate::DiscType::Unknown;
			d->file.reset();
			return;
	}

	if (!d->discReader || !d->discReader->isOpen()) {
		// Error opening the DiscReader.
		UNREF_AND_NULL(d->discReader);
		d->file.reset();
		d->fileType = FileType::Unknown;
		d->discType = WiiUPrivate::DiscType::Unknown;
		return;
	}

	// Re-read the disc header for WUX.
	if (d->discType > WiiUPrivate::DiscType::WUD) {
		size = d->discReader->seekAndRead(0, header, sizeof(header));
		if (size != sizeof(header)) {
			// Seek and/or read error.
			UNREF_AND_NULL_NOCHK(d->discReader);
			d->file.reset();
			d->discType = WiiUPrivate::DiscType::Unknown;
			return;
		}
	}

	// Verify the secondary magic number at 0x10000.
	uint32_t disc_magic;
	size = d->discReader->seekAndRead(0x10000, &disc_magic, sizeof(disc_magic));
	if (size != sizeof(disc_magic)) {
		// Seek and/or read error.
		UNREF_AND_NULL_NOCHK(d->discReader);
		d->file.reset();
		d->discType = WiiUPrivate::DiscType::Unknown;
		return;
	}

	if (disc_magic == cpu_to_be32(WIIU_SECONDARY_MAGIC)) {
		// Secondary magic matches.
		d->isValid = true;

		// Save the disc header.
		memcpy(&d->discHeader, header, sizeof(d->discHeader));
	} else {
		// No match.
		UNREF_AND_NULL_NOCHK(d->discReader);
		d->file.reset();
		d->discType = WiiUPrivate::DiscType::Unknown;
		return;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiU::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(GCN_DiscHeader) ||
	    info->szFile < 0x20000)
	{
		// Either no detection information was specified,
		// or the header is too small.
		// szFile: Partition table is at 0x18000, so we
		// need to have at least 0x20000.
		return static_cast<int>(WiiUPrivate::DiscType::Unknown);
	}

	// Check if this is disc is in WUX format.
	if (WuxReader::isDiscSupported_static(info->header.pData, info->header.size) >= 0) {
		// Disc image is in WUX format.
		// TODO: Also check for other Wii U magic numbers if WUX is found.
		// TODO: Verify block size?
		return static_cast<int>(WiiUPrivate::DiscType::WUX);
	}

	// Game ID must start with "WUP-".
	// NOTE: There's also a secondary magic number at 0x10000,
	// but we can't check it here.
	// TODO: Dev discs don't have a "WUP-" magic number.
	const WiiU_DiscHeader *const wiiu_header = reinterpret_cast<const WiiU_DiscHeader*>(info->header.pData);
	if (wiiu_header->magic != cpu_to_be32(WIIU_MAGIC)) {
		// Not Wii U.
		return static_cast<int>(WiiUPrivate::DiscType::Unknown);
	}

	// Check hyphens.
	// TODO: Verify version numbers and region code.
	if (wiiu_header->hyphen1 != '-' ||
	    wiiu_header->hyphen2 != '-' ||
	    wiiu_header->hyphen3 != '-' ||
	    wiiu_header->hyphen4 != '-' ||
	    wiiu_header->hyphen5 != '-')
	{
		// Missing hyphen.
		return static_cast<int>(WiiUPrivate::DiscType::Unknown);
	}

	// Check for GCN/Wii magic numbers.
	const GCN_DiscHeader *gcn_header = reinterpret_cast<const GCN_DiscHeader*>(info->header.pData);
	if (gcn_header->magic_wii == be32_to_cpu(WII_MAGIC) ||
	    gcn_header->magic_gcn == be32_to_cpu(GCN_MAGIC))
	{
		// GameCube and/or Wii magic is present.
		// This is not a Wii U disc image.
		return static_cast<int>(WiiUPrivate::DiscType::Unknown);
	}

	// Disc header is valid.
	return static_cast<int>(WiiUPrivate::DiscType::WUD);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *WiiU::systemName(unsigned int type) const
{
	RP_D(const WiiU);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Wii U has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"WiiU::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Nintendo Wii U", "Wii U", "Wii U", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t WiiU::supportedImageTypes_static(void)
{
#ifdef HAVE_JPEG
	return IMGBF_EXT_MEDIA |
	       IMGBF_EXT_COVER | IMGBF_EXT_COVER_3D |
	       IMGBF_EXT_COVER_FULL;
#else /* !HAVE_JPEG */
	return IMGBF_EXT_MEDIA | IMGBF_EXT_COVER_3D;
#endif /* HAVE_JPEG */
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
vector<RomData::ImageSizeDef> WiiU::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_EXT_MEDIA: {
			static const ImageSizeDef sz_EXT_MEDIA[] = {
				{nullptr, 160, 160, 0},
				{"M", 500, 500, 1},
			};
			return vector<ImageSizeDef>(sz_EXT_MEDIA,
				sz_EXT_MEDIA + ARRAY_SIZE(sz_EXT_MEDIA));
		}
#ifdef HAVE_JPEG
		case IMG_EXT_COVER: {
			static const ImageSizeDef sz_EXT_COVER[] = {
				{nullptr, 160, 224, 0},
				{"M", 350, 500, 1},
				{"HQ", 768, 1080, 2},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER,
				sz_EXT_COVER + ARRAY_SIZE(sz_EXT_COVER));
		}
#endif /* HAVE_JPEG */
		case IMG_EXT_COVER_3D: {
			static const ImageSizeDef sz_EXT_COVER_3D[] = {
				{nullptr, 176, 248, 0},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER_3D,
				sz_EXT_COVER_3D + ARRAY_SIZE(sz_EXT_COVER_3D));
		}
#ifdef HAVE_JPEG
		case IMG_EXT_COVER_FULL: {
			static const ImageSizeDef sz_EXT_COVER[] = {
				{nullptr, 340, 224, 0},
				{"M", 752, 500, 1},
				{"HQ", 1632, 1080, 2},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER,
				sz_EXT_COVER + ARRAY_SIZE(sz_EXT_COVER));
		}
#endif /* HAVE_JPEG */
		default:
			break;
	}

	// Unsupported image type.
	return vector<ImageSizeDef>();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int WiiU::loadFieldData(void)
{
	RP_D(WiiU);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->discType < 0) {
		// Disc image isn't valid.
		return -EIO;
	}

	// Disc header is read in the constructor.
	const WiiU_DiscHeader *const discHeader = &d->discHeader;
	d->fields.reserve(4);	// Maximum of 4 fields.

	// Game ID
	d->fields.addField_string(C_("RomData", "Game ID"),
		latin1_to_utf8(discHeader->id, sizeof(discHeader->id)));

	// Publisher
	// Look up the publisher ID.
	char publisher_code[5];
	const char *publisher = nullptr;
	string s_publisher;

	const uint32_t publisher_id = WiiUData::lookup_disc_publisher(discHeader->id4);
	publisher_code[0] = (publisher_id >> 24) & 0xFF;
	publisher_code[1] = (publisher_id >> 16) & 0xFF;
	publisher_code[2] = (publisher_id >>  8) & 0xFF;
	publisher_code[3] =  publisher_id & 0xFF;
	publisher_code[4] = 0;
	if (publisher_id != 0 && (publisher_id & 0xFFFF0000) == 0x30300000) {
		// Publisher ID is a valid two-character ID.
		publisher = NintendoPublishers::lookup(&publisher_code[2]);
	}
	if (publisher) {
		s_publisher = publisher;
	} else {
		if (ISALNUM(publisher_code[0]) && ISALNUM(publisher_code[1]) &&
		    ISALNUM(publisher_code[2]) && ISALNUM(publisher_code[3]))
		{
			s_publisher = rp_sprintf(C_("RomData", "Unknown (%.4s)"), publisher_code);
		} else {
			s_publisher = rp_sprintf(C_("RomData", "Unknown (%02X %02X %02X %02X)"),
				static_cast<uint8_t>(publisher_code[0]),
				static_cast<uint8_t>(publisher_code[1]),
				static_cast<uint8_t>(publisher_code[2]),
				static_cast<uint8_t>(publisher_code[3]));
		}
	}
	d->fields.addField_string(C_("RomData", "Publisher"), s_publisher);

	// Game version
	// TODO: Validate the version characters.
	d->fields.addField_string(C_("RomData", "Version"),
		latin1_to_utf8(discHeader->version, sizeof(discHeader->version)));

	// OS version
	// TODO: Validate the version characters.
	const char s_os_version[] = {
		discHeader->os_version[0], '.',
		discHeader->os_version[1], '.',
		discHeader->os_version[2], '\0'
	};
	d->fields.addField_string(C_("WiiU", "OS Version"), s_os_version);

	// Region
	// TODO: Compare against list of regions and show the fancy name.
	d->fields.addField_string(C_("RomData", "Region Code"),
		latin1_to_utf8(discHeader->region, sizeof(discHeader->region)));

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
int WiiU::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	RP_D(const WiiU);
	if (!d->isValid || (int)d->discType < 0) {
		// Disc image isn't valid.
		return -EIO;
	}

	// Get the image sizes and sort them based on the
	// requested image size.
	vector<ImageSizeDef> sizeDefs = supportedImageSizes(imageType);
	if (sizeDefs.empty()) {
		// No image sizes.
		return -ENOENT;
	}

	// Select the best size.
	const ImageSizeDef *const sizeDef = d->selectBestSize(sizeDefs, size);
	if (!sizeDef) {
		// No size available...
		return -ENOENT;
	}

	// NOTE: Only downloading the first size as per the
	// sort order, since GameTDB basically guarantees that
	// all supported sizes for an image type are available.
	// TODO: Add cache keys for other sizes in case they're
	// downloaded and none of these are available?

	// Determine the image type name.
	const char *imageTypeName_base;
	const char *ext;
	switch (imageType) {
		case IMG_EXT_MEDIA:
			imageTypeName_base = "disc";
			ext = ".png";
			break;
#ifdef HAVE_JPEG
		case IMG_EXT_COVER:
			imageTypeName_base = "cover";
			ext = ".jpg";
			break;
#endif /* HAVE_JPEG */
		case IMG_EXT_COVER_3D:
			imageTypeName_base = "cover3D";
			ext = ".png";
			break;
#ifdef HAVE_JPEG
		case IMG_EXT_COVER_FULL:
			imageTypeName_base = "coverfull";
			ext = ".jpg";
			break;
#endif /* HAVE_JPEG */
		default:
			// Unsupported image type.
			return -ENOENT;
	}

	// Disc header is read in the constructor.
	const WiiU_DiscHeader *const discHeader = &d->discHeader;

	// Look up the publisher ID.
	const uint32_t publisher_id = WiiUData::lookup_disc_publisher(discHeader->id4);
	if (publisher_id == 0 || (publisher_id & 0xFFFF0000) != 0x30300000) {
		// Either the publisher ID is unknown, or it's a
		// 4-character ID, which isn't supported by
		// GameTDB at the moment.
		return -ENOENT;
	}

	// Determine the GameTDB language code(s).
	// TODO: Figure out the actual Wii U region code.
	// Using the game ID for now.
	const vector<uint16_t> tdb_lc = GameCubeRegions::gcnRegionToGameTDB(~0U, discHeader->id4[3]);

	// Game ID.
	// Replace any non-printable characters with underscores.
	// (GameCube NDDEMO has ID6 "00\0E01".)
	char id6[7];
	for (unsigned int i = 0; i < 4; i++) {
		id6[i] = (ISPRINT(discHeader->id4[i]))
			? discHeader->id4[i]
			: '_';
	}

	// Publisher ID.
	id6[4] = (publisher_id >> 8) & 0xFF;
	id6[5] = publisher_id & 0xFF;
	id6[6] = 0;

	// If we're downloading a "high-resolution" image (M or higher),
	// also add the default image to ExtURLs in case the user has
	// high-resolution image downloads disabled.
	const ImageSizeDef *szdefs_dl[2];
	szdefs_dl[0] = sizeDef;
	unsigned int szdef_count;
	if (sizeDef->index > 0) {
		// M or higher.
		szdefs_dl[1] = &sizeDefs[0];
		szdef_count = 2;
	} else {
		// Default or S.
		szdef_count = 1;
	}

	// Add the URLs.
	pExtURLs->resize(szdef_count * tdb_lc.size());
	auto extURL_iter = pExtURLs->begin();
	const auto tdb_lc_cend = tdb_lc.cend();
	for (unsigned int i = 0; i < szdef_count; i++) {
		// Current image type.
		char imageTypeName[16];
		snprintf(imageTypeName, sizeof(imageTypeName), "%s%s",
			 imageTypeName_base, (szdefs_dl[i]->name ? szdefs_dl[i]->name : ""));

		// Add the images.
		for (auto tdb_iter = tdb_lc.cbegin();
		     tdb_iter != tdb_lc_cend; ++tdb_iter, ++extURL_iter)
		{
			const string lc_str = SystemRegion::lcToStringUpper(*tdb_iter);
			extURL_iter->url = d->getURL_GameTDB("wiiu", imageTypeName, lc_str.c_str(), id6, ext);
			extURL_iter->cache_key = d->getCacheKey_GameTDB("wiiu", imageTypeName, lc_str.c_str(), id6, ext);
			extURL_iter->width = szdefs_dl[i]->width;
			extURL_iter->height = szdefs_dl[i]->height;
			extURL_iter->high_res = (szdefs_dl[i]->index > 0);
		}
	}

	// All URLs added.
	return 0;
}

}
