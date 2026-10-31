/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * FlipnoteStudio.cpp: Flipnote Studio animation reader.                   *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "FlipnoteStudio.hpp"
#include "RomData_p.hpp"

#include "flipnote_structs.h"
#include "../Console/gcn_card.h"	// for GC_UNIX_TIME_DIFF

// Other rom-properties libraries
#include "librpbase/img/RpJpeg.hpp"
#include "librpfile/MemFile.hpp"
#include "librptexture/decoder/ImageDecoder_NDS.hpp"
#include "bitstuff.h"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class FlipnoteStudioPrivate final : public RomDataPrivate
{
public:
	explicit FlipnoteStudioPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
public:
	RP_DISABLE_COPY(FlipnoteStudioPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 2+1> exts;
	static const array<const char*, 3+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	enum class AnimType {
		Unknown	= -1,

		PPM	= 0,	// Flipnote Studio (DSi)
		KWZ	= 1,	// Flipnote Studio 3D (3DS)

		Max
	};
	AnimType animType;

	// File header
	union {
		FlipnoteStudio_PPM_Header_t ppm;
		FlipnoteStudio3D_KFH_t kfh;
	} header;

	// Maximum of 1 MB for the Flipnote Studio 3D JPEG preview image.
	static const uint32_t FLIPNOTE3D_KTN_MAX_JPEG_SIZE = 1048576U;

	// Preview image
	rp_image_ptr img_image;

	/**
	 * Load the image.
	 * @return Image, or nullptr on error.
	 */
	rp_image_const_ptr loadImage(void);

private:
	// PPM preview palette
	// TODO: Get the exact palette from Flipnote Studio.
	// Using the palette from: https://www.dsibrew.org/wiki/Flipnote_Files/PPM
	// NOTE: Stored in NDS BGR555 format because fromNDS_CI4() requires it.
	static const array<uint16_t, 16> ppm_palette;
};

ROMDATA_IMPL(FlipnoteStudio)

/** FlipnoteStudioPrivate **/

/* RomDataInfo */
const array<const char*, 2+1> FlipnoteStudioPrivate::exts = {{
	".ppm",	// Flipnote Studio (DSi)
	".kwz",	// Flipnote Studio 3D (3DS)

	nullptr
}};
const array<const char*, 3+1> FlipnoteStudioPrivate::mimeTypes = {{
	// Unofficial MIME types.
	"image/x-portable-pixmap",	// HACK due to .ppm extension conflict

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"image/x-flipnote-studio",	// Flipnote Studio (DSi)
	"image/x-flipnote-studio-3d",	// Flipnote Studio 3D (3DS)

	nullptr
}};
const RomDataInfo FlipnoteStudioPrivate::romDataInfo = {
	"FlipnoteStudio", exts.data(), mimeTypes.data()
};

// PPM preview palette
// TODO: Get the exact palette from Flipnote Studio.
// Using the palette from: https://www.dsibrew.org/wiki/Flipnote_Files/PPM
// NOTE: Stored in NDS BGR555 format because fromNDS_CI4() requires it.
#define NDS_BGR555(r, g, b) (((b) << 10) | ((g) << 5) | (r))
const array<uint16_t, 16> FlipnoteStudioPrivate::ppm_palette = {{
	NDS_BGR555(31, 31, 31),	// TODO: Transparent?
	NDS_BGR555(10, 10, 10),	// dark gray
	NDS_BGR555(31, 31, 31),	// white
	NDS_BGR555(20, 20, 20),	// light gray
	NDS_BGR555(31,  0,  0),	// pure red
	NDS_BGR555(15,  0,  0),	// dark red
	NDS_BGR555(31, 15, 15),	// light red / pink
	NDS_BGR555( 0, 31,  0),	// pure green
	NDS_BGR555( 0,  0, 31),	// pure blue
	NDS_BGR555( 0,  0, 15),	// dark blue
	NDS_BGR555(15, 15, 31),	// light blue
	NDS_BGR555( 0, 31,  0),	// pure green
	NDS_BGR555(31,  0, 31),	// magenta / purple
	NDS_BGR555( 0, 31,  0),	// pure green
	NDS_BGR555( 0, 31,  0),	// pure green
	NDS_BGR555( 0, 31,  0),	// pure green
}};

FlipnoteStudioPrivate::FlipnoteStudioPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the ROM header struct.
	memset(&header, 0, sizeof(header));
}

