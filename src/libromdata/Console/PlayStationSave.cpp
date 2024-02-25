/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PlayStationSave.hpp: Sony PlayStation save file reader.                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * Copyright (c) 2017-2018 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - http://www.psdevwiki.com/ps3/Game_Saves#Game_Saves_PS1
// - http://problemkaputt.de/psx-spx.htm

#include "stdafx.h"
#include "PlayStationSave.hpp"
#include "ps1_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// C++ STL classes
using std::vector;

namespace LibRomData {

class PlayStationSavePrivate final : public RomDataPrivate
{
public:
	PlayStationSavePrivate(const IRpFilePtr &file);
	~PlayStationSavePrivate() final = default;

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(PlayStationSavePrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// Animated icon data
	IconAnimDataPtr iconAnimData;

public:
	// Save file type
	enum class SaveType {
		Unknown	= -1,

		PSV = 0,	// PS1 on PS3 individual save file.
		Raw,		// Raw blocks without header information
		Block,		// Prefixed by header of the first block (*.mcs, *.ps1)
		_54,		// Prefixed by 54-byte header (*.mcb, *.mcx, *.pda, *.psx)

		Max
	};
	SaveType saveType;

public:
	// Save file header
	union {
		PS1_PSV_Header psvHeader;
		PS1_Block_Entry blockHeader;
		PS1_54_Header ps54Header;
	} mxh;
	PS1_SC_Struct scHeader;

	/**
	 * Load the save file's icons.
	 *
	 * This will load all of the animated icon frames,
	 * though only the first frame will be returned.
	 *
	 * @return Icon, or nullptr on error.
	 */
	rp_image_const_ptr loadIcon(void);
};

ROMDATA_IMPL(PlayStationSave)
ROMDATA_IMPL_IMG(PlayStationSave)

/** PlayStationSavePrivate **/

/* RomDataInfo */
const char *const PlayStationSavePrivate::exts[] = {
	".psv",
	".mcb", ".mcx", ".pda", ".psx",
	".mcs", ".ps1",

	// TODO: support RAW?
	nullptr
};
const char *const PlayStationSavePrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-ps1-save",

	nullptr
};
const RomDataInfo PlayStationSavePrivate::romDataInfo = {
	"PlayStationSave", exts, mimeTypes
};

PlayStationSavePrivate::PlayStationSavePrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, iconAnimData(nullptr)
	, saveType(SaveType::Unknown)
{
	// Clear the various headers.
	memset(&mxh, 0, sizeof(mxh));
	memset(&scHeader, 0, sizeof(scHeader));
}

/**
 * Load the save file's icons.
 *
 * This will load all of the animated icon frames,
 * though only the first frame will be returned.
 *
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr PlayStationSavePrivate::loadIcon(void)
{
	if (iconAnimData) {
		// Icon has already been loaded.
		return iconAnimData->frames[0];
	}

	if ((int)saveType < 0) {
		// Invalid save type...
		return nullptr;
	}

	// Determine how many frames need to be decoded.
	int frames;
	int delay;	// in PAL frames
	switch (scHeader.icon_flag) {
		case PS1_SC_ICON_NONE:
		default:
			// No frames.
			return nullptr;

		case PS1_SC_ICON_STATIC:
		case PS1_SC_ICON_ALT_STATIC:
			// One frame.
			frames = 1;
			delay = 0;
			break;

		case PS1_SC_ICON_ANIM_2:
		case PS1_SC_ICON_ALT_ANIM_2:
			// Two frames.
			// Icon delay is 16 PAL frames.
			frames = 2;
			delay = 16;
			break;

		case PS1_SC_ICON_ANIM_3:
		case PS1_SC_ICON_ALT_ANIM_3:
			// Three frames.
			// Icon delay is 11 PAL frames.
			frames = 3;
			delay = 11;
			break;
	}

	this->iconAnimData = std::make_shared<IconAnimData>();
	iconAnimData->count = frames;
	iconAnimData->seq_count = frames;

	// Decode the icon frames.
	for (int i = 0; i < frames; i++) {
		iconAnimData->delays[i].numer = delay;
		iconAnimData->delays[i].denom = 50;
		iconAnimData->delays[i].ms = (delay * 1000 / 50);
		iconAnimData->seq_index[i] = i;

		// Icon format is linear 16x16 4bpp with RGB555 palette.
		iconAnimData->frames[i] = ImageDecoder::fromLinearCI4(
			ImageDecoder::PixelFormat::BGR555_PS1, false, 16, 16,
			scHeader.icon_data[i], sizeof(scHeader.icon_data[i]),
			scHeader.icon_pal, sizeof(scHeader.icon_pal));
	}


	// Return the first frame.
	return iconAnimData->frames[0];
}

/** PlayStationSave **/

/**
 * Read a PlayStation save file.
 *
 * A save file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
PlayStationSave::PlayStationSave(const IRpFilePtr &file)
	: super(new PlayStationSavePrivate(file))
{
	// This class handles save files.
	RP_D(PlayStationSave);
	d->mimeType = "application/x-ps1-save";	// unofficial, not on fd.o
	d->fileType = FileType::SaveFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the save file header.
	uint8_t header[1024];
	d->file->rewind();
	size_t size = d->file->read(&header, sizeof(header));
	if (size != sizeof(header)) {
		d->file.reset();
		return;
	}

	// Check if this ROM image is supported.
	const DetectInfo info = {
		{0, sizeof(header), header},
		nullptr,	// ext (not needed for PlayStationSave)
		d->file->size()	// szFile
	};
	d->saveType = static_cast<PlayStationSavePrivate::SaveType>(isRomSupported(&info));

	switch (d->saveType) {
		case PlayStationSavePrivate::SaveType::PSV:
			// PSV (PS1 on PS3)
			memcpy(&d->mxh.psvHeader, header, sizeof(d->mxh.psvHeader));
			memcpy(&d->scHeader, &header[sizeof(d->mxh.psvHeader)], sizeof(d->scHeader));
			break;
		case PlayStationSavePrivate::SaveType::Raw:
			memcpy(&d->scHeader, header, sizeof(d->scHeader));
			break;
		case PlayStationSavePrivate::SaveType::Block:
			memcpy(&d->mxh.blockHeader, header, sizeof(d->mxh.blockHeader));
			memcpy(&d->scHeader, &header[sizeof(d->mxh.blockHeader)], sizeof(d->scHeader));
			break;
		case PlayStationSavePrivate::SaveType::_54:
			memcpy(&d->mxh.ps54Header, header, sizeof(d->mxh.ps54Header));
			memcpy(&d->scHeader, &header[sizeof(d->mxh.ps54Header)], sizeof(d->scHeader));
			break;
		default:
			// Unknown save type.
			d->file.reset();
			d->saveType = PlayStationSavePrivate::SaveType::Unknown;
			return;
	}

	d->isValid = true;
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int PlayStationSave::isRomSupported_static(const DetectInfo *info)
{
	// NOTE: Only PSV is supported right now.
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(PS1_SC_Struct))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(PlayStationSavePrivate::SaveType::Unknown);
	}

	// Check for PSV+SC.
	if (info->header.size >= sizeof(PS1_PSV_Header) + sizeof(PS1_SC_Struct)) {
		// Check for SC magic.
		const PS1_SC_Struct *const scHeader =
			reinterpret_cast<const PS1_SC_Struct*>(
				&info->header.pData[sizeof(PS1_PSV_Header)]);
		if (scHeader->magic == cpu_to_be16(PS1_SC_MAGIC)) {
			// Check the PSV magic.
			const PS1_PSV_Header *const psvHeader =
				reinterpret_cast<const PS1_PSV_Header*>(info->header.pData);
			if (psvHeader->magic == cpu_to_be64(PS1_PSV_MAGIC)) {
				// This is a PSV (PS1 on PS3) save file.
				return static_cast<int>(PlayStationSavePrivate::SaveType::PSV);
			}

			// PSV magic is incorrect.
			return static_cast<int>(PlayStationSavePrivate::SaveType::Unknown);
		}
	}

	// Check for Block Entry + SC.
	if (info->header.size >= sizeof(PS1_Block_Entry) + sizeof(PS1_SC_Struct)) {
		// Check for SC magic.
		const PS1_SC_Struct *const scHeader =
			reinterpret_cast<const PS1_SC_Struct*>(
				&info->header.pData[sizeof(PS1_Block_Entry)]);
		if (scHeader->magic == cpu_to_be16(PS1_SC_MAGIC)) {
			const uint8_t *const header = info->header.pData;

			// Check the block magic.
			static const uint8_t block_magic[4] = {
				PS1_ENTRY_ALLOC_FIRST, 0x00, 0x00, 0x00,
			};
			if (memcmp(header, block_magic, sizeof(block_magic)) != 0) {
				// Block magic is incorrect.
				return static_cast<int>(PlayStationSavePrivate::SaveType::Unknown);
			}

			// Check the checksum
			uint8_t checksum = 0;
			for (int i = sizeof(PS1_Block_Entry)-1; i >= 0; i--) {
				checksum ^= header[i];
			}
			if (checksum != 0) return -1;

			return static_cast<int>(PlayStationSavePrivate::SaveType::Block);
		}
	}

	// Check for PS1 54.
	if (info->header.size >= sizeof(PS1_54_Header) + sizeof(PS1_SC_Struct)) {
		// Check for SC magic.
		const PS1_SC_Struct *const scHeader =
			reinterpret_cast<const PS1_SC_Struct*>(
				&info->header.pData[sizeof(PS1_54_Header)]);
		if (scHeader->magic == cpu_to_be16(PS1_SC_MAGIC)) {
			// Extra filesize check to prevent false-positives
			if (info->szFile % 8192 != 54) return -1;
			return static_cast<int>(PlayStationSavePrivate::SaveType::_54);
		}
	}

	// Check for PS1 SC by itself.
	const PS1_SC_Struct *const scHeader =
		reinterpret_cast<const PS1_SC_Struct*>(info->header.pData);
	if (scHeader->magic == cpu_to_be16(PS1_SC_MAGIC)) {
		// Extra filesize check to prevent false-positives
		if (info->szFile % 8192 == 0) {
			return static_cast<int>(PlayStationSavePrivate::SaveType::Raw);
		}
	}

	// Not supported.
	return static_cast<int>(PlayStationSavePrivate::SaveType::Unknown);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *PlayStationSave::systemName(unsigned int type) const
{
	RP_D(const PlayStationSave);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// PlayStation has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"PlayStationSave::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Sony PlayStation", "PlayStation", "PS1", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t PlayStationSave::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON;
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
vector<RomData::ImageSizeDef> PlayStationSave::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_ICON) {
		// Only icons are supported.
		return {};
	}

	// PlayStation save files have 16x16 icons.
	return {{nullptr, 16, 16, 0}};
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
uint32_t PlayStationSave::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_INT_ICON: {
			// Use nearest-neighbor scaling when resizing.
			// Also, need to check if this is an animated icon.
			RP_D(const PlayStationSave);
			const_cast<PlayStationSavePrivate*>(d)->loadIcon();
			if (d->iconAnimData && d->iconAnimData->count > 1) {
				// Animated icon.
				ret = IMGPF_RESCALE_NEAREST | IMGPF_ICON_ANIMATED;
			} else {
				// Not animated.
				ret = IMGPF_RESCALE_NEAREST;
			}
			break;
		}

		default:
			break;
	}
	return ret;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int PlayStationSave::loadFieldData(void)
{
	RP_D(PlayStationSave);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->saveType < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

	// PSV (PS1 on PS3) save file header.
	const PS1_SC_Struct *const scHeader = &d->scHeader;
	d->fields.reserve(2);	// Maximum of 2 fields.

	// Filename.
	const char* filename = nullptr;
	switch (d->saveType) {
		case PlayStationSavePrivate::SaveType::PSV:
			filename = d->mxh.psvHeader.filename;
			break;
		case PlayStationSavePrivate::SaveType::Block:
			filename = d->mxh.blockHeader.filename;
			break;
		case PlayStationSavePrivate::SaveType::_54:
			filename = d->mxh.ps54Header.filename;
			break;
		default:
			break;
	}

	if (filename) {
		d->fields.addField_string(C_("RomData", "Filename"),
			cp1252_sjis_to_utf8(filename, 20));
	}

	// Description.
	d->fields.addField_string(C_("PlayStationSave", "Description"),
		cp1252_sjis_to_utf8(scHeader->title, sizeof(scHeader->title)));

	// TODO: Moar fields.

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int PlayStationSave::loadMetaData(void)
{
	RP_D(PlayStationSave);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// PSV (PS1 on PS3) save file header.
	const PS1_SC_Struct *const scHeader = &d->scHeader;

	// Title. (Description)
	d->metaData->addMetaData_string(Property::Title,
		cp1252_sjis_to_utf8(scHeader->title, sizeof(scHeader->title)));

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int PlayStationSave::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(PlayStationSave);
	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported by PS1.
		pImage.reset();
		return -ENOENT;
	} else if (d->iconAnimData) {
		// Image has already been loaded.
		// NOTE: PS1 icon animations are always sequential,
		// so we can use a shortcut here.
		pImage = d->iconAnimData->frames[0];
		return 0;
	} else if (!d->file) {
		// File isn't open.
		pImage.reset();
		return -EBADF;
	} else if (!d->isValid) {
		// Save file isn't valid.
		pImage.reset();
		return -EIO;
	}

	// Load the icon.
	// TODO: -ENOENT if the file doesn't actually have an icon.
	pImage = d->loadIcon();
	return ((bool)pImage ? 0 : -EIO);
}

/**
 * Get the animated icon data.
 *
 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
 * object has an animated icon.
 *
 * @return Animated icon data, or nullptr if no animated icon is present.
 */
IconAnimDataConstPtr PlayStationSave::iconAnimData(void) const
{
	RP_D(const PlayStationSave);
	if (!d->iconAnimData) {
		// Load the icon.
		if (!const_cast<PlayStationSavePrivate*>(d)->loadIcon()) {
			// Error loading the icon.
			return nullptr;
		}
		if (!d->iconAnimData) {
			// Still no icon...
			return nullptr;
		}
	}

	if (d->iconAnimData->count <= 1) {
		// Not an animated icon.
		return nullptr;
	}

	// Return the icon animation data.
	return d->iconAnimData;
}

}