/**
 * Load the image.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr FlipnoteStudioPrivate::loadImage(void)
{
	rp_image_const_ptr img;

	if (img_image) {
		// Preview image has already been loaded.
		// NOTE: Assigning to `img` for named-return-value optimization.
		img = img_image;
		return img;
	} else if (!this->isValid || !this->file) {
		// Can't load the preview image.
		return img;
	}

	rp_image_ptr tmp_image;
	switch (animType) {
		default:
			assert(!"Unsupported animation type???");
			break;

		case AnimType::PPM: {
			// PPM images are stored in the header in NDS 4bpp tiled format.
			tmp_image = ImageDecoder::fromNDS_CI4(
				FLIPNOTE_PPM_WIDTH, FLIPNOTE_PPM_HEIGHT,
				header.ppm.preview_bitmap, sizeof(header.ppm.preview_bitmap),
				ppm_palette.data(), sizeof(ppm_palette));
			if (!tmp_image) {
				// Unable to decode the icon.
				return img;
			}

			// Set the sBIT metadata.
			// TODO: Transparency?
			static const rp_image::sBIT_t sBIT = {5,5,5,0,0};
			tmp_image->set_sBIT(sBIT);
			break;
		}

		case AnimType::KWZ: {
			// Find the thumbnail preview chunk. ('KTN')
			if (!file || !file->isOpen()) {
				// File is closed...
				return img;
			}

			// TODO: Scan past the first chunk if it isn't 'KTN'.
			// For now, only check the first chunk.
			FlipnoteStudio3D_Chunk_Header_t ktnHeader;
			uint32_t addr = le32_to_cpu(header.kfh.header.size) + 8;
			size_t size = file->seekAndRead(addr, &ktnHeader, sizeof(ktnHeader));
			if (size != sizeof(ktnHeader)) {
				return img;
			}

			if ((be32_to_cpu(ktnHeader.magic) & FLIPNOTE3D_KTN_MAGIC_MASK) != FLIPNOTE3D_KTN_MAGIC) {
				// Not the 'KTN' chunk.
				return img;
			}

			const uint32_t jpeg_size = le32_to_cpu(ktnHeader.size);
			if (jpeg_size > FLIPNOTE3D_KTN_MAX_JPEG_SIZE) {
				// JPEG is too big.
				return img;
			}

			// Load the JPEG.
			unique_ptr<uint8_t[]> jpeg_buf(new uint8_t[jpeg_size]);
			size = file->read(jpeg_buf.get(), jpeg_size);
			if (size != jpeg_size) {
				// Short read...
				return img;
			}

			// Decode the JPEG.
			// TODO: Truncate the bottom 4 rows? (JPEG is 80x64, but only 80x60 is displayed...)
			MemFilePtr memFile = std::make_shared<MemFile>(jpeg_buf.get(), jpeg_size);
			tmp_image = RpJpeg::load(memFile);
			if (!tmp_image) {
				// Unable to decode the JPEG...
				return img;
			}
			break;
		}
	}

	// Save and return the icon.
	// NOTE: Assigning to `icon` for named-return-value optimization.
	this->img_image = tmp_image;
	img = std::move(tmp_image);
	return img;
}

/** FlipnoteStudio **/

/**
 * Read a Flipnote Studio (or Flipnote Studio 3D) animation file.
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
FlipnoteStudio::FlipnoteStudio(const IRpFilePtr &file)
	: super(new FlipnoteStudioPrivate(file))
{
	RP_D(FlipnoteStudio);
	d->fileType = FileType::AnimationFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the PPM header.
	d->file->rewind();
	size_t size = d->file->read(&d->header, sizeof(d->header));
	if (size != sizeof(d->header)) {
		d->file.reset();
		return;
	}

	DetectInfo info = {
		{0, sizeof(d->header), reinterpret_cast<const uint8_t*>(&d->header)},
		nullptr,	// ext (not needed for FlipnoteStudio)
		0		// szFile (not needed for FlipnoteStudio)
	};
	d->animType = static_cast<FlipnoteStudioPrivate::AnimType>(isRomSupported_static(&info));

	d->isValid = ((int)d->animType >= 0);
	if (!d->isValid) {
		d->file.reset();
		return;
	}

	switch (d->animType) {
		default:
			assert(!"Unsupported animation type???");
			break;

		case FlipnoteStudioPrivate::AnimType::PPM:
			d->mimeType = "image/x-flipnote-studio";
			break;
		case FlipnoteStudioPrivate::AnimType::KWZ:
			d->mimeType = "image/x-flipnote-studio-3d";
			break;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int FlipnoteStudio::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(FlipnoteStudio_PPM_Header_t))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(FlipnoteStudioPrivate::AnimType::Unknown);
	}

	// Check for a Flipnote Studio (DSi) animation. (PPM)
	const FlipnoteStudio_PPM_Header_t *const ppmHeader =
		reinterpret_cast<const FlipnoteStudio_PPM_Header_t*>(info->header.pData);
	if (ppmHeader->magic == cpu_to_be32(FLIPNOTE_PPM_MAGIC)) {
		// Magic number is correct.
		// Verify some fields that are known to have fixed values.
		if (ppmHeader->unknown_00E == cpu_to_le16(0x0024) &&
		    ppmHeader->unknown_09E == 0)
		{
			// Fixed values match.
			return static_cast<int>(FlipnoteStudioPrivate::AnimType::PPM);
		}
	}

	// Check for a Flipnote Studio (3DS) animation. (KWZ)
	const FlipnoteStudio3D_KFH_t *const kfhHeader =
		reinterpret_cast<const FlipnoteStudio3D_KFH_t*>(info->header.pData);
	if ((be32_to_cpu(kfhHeader->header.magic) & FLIPNOTE3D_KFH_MAGIC_MASK) == FLIPNOTE3D_KFH_MAGIC) {
		// Magic number is correct.
		// Verify the header size.
		if ((le32_to_cpu(kfhHeader->header.size) + 8) == sizeof(*kfhHeader)) {
			// Header size is correct.
			// TODO: Verify other values?
			return static_cast<int>(FlipnoteStudioPrivate::AnimType::KWZ);
		}
	}

	// Not supported.
	return static_cast<int>(FlipnoteStudioPrivate::AnimType::Unknown);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *FlipnoteStudio::systemName(unsigned int type) const
{
	RP_D(const FlipnoteStudio);
	if (!d->isValid || !isSystemNameTypeValid(type)) {
		return nullptr;
	}

	// Flipnote Studio has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"FlipnoteStudio::systemName() array index optimization needs to be updated.");

	static const array<const char*, 4> sysNames = {{
		"Flipnote Studio", "Flipnote", "Flipnote", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t FlipnoteStudio::supportedImageTypes_static(void)
{
	return IMGBF_INT_IMAGE;
}

/**
 * Get a bitfield of image types this object can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t FlipnoteStudio::supportedImageTypes(void) const
{
	// TODO: For KWZ, verify that a 'KTN' chunk is present?
	return IMGBF_INT_IMAGE;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> FlipnoteStudio::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported.
		return {};
	}

	// NOTE: Only returning PPM (DSi) sizes.
	return {{nullptr, FLIPNOTE_PPM_WIDTH, FLIPNOTE_PPM_HEIGHT, 0}};
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> FlipnoteStudio::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported.
		return {};
	}

	RP_D(const FlipnoteStudio);
	switch (d->animType) {
		default:
			assert(!"Unsupported animation type???");
			break;

		case FlipnoteStudioPrivate::AnimType::PPM:
			return {{nullptr, FLIPNOTE_PPM_WIDTH, FLIPNOTE_PPM_HEIGHT, 0}};

		case FlipnoteStudioPrivate::AnimType::KWZ:
			// TODO: Verify that a 'KTN' chunk is present?
			return {{nullptr, FLIPNOTE3D_KTN_WIDTH, FLIPNOTE3D_KTN_HEIGHT, 0}};
	}

	return {};
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
uint32_t FlipnoteStudio::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	switch (imageType) {
		case IMG_INT_IMAGE:
			// Use nearest-neighbor scaling.
			return IMGPF_RESCALE_NEAREST;
		default:
			break;
	}
	return 0;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int FlipnoteStudio::loadFieldData(void)
{
	RP_D(FlipnoteStudio);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || static_cast<int>(d->animType) < 0) {
		// Animation file isn't valid.
		return -EIO;
	}

	switch (d->animType) {
		default:
			assert(!"Unsupported animation type???");
			break;

		case FlipnoteStudioPrivate::AnimType::PPM: {
			// File header is read in the constructor.
			const FlipnoteStudio_PPM_Header_t *const ppmHeader = &d->header.ppm;
			d->fields.reserve(5);	// Maximum of 5 fields. (TODO: Add more fields?)

			// # of frames
			d->fields.addField_string_numeric(C_("FlipnoteStudio", "# of Frames"),
				le32_to_cpu(ppmHeader->num_frames));

			// Author names
			// TODO: U+E000 private use area from Nintendo fonts.
			if (ppmHeader->orig_author_name[0] != 0) {
				d->fields.addField_string(C_("FlipnoteStudio", "Original Author"),
					utf16le_to_utf8(ppmHeader->orig_author_name, ARRAY_SIZE(ppmHeader->orig_author_name)),
					RomFields::STRF_NTLG_FONT);
			}
			if (ppmHeader->last_edited_author_name[0] != 0) {
				d->fields.addField_string(C_("FlipnoteStudio", "Last Edited By"),
					utf16le_to_utf8(ppmHeader->last_edited_author_name, ARRAY_SIZE(ppmHeader->last_edited_author_name)),
					RomFields::STRF_NTLG_FONT);
			}
			if (ppmHeader->user_name[0] != 0) {
				d->fields.addField_string(C_("FlipnoteStudio", "Username"),
					utf16le_to_utf8(ppmHeader->user_name, ARRAY_SIZE(ppmHeader->user_name)),
					RomFields::STRF_NTLG_FONT);
			}

			// Timestamp
			// TODO: "Timestamp" or "Last Modified"?
			d->fields.addField_dateTime(C_("RomData", "Timestamp"),
				static_cast<rp_time_t>(ppmHeader->timestamp) + static_cast<rp_time_t>(GC_UNIX_TIME_DIFF),
				RomFields::RFT_DATETIME_HAS_DATE |
				RomFields::RFT_DATETIME_HAS_TIME |
				RomFields::RFT_DATETIME_IS_UTC	// DSi doesn't support timezones.
				);

			break;
		}

		case FlipnoteStudioPrivate::AnimType::KWZ: {
			// File header is read in the constructor.
			const FlipnoteStudio3D_KFH_t *const kfhHeader = &d->header.kfh;
			d->fields.reserve(6);	// Maximum of 6 fields. (TODO: Add more fields?)

			// # of frames
			d->fields.addField_string_numeric(C_("FlipnoteStudio", "# of Frames"),
				le32_to_cpu(kfhHeader->frame_count));

			// Author names
			// TODO: U+E000 private use area from Nintendo fonts.
			if (kfhHeader->root_author_name[0] != 0) {
				d->fields.addField_string(C_("FlipnoteStudio", "Root Author"),
					utf16le_to_utf8(kfhHeader->root_author_name, ARRAY_SIZE(kfhHeader->root_author_name)),
					RomFields::STRF_NTLG_FONT);
			}
			if (kfhHeader->parent_author_name[0] != 0) {
				d->fields.addField_string(C_("FlipnoteStudio", "Parent Author"),
					utf16le_to_utf8(kfhHeader->parent_author_name, ARRAY_SIZE(kfhHeader->parent_author_name)),
					RomFields::STRF_NTLG_FONT);
			}
			if (kfhHeader->current_author_name[0] != 0) {
				d->fields.addField_string(C_("FlipnoteStudio", "Current Author"),
					utf16le_to_utf8(kfhHeader->current_author_name, ARRAY_SIZE(kfhHeader->current_author_name)),
					RomFields::STRF_NTLG_FONT);
			}

			// Timestamps
			d->fields.addField_dateTime(C_("RomData", "Creation Time"),
				static_cast<rp_time_t>(kfhHeader->creation_time) + static_cast<rp_time_t>(GC_UNIX_TIME_DIFF),
				RomFields::RFT_DATETIME_HAS_DATE |
				RomFields::RFT_DATETIME_HAS_TIME |
				RomFields::RFT_DATETIME_IS_UTC	// 3DS doesn't support timezones.
				);
			d->fields.addField_dateTime(C_("RomData", "Modified Time"),
				static_cast<rp_time_t>(kfhHeader->modified_time) + static_cast<rp_time_t>(GC_UNIX_TIME_DIFF),
				RomFields::RFT_DATETIME_HAS_DATE |
				RomFields::RFT_DATETIME_HAS_TIME |
				RomFields::RFT_DATETIME_IS_UTC	// 3DS doesn't support timezones.
				);

			break;
		}
	}

	// Finished reading the field data.
	return d->fields.count();
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int FlipnoteStudio::loadMetaData(void)
{
	RP_D(FlipnoteStudio);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || static_cast<int>(d->animType) < 0) {
		// Animation file isn't valid.
		return -EIO;
	}

	switch (d->animType) {
		default:
			assert(!"Unsupported animation type???");
			break;

		case FlipnoteStudioPrivate::AnimType::PPM: {
			// File header is read in the constructor.
			const FlipnoteStudio_PPM_Header_t *const ppmHeader = &d->header.ppm;
			d->metaData.reserve(2);	// Maximum of 2 metadata properties. (TODO: Add more?)

			// Author name (NOTE: Only listing original author.)
			if (ppmHeader->orig_author_name[0] != 0) {
				d->metaData.addMetaData_string(Property::Author,
					utf16le_to_utf8(ppmHeader->orig_author_name, ARRAY_SIZE(ppmHeader->orig_author_name)));
			}

			// Timestamp
			// NOTE: Using CreationDate. (no ModificationDate property...)
			// TODO: Adjust for local timezone, since it's UTC.
			d->metaData.addMetaData_timestamp(Property::CreationDate,
				static_cast<rp_time_t>(ppmHeader->timestamp) + static_cast<rp_time_t>(GC_UNIX_TIME_DIFF));

			break;
		}

		case FlipnoteStudioPrivate::AnimType::KWZ: {
			// File header is read in the constructor.
			const FlipnoteStudio3D_KFH_t *const kfhHeader = &d->header.kfh;
			d->metaData.reserve(2);	// Maximum of 2 metadata properties. (TODO: Add more?)

			// Author name (NOTE: Only listing root author.)
			if (kfhHeader->root_author_name[0] != 0) {
				d->metaData.addMetaData_string(Property::Author,
					utf16le_to_utf8(kfhHeader->root_author_name, ARRAY_SIZE(kfhHeader->root_author_name)));
			}

			// Timestamp
			// NOTE: Using CreationDate. (no ModificationDate property...)
			// TODO: Adjust for local timezone, since it's UTC.
			d->metaData.addMetaData_timestamp(Property::CreationDate,
				static_cast<rp_time_t>(kfhHeader->creation_time) + static_cast<rp_time_t>(GC_UNIX_TIME_DIFF));

			break;
		}
	}

	// Finished reading the metadata.
	return d->metaData.count();
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int FlipnoteStudio::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(FlipnoteStudio);

	ROMDATA_loadInternalImage_single(
		IMG_INT_IMAGE,	// ourImageType
		d->file,	// file
		d->isValid,	// isValid
		d->animType,	// romType
		d->img_image,	// imgCache
		d->loadImage);	// func
}

} // namespace LibRomData
