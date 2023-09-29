/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DreamcastSave.cpp: Sega Dreamcast save file reader.                     *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DreamcastSave.hpp"
#include "dc_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// C++ STL classes
using std::string;
using std::vector;

namespace LibRomData {

class DreamcastSavePrivate final : public RomDataPrivate
{
	public:
		DreamcastSavePrivate(const IRpFilePtr &file);
		~DreamcastSavePrivate() final = default;

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(DreamcastSavePrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// Internal images
		rp_image_ptr img_banner;

		// Animated icon data
		IconAnimDataPtr iconAnimData;

	public:
		// Save file type.
		// Applies to the main file, e.g. VMS or DCI.
		enum class SaveType {
			Unknown	= -1,

			VMS	= 0,	// VMS file (also .VMI+.VMS)
			VMI	= 1,	// VMI file (standalone)
			DCI	= 2,	// DCI (Nexus)

			Max
		};
		SaveType saveType;

	public:
		// Which headers do we have loaded?
		enum DC_LoadedHeaders {
			DC_HAVE_UNKNOWN = 0,

			// VMS data. Present in .VMS and .DCI files.
			DC_HAVE_VMS = (1U << 0),

			// VMI header. Present in .VMI files only.
			DC_HAVE_VMI = (1U << 1),

			// Directory entry. Present in .VMI and .DCI files.
			DC_HAVE_DIR_ENTRY = (1U << 2),

			// ICONDATA_VMS.
			DC_IS_ICONDATA_VMS = (1U << 3),
		};
		uint32_t loaded_headers;

		// VMI save file. (for .VMI+.VMS)
		// NOTE: Standalone VMI uses this->file.
		IRpFilePtr vmi_file;

		// Offset in the main file to the data area.
		// - VMS: 0
		// - DCI: 32
		uint32_t data_area_offset;
		static const uint32_t DATA_AREA_OFFSET_VMS = 0;
		static const uint32_t DATA_AREA_OFFSET_DCI = 32;

		/** NOTE: Fields have been byteswapped when loaded. **/
		// VMS header.
		DC_VMS_Header vms_header;
		// Header offset. (0 for standard save files; 0x200 for game files.)
		uint32_t vms_header_offset;
		// VMI header.
		DC_VMI_Header vmi_header;
		// Directory entry.
		DC_VMS_DirEnt vms_dirent;

		// Creation time. Converted from binary or BCD,
		// depending on if we loaded a VMI or DCI.
		// If the original value is invalid, this will
		// be set to -1.
		time_t ctime;

		// Time conversion functions.
		static time_t vmi_to_unix_time(const DC_VMI_Timestamp *vmi_tm);

		// Is this a VMS game file?
		bool isGameFile;

		/**
		 * Read and verify the VMS header.
		 * This function sets vms_header and vms_header_offset.
		 * @param address Address to check.
		 * @return DC_LoadedHeaders flag if read and verified; 0 if not.
		 */
		unsigned int readAndVerifyVmsHeader(uint32_t address);

		/**
		 * Read the VMI header from the specified file.
		 * @param vmi_file VMI file.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int readVmiHeader(const IRpFilePtr &vmi_file);

		// Graphic eyecatch sizes.
		static const uint32_t eyecatch_sizes[4];

		// VMS icon struct.
		// For processing VMS icons only;
		// do not use directly for saving!
		// NOTE: If using `icon_mono`, don't use `palette` or `icon_color`.
		union VmsIcon_buf_t {
			struct {
				union {
					uint16_t u16[DC_VMS_ICON_PALETTE_SIZE >> 1];
					uint32_t u32[DC_VMS_ICON_PALETTE_SIZE >> 2];
				} palette;
				union {
					uint8_t   u8[DC_VMS_ICON_DATA_SIZE];
					uint32_t u32[DC_VMS_ICON_DATA_SIZE >> 2];
				} icon_color;
			};
			union {
				uint8_t   u8[DC_VMS_ICONDATA_MONO_ICON_SIZE];
				uint32_t u32[DC_VMS_ICONDATA_MONO_ICON_SIZE >> 2];
			} icon_mono;
		};

		/**
		 * Load the save file's icons.
		 *
		 * This will load all of the animated icon frames,
		 * though only the first frame will be returned.
		 *
		 * @return Icon, or nullptr on error.
		 */
		rp_image_const_ptr loadIcon(void);

		/**
		 * Load the icon from an ICONDATA_VMS file.
		 *
		 * If a color icon is present, that will be loaded.
		 * Otherwise, the monochrome icon will be loaded.
		 *
		 * @return Icon, or nullptr on error.
		 */
		rp_image_const_ptr loadIcon_ICONDATA_VMS(void);

		/**
		 * Load the save file's banner.
		 * @return Banner, or nullptr on error.
		 */
		rp_image_const_ptr loadBanner(void);
};

ROMDATA_IMPL(DreamcastSave)

/** DreamcastSavePrivate **/

/* RomDataInfo */
const char *const DreamcastSavePrivate::exts[] = {
	".vms", ".vmi", ".dci",

	nullptr
};
const char *const DreamcastSavePrivate::mimeTypes[] = {
	// NOTE: Ordering matches SaveType.

	// Unofficial MIME types used by Sega.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-dreamcast-vms",		// .vms
	"application/x-dreamcast-vms-info",	// .vmi

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-dreamcast-dci",

	nullptr
};
const RomDataInfo DreamcastSavePrivate::romDataInfo = {
	"DreamcastSave", exts, mimeTypes
};

// Graphic eyecatch sizes.
const uint32_t DreamcastSavePrivate::eyecatch_sizes[4] = {
	0,	// DC_VMS_EYECATCH_NONE
	DC_VMS_EYECATCH_ARGB4444_DATA_SIZE,
	DC_VMS_EYECATCH_CI8_PALETTE_SIZE + DC_VMS_EYECATCH_CI8_DATA_SIZE,
	DC_VMS_EYECATCH_CI4_PALETTE_SIZE + DC_VMS_EYECATCH_CI4_DATA_SIZE
};

DreamcastSavePrivate::DreamcastSavePrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, saveType(SaveType::Unknown)
	, loaded_headers(0)
	, vmi_file(nullptr)
	, data_area_offset(0)
	, vms_header_offset(0)
	, ctime(-1)
	, isGameFile(false)
{
	// Clear the various structs.
	memset(&vms_header, 0, sizeof(vms_header));
	memset(&vmi_header, 0, sizeof(vmi_header));
	memset(&vms_dirent, 0, sizeof(vms_dirent));
}

/**
 * Convert a VMI timestamp to Unix time.
 * @param vmi_tm VMI BCD timestamp.
 * @return Unix time, or -1 if an error occurred.
 *
 * NOTE: -1 is a valid Unix timestamp (1970/01/01), but is
 * not likely to be valid for Dreamcast, since Dreamcast
 * was released in 1998.
 *
 * NOTE: vmi_tm->year must have been byteswapped prior to
 * calling this function.
 */
time_t DreamcastSavePrivate::vmi_to_unix_time(const DC_VMI_Timestamp *vmi_tm)
{
	// Convert the VMI time to Unix time.
	// NOTE: struct tm has some oddities:
	// - tm_year: year - 1900
	// - tm_mon: 0 == January
	struct tm dctime;

	dctime.tm_year = vmi_tm->year - 1900;
	dctime.tm_mon  = vmi_tm->mon - 1;
	dctime.tm_mday = vmi_tm->mday;
	dctime.tm_hour = vmi_tm->hour;
	dctime.tm_min  = vmi_tm->min;
	dctime.tm_sec  = vmi_tm->sec;

	// tm_wday and tm_yday are output variables.
	dctime.tm_wday = 0;
	dctime.tm_yday = 0;
	dctime.tm_isdst = 0;

	// If conversion fails, d->ctime will be set to -1.
	return timegm(&dctime);
}

/**
 * Read and verify the VMS header.
 * This function sets vms_header and vms_header_offset.
 * @param address Address to check.
 * @return DC_LoadedHeaders flag if read and verified; 0 if not.
 */
unsigned int DreamcastSavePrivate::readAndVerifyVmsHeader(uint32_t address)
{
	size_t size = file->seekAndRead(address, &vms_header, sizeof(vms_header));
	if (size != sizeof(vms_header)) {
		// Seek and/or read error.
		return DC_HAVE_UNKNOWN;
	}

	// Validate the description fields.
	// The description fields cannot contain any control characters
	// other than 0x00 (NULL). In the case of a game file, the first
	// 512 bytes is program code, so there will almost certainly be
	// some control character.
	// In addition, the first 8 characters of each field must not be NULL.
	// NOTE: Need to use unsigned here.
#define CHECK_FIELD(field) \
	do { \
		const uint8_t *chr = reinterpret_cast<const uint8_t*>(field); \
		for (int i = 8; i > 0; i--, chr++) { \
			/* First 8 characters must not be a control code or NULL. */ \
			if (*chr < 0x20) { \
				/* Invalid character. */ \
				return DC_HAVE_UNKNOWN; \
			} \
		} \
		for (int i = ARRAY_SIZE(field)-8; i > 0; i--, chr++) { \
			/* Remaining characters must not be a control code, * \
			 * but may be NULL.                                 */ \
			if (*chr < 0x20 && *chr != 0) { \
				/* Invalid character. */ \
				return DC_HAVE_UNKNOWN; \
			} \
		} \
	} while (0)
	CHECK_FIELD(vms_header.vms_description);

	// Check for ICONDATA_VMS.
	// Monochrome icon is usually within the first 256 bytes
	// of the start of the file.
	if ((loaded_headers & DC_IS_ICONDATA_VMS) ||
	    ((unsigned int)vms_header.dc_description[0] >= sizeof(DC_VMS_ICONDATA_Header) &&
	     vms_header.dc_description[1] == 0 &&
	     vms_header.dc_description[2] == 0 &&
	     vms_header.dc_description[3] == 0))
	{
		// This is probably ICONDATA_VMS.
		if (this->saveType == SaveType::DCI) {
			rp_byte_swap_32_array(vms_header.dci_dword, sizeof(vms_header.icondata_vms));
		}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		// Byteswap the fields.
		vms_header.icondata_vms.mono_icon_addr = le32_to_cpu(vms_header.icondata_vms.mono_icon_addr);
		vms_header.icondata_vms.color_icon_addr = le32_to_cpu(vms_header.icondata_vms.color_icon_addr);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

		this->vms_header_offset = address;
		return DC_IS_ICONDATA_VMS;
	}

	CHECK_FIELD(vms_header.dc_description);

	// Description fields are valid.

	// If DCI, the entire vms_header must be 32-bit byteswapped first.
	if (this->saveType == SaveType::DCI) {
		rp_byte_swap_32_array(vms_header.dci_dword, sizeof(vms_header.dci_dword));
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the fields.
	vms_header.icon_count      = le16_to_cpu(vms_header.icon_count);
	vms_header.icon_anim_speed = le16_to_cpu(vms_header.icon_anim_speed);
	vms_header.eyecatch_type   = le16_to_cpu(vms_header.eyecatch_type);
	vms_header.crc             = le16_to_cpu(vms_header.crc);
	vms_header.data_size       = le32_to_cpu(vms_header.data_size);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// VMS header loaded.
	this->vms_header_offset = address;
	return DC_HAVE_VMS;
}

/**
 * Read the VMI header from the specified file.
 * @param vmi_file VMI file.
 * @return 0 on success; negative POSIX error code on error.
 */
int DreamcastSavePrivate::readVmiHeader(const IRpFilePtr &vmi_file)
{
	// NOTE: vmi_file shadows this->vmi_file.

	// Read the VMI header.
	vmi_file->rewind();
	size_t size = vmi_file->read(&vmi_header, sizeof(vmi_header));
	if (size != sizeof(vmi_header)) {
		// Read error.
		int lastError = vmi_file->lastError();
		return (lastError != 0 ? -lastError : -EIO);
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the VMI header.
	vmi_header.ctime.year  = le16_to_cpu(vmi_header.ctime.year);
	vmi_header.vmi_version = le16_to_cpu(vmi_header.vmi_version);
	vmi_header.mode        = le16_to_cpu(vmi_header.mode);
	vmi_header.reserved    = le16_to_cpu(vmi_header.reserved);
	vmi_header.filesize    = le32_to_cpu(vmi_header.filesize);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// VMI header is loaded.
	loaded_headers |= DreamcastSavePrivate::DC_HAVE_VMI;

	// Convert the VMI time to Unix time.
	this->ctime = vmi_to_unix_time(&vmi_header.ctime);

	// File size, in blocks.
	const unsigned int blocks = (vmi_header.filesize / DC_VMS_BLOCK_SIZE);

	// Convert to a directory entry.
	if (vmi_header.mode & DC_VMI_MODE_FTYPE_MASK) {
		// Game file.
		vms_dirent.filetype = DC_VMS_DIRENT_FTYPE_GAME;
		vms_dirent.header_addr = 1;
	} else {
		// Data file.
		vms_dirent.filetype = DC_VMS_DIRENT_FTYPE_DATA;
		vms_dirent.header_addr = 0;
	}
	vms_dirent.protect = ((vmi_header.mode & DC_VMI_MODE_PROTECT_MASK)
					? DC_VMS_DIRENT_PROTECT_COPY_PROTECTED
					: DC_VMS_DIRENT_PROTECT_COPY_OK);
	vms_dirent.address = 200 - blocks;	// Fake starting address.
	memcpy(vms_dirent.filename, vmi_header.vms_filename, DC_VMS_FILENAME_LENGTH);
	// TODO: Convert the timestamp to BCD?
	vms_dirent.size = blocks;
	memset(vms_dirent.reserved, 0, sizeof(vms_dirent.reserved));

	// VMI header loaded.
	loaded_headers |= DreamcastSavePrivate::DC_HAVE_DIR_ENTRY;
	return 0;
}

/**
 * Load the save file's icons.
 *
 * This will load all of the animated icon frames,
 * though only the first frame will be returned.
 *
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr DreamcastSavePrivate::loadIcon(void)
{
	if (iconAnimData) {
		// Icon has already been loaded.
		return iconAnimData->frames[0];
	} else if (!this->file || !this->isValid) {
		// Can't load the icon.
		return nullptr;
	}

	if (loaded_headers & DC_IS_ICONDATA_VMS) {
		// Special handling for ICONDATA_VMS.
		return loadIcon_ICONDATA_VMS();
	} else if (!(loaded_headers & DC_HAVE_VMS)) {
		// No VMS header. Cannot load the icon.
		return nullptr;
	}

	// Check the icon count.
	uint16_t icon_count = vms_header.icon_count;
	if (icon_count == 0) {
		// No icon.
		return nullptr;
	} else if (icon_count > 3) {
		// VMU files have a maximum of 3 frames.
		// Truncate the frame count.
		icon_count = 3;
	}

	// Sanity check: Each icon is 512 bytes, plus a 32-byte palette.
	// Make sure the file is big enough.
	uint32_t sz_reserved = vms_header_offset +
		static_cast<uint32_t>(sizeof(vms_header)) +
		DC_VMS_ICON_PALETTE_SIZE +
		(icon_count * DC_VMS_ICON_DATA_SIZE);
	if (vms_header.eyecatch_type <= 3) {
		sz_reserved += eyecatch_sizes[vms_header.eyecatch_type];
	}
	if (static_cast<off64_t>(sz_reserved) > this->file->size()) {
		// File is NOT big enough.
		return nullptr;
	}

	// Temporary icon buffer.
	VmsIcon_buf_t buf;

	// Load the palette.
	size_t size = file->seekAndRead(vms_header_offset + static_cast<uint32_t>(sizeof(vms_header)),
					buf.palette.u16, sizeof(buf.palette.u16));
	if (size != sizeof(buf.palette.u16)) {
		// Seek and/or read error.
		return nullptr;
	}

	if (this->saveType == SaveType::DCI) {
		// Apply 32-bit byteswapping to the palette.
		rp_byte_swap_32_array(buf.palette.u32, sizeof(buf.palette.u32));
	}

	this->iconAnimData = std::make_shared<IconAnimData>();
	iconAnimData->count = 0;

	// icon_anim_speed is in units of 1/30th of a second.
	const IconAnimData::delay_t delay = {
		vms_header.icon_anim_speed, 30,
		(vms_header.icon_anim_speed * 100) / 30
	};

	// Load the icons. (32x32, 4bpp)
	// Icons are stored contiguously immediately after the palette.
	for (int i = 0; i < icon_count; i++) {
		size = file->read(buf.icon_color.u8, sizeof(buf.icon_color.u8));
		if (size != sizeof(buf.icon_color)) {
			// Read error.
			break;
		}

		if (this->saveType == SaveType::DCI) {
			// Apply 32-bit byteswapping to the palette.
			rp_byte_swap_32_array(buf.icon_color.u32, sizeof(buf.icon_color.u32));
		}

		iconAnimData->delays[i] = delay;
		iconAnimData->frames[i] = ImageDecoder::fromLinearCI4(
			ImageDecoder::PixelFormat::ARGB4444, true,
			DC_VMS_ICON_W, DC_VMS_ICON_H,
			buf.icon_color.u8, sizeof(buf.icon_color.u8),
			buf.palette.u16, sizeof(buf.palette.u16));
		if (!iconAnimData->frames[i])
			break;

		// Icon loaded.
		iconAnimData->count++;
	}

	// NOTE: We're not deleting iconAnimData even if we only have
	// a single icon because iconAnimData() will call loadIcon()
	// if iconAnimData is nullptr.

	// Set up the icon animation sequence.
	for (int i = 0; i < iconAnimData->count; i++) {
		iconAnimData->seq_index[i] = i;
	}
	iconAnimData->seq_count = iconAnimData->count;

	// Return the first frame.
	return iconAnimData->frames[0];
}

/**
 * Load the icon from an ICONDATA_VMS file.
 *
 * If a color icon is present, that will be loaded.
 * Otherwise, the monochrome icon will be loaded.
 *
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr DreamcastSavePrivate::loadIcon_ICONDATA_VMS(void)
{
	if (iconAnimData) {
		// Icon has already been loaded.
		return iconAnimData->frames[0];
	} else if (!this->file || !this->isValid) {
		// Can't load the icon.
		return nullptr;
	}

	if (!(loaded_headers & DC_IS_ICONDATA_VMS)) {
		// Not ICONDATA_VMS.
		return nullptr;
	}

	// NOTE: We need to set up iconAnimData in order to ensure
	// this icon is deleted when the DreamcastSave is deleted.
	this->iconAnimData = std::make_shared<IconAnimData>();
	iconAnimData->count = 1;
	iconAnimData->seq_index[0] = 0;
	iconAnimData->delays[0].numer = 0;
	iconAnimData->delays[0].denom = 0;
	iconAnimData->delays[0].ms = 0;
	iconAnimData->frames[0] = nullptr;

	// Temporary icon buffer.
	VmsIcon_buf_t buf;

	// Do we have a color icon?
	if (vms_header.icondata_vms.color_icon_addr >= sizeof(vms_header.icondata_vms)) {
		// We have a color icon.

		// Load the palette.
		size_t size = file->seekAndRead(vms_header_offset + vms_header.icondata_vms.color_icon_addr,
						buf.palette.u16, sizeof(buf.palette.u16));
		if (size != sizeof(buf.palette.u16)) {
			// Seek and/or read error.
			return nullptr;
		}

		if (this->saveType == SaveType::DCI) {
			// Apply 32-bit byteswapping to the palette.
			rp_byte_swap_32_array(buf.palette.u32, sizeof(buf.palette.u32));
		}

		// Load the icon data.
		size = file->read(buf.icon_color.u8, sizeof(buf.icon_color.u8));
		if (size != sizeof(buf.icon_color.u8)) {
			// Read error.
			return nullptr;
		}

		if (this->saveType == SaveType::DCI) {
			// Apply 32-bit byteswapping to the icon data.
			rp_byte_swap_32_array(buf.icon_color.u32, sizeof(buf.icon_color.u32));
		}

		// Convert the icon to rp_image.
		const rp_image_ptr img = ImageDecoder::fromLinearCI4(
			ImageDecoder::PixelFormat::ARGB4444, true,
			DC_VMS_ICON_W, DC_VMS_ICON_H,
			buf.icon_color.u8, sizeof(buf.icon_color.u8),
			buf.palette.u16, sizeof(buf.palette.u16));
		if (img) {
			// Icon converted successfully.
			iconAnimData->frames[0] = img;
			return img;
		}
	}

	// We don't have a color icon.
	// Load the monochrome icon.
	size_t size = file->seekAndRead(vms_header_offset + vms_header.icondata_vms.mono_icon_addr,
					buf.icon_mono.u8, sizeof(buf.icon_mono.u8));
	if (size != sizeof(buf.icon_mono.u8)) {
		// Seek and/or read error.
		return nullptr;
	}

	if (this->saveType == SaveType::DCI) {
		// Apply 32-bit byteswapping to the icon data.
		rp_byte_swap_32_array(buf.icon_mono.u32, sizeof(buf.icon_mono.u32));
	}

	// Convert the icon to rp_image.
	const rp_image_ptr img = ImageDecoder::fromLinearMono(
		DC_VMS_ICON_W, DC_VMS_ICON_H,
		buf.icon_mono.u8, sizeof(buf.icon_mono.u8));
	if (img) {
		// Adjust the palette to use a more
		// VMU-like color scheme.
		assert(img->palette_len() >= 2);
		if (img->palette_len() < 2) {
			// Can't adjust the palette...
			// Just return it as-is.
			return img;
		}

		uint32_t *const palette = img->palette();
		assert(palette != nullptr);
		if (palette) {
			palette[0] = 0xFF8CCEAD;	// Green
			palette[1] = 0xFF081884;	// Blue
		}

		iconAnimData->frames[0] = img;
	}

	// Return the ICONDATA_VMS image.
	return img;
}

/**
 * Load the save file's banner.
 * @return Banner, or nullptr on error.
 */
rp_image_const_ptr DreamcastSavePrivate::loadBanner(void)
{
	if (img_banner) {
		// Banner is already loaded.
		return img_banner;
	} else if (!this->file || !this->isValid) {
		// Can't load the banner.
		return nullptr;
	}

	if (!(loaded_headers & DC_HAVE_VMS)) {
		// No VMS header. Cannot load the banner.
		return nullptr;
	}

	// Determine the eyecatch size.
	if (vms_header.eyecatch_type == DC_VMS_EYECATCH_NONE ||
	    vms_header.eyecatch_type > DC_VMS_EYECATCH_CI4)
	{
		// No eyecatch.
		return nullptr;
	}

	const unsigned int eyecatch_size = eyecatch_sizes[vms_header.eyecatch_type];

	// Skip over the icons.
	// Sanity check: Each icon is 512 bytes, plus a 32-byte palette.
	// Make sure the file is big enough.
	const uint32_t sz_icons = static_cast<uint32_t>(sizeof(vms_header)) +
		DC_VMS_ICON_PALETTE_SIZE +
		(vms_header.icon_count * DC_VMS_ICON_DATA_SIZE);
	if (static_cast<off64_t>(sz_icons) + eyecatch_size > file->size()) {
		// File is NOT big enough.
		return nullptr;
	}

	// Load the eyecatch data.
	auto data = aligned_uptr<uint8_t>(16, eyecatch_size);
	file->seek(vms_header_offset + sz_icons);
	size_t size = file->read(data.get(), eyecatch_size);
	if (size != eyecatch_size) {
		// Error loading the eyecatch data.
		return nullptr;
	}

	if (this->saveType == SaveType::DCI) {
		// Apply 32-bit byteswapping to the eyecatch data.
		rp_byte_swap_32_array(reinterpret_cast<uint32_t*>(data.get()), eyecatch_size);
	}

	// Convert the eycatch to rp_image.
	switch (vms_header.eyecatch_type) {
		case DC_VMS_EYECATCH_NONE:
		default:
			// Invalid eyecatch type.
			break;

		case DC_VMS_EYECATCH_ARGB4444: {
			// ARGB4444 eyecatch.
			// FIXME: Completely untested.
			img_banner = ImageDecoder::fromLinear16(
				ImageDecoder::PixelFormat::ARGB4444,
				DC_VMS_EYECATCH_W, DC_VMS_EYECATCH_H,
				reinterpret_cast<const uint16_t*>(data.get()), DC_VMS_EYECATCH_ARGB4444_DATA_SIZE);
			break;
		}

		case DC_VMS_EYECATCH_CI8: {
			// CI8 eyecatch.
			// TODO: Needs more testing.
			const uint8_t *image_buf = data.get() + DC_VMS_EYECATCH_CI8_PALETTE_SIZE;
			img_banner = ImageDecoder::fromLinearCI8(
				ImageDecoder::PixelFormat::ARGB4444,
				DC_VMS_EYECATCH_W, DC_VMS_EYECATCH_H,
				image_buf, DC_VMS_EYECATCH_CI8_DATA_SIZE,
				reinterpret_cast<const uint16_t*>(data.get()), DC_VMS_EYECATCH_CI8_PALETTE_SIZE);
			break;
		}

		case DC_VMS_EYECATCH_CI4: {
			// CI4 eyecatch.
			const uint8_t *image_buf = data.get() + DC_VMS_EYECATCH_CI4_PALETTE_SIZE;
			img_banner = ImageDecoder::fromLinearCI4(
				ImageDecoder::PixelFormat::ARGB4444, true,
				DC_VMS_EYECATCH_W, DC_VMS_EYECATCH_H,
				image_buf, DC_VMS_EYECATCH_CI4_DATA_SIZE,
				reinterpret_cast<const uint16_t*>(data.get()), DC_VMS_EYECATCH_CI4_PALETTE_SIZE);
			break;
		}
	}

	return img_banner;
}

/** DreamcastSave **/

/**
 * Read a Sega Dreamcast save file.
 *
 * A save file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid save file.
 *
 * @param file Open disc image.
 */
DreamcastSave::DreamcastSave(const IRpFilePtr &file)
	: super(new DreamcastSavePrivate(file))
{
	// This class handles save files.
	RP_D(DreamcastSave);
	d->fileType = FileType::SaveFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	static_assert(DC_VMS_ICON_PALETTE_SIZE == 32,
		"DC_VMS_ICON_PALETTE_SIZE is wrong. (Should be 32.)");
	static_assert(DC_VMS_ICON_DATA_SIZE == 512,
		"DC_VMS_ICON_DATA_SIZE is wrong. (Should be 512.)");

	static_assert(DC_VMS_EYECATCH_ARGB4444_DATA_SIZE == 8064,
		"DC_VMS_EYECATCH_ARGB4444_DATA_SIZE is wrong. (Should be 8,064.)");
	static_assert(DC_VMS_EYECATCH_CI8_PALETTE_SIZE + DC_VMS_EYECATCH_CI8_DATA_SIZE == 4544,
		"DC_VMS_EYECATCH_CI8_PALETTE_SIZE + DC_VMS_EYECATCH_CI8_DATA_SIZE is wrong. (Should be 4,544.)");
	static_assert(DC_VMS_EYECATCH_CI4_PALETTE_SIZE + DC_VMS_EYECATCH_CI4_DATA_SIZE == 2048,
		"DC_VMS_EYECATCH_CI4_PALETTE_SIZE + DC_VMS_EYECATCH_CI4_DATA_SIZE is wrong. (Should be 2,048.)");

	// Determine the VMS save type by checking the file size.
	// Standard VMS is always a multiple of DC_VMS_BLOCK_SIZE.
	// DCI is a multiple of DC_VMS_BLOCK_SIZE, plus 32 bytes.
	// NOTE: May be DC_VMS_ICONDATA_MONO_MINSIZE for ICONDATA_VMS.
	const off64_t fileSize = d->file->size();
	if (fileSize % DC_VMS_BLOCK_SIZE == 0 ||
	    fileSize == DC_VMS_ICONDATA_MONO_MINSIZE)
	{
		// VMS file.
		d->saveType = DreamcastSavePrivate::SaveType::VMS;
		d->data_area_offset = DreamcastSavePrivate::DATA_AREA_OFFSET_VMS;
	}
	else if ((fileSize - 32) % DC_VMS_BLOCK_SIZE == 0 ||
		 (fileSize - 32) == DC_VMS_ICONDATA_MONO_MINSIZE)
	{
		d->saveType = DreamcastSavePrivate::SaveType::DCI;
		d->data_area_offset = DreamcastSavePrivate::DATA_AREA_OFFSET_DCI;

		// Load the directory entry.
		d->file->rewind();
		size_t size = d->file->read(&d->vms_dirent, sizeof(d->vms_dirent));
		if (size != sizeof(d->vms_dirent)) {
			// Read error.
			d->file.reset();
			return;
		}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		// Byteswap the directory entry.
		d->vms_dirent.address     = le16_to_cpu(d->vms_dirent.address);
		d->vms_dirent.size        = le16_to_cpu(d->vms_dirent.size);
		d->vms_dirent.header_addr = le16_to_cpu(d->vms_dirent.header_addr);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

		d->isGameFile = !!(d->vms_dirent.filetype == DC_VMS_DIRENT_FTYPE_GAME);
		d->loaded_headers |= DreamcastSavePrivate::DC_HAVE_DIR_ENTRY;

		// Is this ICONDATA_VMS?
		if (!strncmp(d->vms_dirent.filename, "ICONDATA_VMS", sizeof(d->vms_dirent.filename))) {
			// This is ICONDATA_VMS.
			d->loaded_headers |= DreamcastSavePrivate::DC_IS_ICONDATA_VMS;
		}
	} else if (fileSize == sizeof(DC_VMI_Header)) {
		// Standalone VMI file.
		d->saveType = DreamcastSavePrivate::SaveType::VMI;
		d->data_area_offset = DreamcastSavePrivate::DATA_AREA_OFFSET_VMS;

		// Load the VMI header.
		int ret = d->readVmiHeader(d->file);
		if (ret != 0) {
			// Read error.
			d->file.reset();
			return;
		}

		// Nothing else to do here for standalone VMI files.
		d->mimeType = d->mimeTypes[(int)d->saveType];
		d->isValid = true;
		return;
	} else {
		// Not valid.
		d->saveType = DreamcastSavePrivate::SaveType::Unknown;
		d->file.reset();
		return;
	}

	// Set the MIME type.
	d->mimeType = d->mimeTypes[(int)d->saveType];

	// TODO: Load both VMI and VMS timestamps?
	// Currently, only the VMS timestamp is loaded.

	// Read the save file header.
	// Regular save files have the header in block 0.
	// Game files have the header in block 1.
	if (d->loaded_headers & DreamcastSavePrivate::DC_HAVE_DIR_ENTRY) {
		// Use the header address specified in the directory entry.
		const unsigned int headerLoaded = d->readAndVerifyVmsHeader(
			d->data_area_offset + (d->vms_dirent.header_addr * DC_VMS_BLOCK_SIZE));
		if (headerLoaded != DreamcastSavePrivate::DC_HAVE_UNKNOWN) {
			// Valid VMS header.
			d->loaded_headers |= headerLoaded;
		} else {
			// Not valid.
			d->file.reset();
			return;
		}

		// Convert the VMS BCD time to Unix time.
		d->ctime = d->bcd_to_unix_time(
			reinterpret_cast<const uint8_t*>(&d->vms_dirent.ctime),
			sizeof(d->vms_dirent.ctime));
	} else {
		// If the VMI file is not available, we'll use a heuristic:
		// The description fields cannot contain any control
		// characters other than 0x00 (NULL).
		unsigned int headerLoaded = d->readAndVerifyVmsHeader(d->data_area_offset);
		if (headerLoaded != DreamcastSavePrivate::DC_HAVE_UNKNOWN) {
			// Valid in block 0: This is a standard save file.
			d->isGameFile = false;
			d->loaded_headers |= headerLoaded;
		} else {
			headerLoaded = d->readAndVerifyVmsHeader(d->data_area_offset + DC_VMS_BLOCK_SIZE);
			if (headerLoaded != DreamcastSavePrivate::DC_HAVE_UNKNOWN) {
				// Valid in block 1: This is a game file.
				d->isGameFile = true;
				d->loaded_headers |= headerLoaded;
			} else {
				// Not valid.
				d->file.reset();
				return;
			}
		}
	}

	// TODO: Verify the file extension and header fields?
	d->isValid = true;
}

/**
 * Read a Sega Dreamcast save file. (.VMI+.VMS pair)
 *
 * This constructor requires two files:
 * - .VMS file (main save file)
 * - .VMI file (directory entry)
 *
 * Both files will be ref()'d.
 * The .VMS file will be used as the main file for the RomData class.
 *
 * To close the files, either delete this object or call close().
 * NOTE: Check isValid() to determine if this is a valid save file.
 *
 * @param vms_file Open .VMS save file.
 * @param vmi_file Open .VMI save file.
 */
DreamcastSave::DreamcastSave(const IRpFilePtr &vms_file, const IRpFilePtr &vmi_file)
	: super(new DreamcastSavePrivate(vms_file))
{
	// This class handles save files.
	RP_D(DreamcastSave);
	d->fileType = FileType::SaveFile;

	if (!d->file || !vmi_file) {
		// Could not ref() the file handle.
		return;
	}

	// ref() the VMI file.
	d->vmi_file = vmi_file;

	// Sanity check:
	// - VMS file should be a multiple of 512 bytes,
	//   or 160 bytes for some monochrome ICONDATA_VMS.
	// - VMI file should be 108 bytes.
	const off64_t vms_fileSize = d->file->size();
	const off64_t vmi_fileSize = d->vmi_file->size();
	if (((vms_fileSize % 512 != 0) && vms_fileSize != DC_VMS_ICONDATA_MONO_MINSIZE) ||
	      vmi_fileSize != sizeof(DC_VMI_Header))
	{
		// Invalid file(s).
		d->vmi_file.reset();
		d->file.reset();
		return;
	}

	// Initialize the save type and data area offset.
	d->saveType = DreamcastSavePrivate::SaveType::VMS;
	d->data_area_offset = DreamcastSavePrivate::DATA_AREA_OFFSET_VMS;

	// Read the VMI header and copy it to the directory entry.
	// TODO: Verify that the file size from vmi_header matches
	// the actual VMS file size? (also vms_dirent.address)
	int ret = d->readVmiHeader(d->vmi_file);
	if (ret != 0) {
		// Error reading the VMI header.
		d->vmi_file.reset();
		d->file.reset();
		return;
	}

	// Is this ICONDATA_VMS?
	if (!strncmp(d->vms_dirent.filename, "ICONDATA_VMS", sizeof(d->vms_dirent.filename))) {
		// This is ICONDATA_VMS.
		d->loaded_headers |= DreamcastSavePrivate::DC_IS_ICONDATA_VMS;
	} else {
		// Load the VMS header.
		// Use the header address specified in the directory entry.
		const unsigned int headerLoaded = d->readAndVerifyVmsHeader(
			d->data_area_offset + (d->vms_dirent.header_addr * DC_VMS_BLOCK_SIZE));
		if (headerLoaded != DreamcastSavePrivate::DC_HAVE_UNKNOWN) {
			// Valid VMS header.
			d->loaded_headers |= headerLoaded;
		} else {
			// Not valid.
			d->vmi_file.reset();
			d->file.reset();
			return;
		}
	}

	// TODO: Verify the file extension and header fields?
	d->isValid = true;
}

/**
 * Close the opened file.
 */
void DreamcastSave::close(void)
{
	RP_D(DreamcastSave);

	// Close the VMI file if it's open.
	d->vmi_file.reset();

	// Call the superclass function.
	super::close();
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int DreamcastSave::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	if (!info || !info->ext) {
		// Either no detection information was specified,
		// or the file extension is missing.
		return static_cast<int>(DreamcastSavePrivate::SaveType::Unknown);
	}

	if (info->szFile == 108) {
		// File size is correct for VMI files.
		// Check the file extension.
		if (!strcasecmp(info->ext, ".vmi")) {
			// It's a match!
			return static_cast<int>(DreamcastSavePrivate::SaveType::VMI);
		}
	}

	if ((info->szFile % DC_VMS_BLOCK_SIZE == 0) ||
	    (info->szFile == DC_VMS_ICONDATA_MONO_MINSIZE))
	{
		// File size is correct for VMS files.
		// Check the file extension.
		if (!strcasecmp(info->ext, ".vms")) {
			// It's a match!
			return static_cast<int>(DreamcastSavePrivate::SaveType::VMS);
		}
	}

	// DCI files have the 32-byte directory entry,
	// followed by 32-bit byteswapped data.
	if (((info->szFile - 32) % 512 == 0) ||
	    ((info->szFile - 32) == DC_VMS_ICONDATA_MONO_MINSIZE))
	{
		// File size is correct for DCI files.
		// Check the first byte. (Should be 0x00, 0x33, or 0xCC.)
		if (info->header.addr == 0 && info->header.size >= 32) {
			const uint8_t *const pData = info->header.pData;
			if (*pData == 0x00 || *pData == 0x33 || *pData == 0xCC) {
				// First byte is correct.
				// Check the file extension.
				if (!strcasecmp(info->ext, ".dci")) {
					// It's a match!
					return static_cast<int>(DreamcastSavePrivate::SaveType::DCI);
				}
			}
		}
	}

	// Not supported.
	return static_cast<int>(DreamcastSavePrivate::SaveType::Unknown);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *DreamcastSave::systemName(unsigned int type) const
{
	RP_D(const DreamcastSave);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Dreamcast has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"DreamcastSave::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Sega Dreamcast", "Dreamcast", "DC", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t DreamcastSave::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON | IMGBF_INT_BANNER;
}

/**
 * Get a bitfield of image types this object can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t DreamcastSave::supportedImageTypes(void) const
{
	RP_D(const DreamcastSave);
	uint32_t ret = 0;

	if (d->vms_header.icon_count > 0 || (d->loaded_headers & DreamcastSavePrivate::DC_IS_ICONDATA_VMS)) {
		ret = IMGBF_INT_ICON;
	}
	if (d->vms_header.eyecatch_type >  DC_VMS_EYECATCH_NONE &&
	    d->vms_header.eyecatch_type <= DC_VMS_EYECATCH_CI4)
	{
		ret |= IMGBF_INT_BANNER;
	}

	return ret;
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
vector<RomData::ImageSizeDef> DreamcastSave::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_INT_ICON:
			return {{nullptr, DC_VMS_ICON_W, DC_VMS_ICON_H, 0}};
		case IMG_INT_BANNER:
			return {{nullptr, DC_VMS_EYECATCH_W, DC_VMS_EYECATCH_H, 0}};
		default:
			break;
	}

	// Unsupported image type.
	return {};
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
vector<RomData::ImageSizeDef> DreamcastSave::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);
	RP_D(const DreamcastSave);

	switch (imageType) {
		case IMG_INT_ICON: {
			if (d->vms_header.icon_count > 0 || (d->loaded_headers & DreamcastSavePrivate::DC_IS_ICONDATA_VMS)) {
				// Icon is present.
				return {{nullptr, DC_VMS_ICON_W, DC_VMS_ICON_H, 0}};
			}
			break;
		}
		case IMG_INT_BANNER: {
			if (d->vms_header.eyecatch_type >  DC_VMS_EYECATCH_NONE &&
			    d->vms_header.eyecatch_type <= DC_VMS_EYECATCH_CI4)
			{
				// Eyecatch (banner) is present.
				return {{nullptr, DC_VMS_EYECATCH_W, DC_VMS_EYECATCH_H, 0}};
			}
			break;
		}
		default:
			break;
	}

	// Unsupported image type.
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
uint32_t DreamcastSave::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_INT_ICON: {
			RP_D(const DreamcastSave);
			// Use nearest-neighbor scaling when resizing.
			// Also, need to check if this is an animated icon.
			const_cast<DreamcastSavePrivate*>(d)->loadIcon();
			if (d->iconAnimData && d->iconAnimData->count > 1) {
				// Animated icon.
				ret = IMGPF_RESCALE_NEAREST | IMGPF_ICON_ANIMATED;
			} else {
				// Not animated.
				ret = IMGPF_RESCALE_NEAREST;
			}
			break;
		}

		case IMG_INT_BANNER:
			// Use nearest-neighbor scaling.
			return IMGPF_RESCALE_NEAREST;

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
int DreamcastSave::loadFieldData(void)
{
	RP_D(DreamcastSave);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->saveType < 0) {
		// Unknown save file type.
		return -EIO;
	}

	// TODO: The "Warning" field is not shown if all fields are shown.
	d->fields.reserve(11);	// Maximum of 11 fields.

	// NOTE: DCI files have a directory entry, but not the
	// extra VMI information.
	switch (d->loaded_headers) {
		case DreamcastSavePrivate::DC_HAVE_VMS |
		     DreamcastSavePrivate::DC_HAVE_VMI:
		case DreamcastSavePrivate::DC_HAVE_VMS |
		     DreamcastSavePrivate::DC_HAVE_DIR_ENTRY:
		case DreamcastSavePrivate::DC_HAVE_VMS |
		     DreamcastSavePrivate::DC_HAVE_VMI |
		     DreamcastSavePrivate::DC_HAVE_DIR_ENTRY:
		case DreamcastSavePrivate::DC_IS_ICONDATA_VMS |
		     DreamcastSavePrivate::DC_HAVE_VMI:
		case DreamcastSavePrivate::DC_IS_ICONDATA_VMS |
		     DreamcastSavePrivate::DC_HAVE_DIR_ENTRY:
		case DreamcastSavePrivate::DC_IS_ICONDATA_VMS |
		     DreamcastSavePrivate::DC_HAVE_VMI |
		     DreamcastSavePrivate::DC_HAVE_DIR_ENTRY:
			// VMS and the directory entry are present.
			// Don't show the "warning" field.
			break;

		case DreamcastSavePrivate::DC_HAVE_VMI:
		case DreamcastSavePrivate::DC_HAVE_DIR_ENTRY:
		case DreamcastSavePrivate::DC_HAVE_VMI |
		     DreamcastSavePrivate::DC_HAVE_DIR_ENTRY:
			d->fields.addField_string(C_("RomData", "Warning"),
				// tr: VMS file is missing.
				C_("DreamcastSave", "The VMS file was not found."),
				RomFields::STRF_WARNING);
			break;

		case DreamcastSavePrivate::DC_HAVE_VMS:
		case DreamcastSavePrivate::DC_IS_ICONDATA_VMS:
			d->fields.addField_string(C_("RomData", "Warning"),
				// tr: VMI file is missing.
				C_("DreamcastSave", "The VMI file was not found."),
				RomFields::STRF_WARNING);
			break;

		default:
			assert(!"DreamcastSave: Unrecognized VMS/VMI combination.");
			d->fields.addField_string(C_("RomData", "Warning"),
				// tr: Should not happen...
				C_("DreamcastSave", "Unrecognized VMS/VMI combination."),
				RomFields::STRF_WARNING);
			break;
	}

	// DC VMI header.
	if (d->loaded_headers & DreamcastSavePrivate::DC_HAVE_VMI) {
		d->fields.addField_string(C_("DreamcastSave", "VMI Description"),
			cp1252_sjis_to_utf8(
				d->vmi_header.description, sizeof(d->vmi_header.description)),
				RomFields::STRF_TRIM_END);
		d->fields.addField_string(C_("DreamcastSave", "VMI Copyright"),
			cp1252_sjis_to_utf8(
				d->vmi_header.copyright, sizeof(d->vmi_header.copyright)),
				RomFields::STRF_TRIM_END);
	}

	// File type.
	const char *filetype;
	if (d->loaded_headers & DreamcastSavePrivate::DC_IS_ICONDATA_VMS) {
		// tr: ICONDATA_VMS
		filetype = C_("DreamcastSave", "Icon Data");
	} else if (d->loaded_headers & DreamcastSavePrivate::DC_HAVE_DIR_ENTRY) {
		// Use the type from the directory entry.
		switch (d->vms_dirent.filetype) {
			case DC_VMS_DIRENT_FTYPE_NONE:
				// tr: No file type entry.
				filetype = C_("DreamcastSave", "None");
				break;
			case DC_VMS_DIRENT_FTYPE_DATA:
				// tr: Save file.
				filetype = C_("DreamcastSave", "Save Data");
				break;
			case DC_VMS_DIRENT_FTYPE_GAME:
				// tr: VMU game file. ("VMS" in Japan; "VMU" in USA; "VM" in Europe)
				filetype = C_("DreamcastSave", "VMU Game");
				break;
			default:
				filetype = nullptr;
				break;
		}
	} else {
		// Determine the type based on the VMS header offset.
		switch (d->vms_header_offset) {
			case 0:
				// tr: Save file.
				filetype = C_("DreamcastSave", "Save Data");
				break;
			case DC_VMS_BLOCK_SIZE:
				// tr: VMU game file. ("VMS" in Japan; "VMU" in USA; "VM" in Europe)
				filetype = C_("DreamcastSave", "VMU Game");
				break;
			default:
				filetype = nullptr;
				break;
		}
	}

	const char *const filetype_title = C_("DreamcastSave", "File Type");
	if (filetype) {
		d->fields.addField_string(filetype_title, filetype);
	} else {
		// Unknown file type.
		d->fields.addField_string(filetype_title,
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"),
				d->vms_dirent.filetype));
	}

	// DC VMS directory entry
	if (d->loaded_headers & DreamcastSavePrivate::DC_HAVE_DIR_ENTRY) {
		// Copy protection
		const char *protect;
		switch (d->vms_dirent.protect) {
			case DC_VMS_DIRENT_PROTECT_COPY_OK:
			default:
				// TODO: Show the value if it isn't 0x00?
				protect = C_("DreamcastSave", "Copy OK");
				break;
			case DC_VMS_DIRENT_PROTECT_COPY_PROTECTED:
				protect = C_("DreamcastSave", "Copy Protected");
				break;
		}

		const char *const protect_title = C_("DreamcastSave", "Copy Protect");
		if (protect) {
			d->fields.addField_string(protect_title, protect);
		} else {
			// Unknown copy protection.
			d->fields.addField_string(protect_title,
				rp_sprintf(C_("RomData", "Unknown (0x%02X)"), d->vms_dirent.protect));
		}

		// Filename
		// TODO: Latin1 or Shift-JIS?
		d->fields.addField_string(C_("DreamcastSave", "Filename"),
			latin1_to_utf8(d->vms_dirent.filename, sizeof(d->vms_dirent.filename)));

		// Creation time
		d->fields.addField_dateTime(C_("DreamcastSave", "Creation Time"), d->ctime,
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_HAS_TIME |
			RomFields::RFT_DATETIME_IS_UTC  // Dreamcast doesn't support timezones.
		);
	}

	const char *const vms_description_title = C_("DreamcastSave", "VMS Description");
	if (d->loaded_headers & DreamcastSavePrivate::DC_IS_ICONDATA_VMS) {
		// DC ICONDATA_VMS header.
		const DC_VMS_ICONDATA_Header *const icondata_vms = &d->vms_header.icondata_vms;

		// VMS description
		d->fields.addField_string(vms_description_title,
			cp1252_sjis_to_utf8(
				icondata_vms->vms_description, sizeof(icondata_vms->vms_description)),
				RomFields::STRF_TRIM_END);

		// Other VMS fields aren't used here.
		// TODO: Indicate if both a mono and color icon are present?
	} else if (d->loaded_headers & DreamcastSavePrivate::DC_HAVE_VMS) {
		// DC VMS header.
		const DC_VMS_Header *const vms_header = &d->vms_header;

		// VMS description
		d->fields.addField_string(vms_description_title,
			cp1252_sjis_to_utf8(
				vms_header->vms_description, sizeof(vms_header->vms_description)),
				RomFields::STRF_TRIM_END);

		// DC description
		d->fields.addField_string(C_("DreamcastSave", "DC Description"),
			cp1252_sjis_to_utf8(
				vms_header->dc_description, sizeof(vms_header->dc_description)),
				RomFields::STRF_TRIM_END);

		// Game title
		// NOTE: This is used as the "sort key" on DC file management,
		// and occasionally has control codes.
		// TODO: Escape the control codes.
		d->fields.addField_string(C_("DreamcastSave", "Game Title"),
			cp1252_sjis_to_utf8(
				vms_header->application, sizeof(vms_header->application)));

		// CRC
		// NOTE: Seems to be 0 for all of the SA2 theme files.
		// NOTE: "CRC" is non-translatable.
		d->fields.addField_string_numeric("CRC",
			vms_header->crc, RomFields::Base::Hex, 4,
			RomFields::STRF_MONOSPACE);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int DreamcastSave::loadMetaData(void)
{
	RP_D(DreamcastSave);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->saveType < 0) {
		// Unknown save type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(2);	// Maximum of 4 metadata properties.

	// TODO: More metadata properties?

	// Title
	if (d->loaded_headers & DreamcastSavePrivate::DC_HAVE_VMS) {
		d->metaData->addMetaData_string(Property::Title,
				cp1252_sjis_to_utf8(
					d->vms_header.dc_description, sizeof(d->vms_header.dc_description)),
					RomFields::STRF_TRIM_END);
	} else if (d->loaded_headers & DreamcastSavePrivate::DC_HAVE_VMI) {
		d->metaData->addMetaData_string(Property::Title,
			cp1252_sjis_to_utf8(
				d->vmi_header.description, sizeof(d->vmi_header.description)),
				RomFields::STRF_TRIM_END);
	}

	// Creation time
	if (d->loaded_headers & DreamcastSavePrivate::DC_HAVE_DIR_ENTRY) {
		// TODO: Dreamcast doesn't support timezones.
		d->metaData->addMetaData_timestamp(Property::CreationDate, d->ctime);
	}

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
int DreamcastSave::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(DreamcastSave);
	switch (imageType) {
		case IMG_INT_ICON:
			if (d->iconAnimData) {
				// Return the first icon frame.
				// NOTE: DC save icon animations are always
				// sequential, so we can use a shortcut here.
				pImage = d->iconAnimData->frames[0];
				return 0;
			}
			break;
		case IMG_INT_BANNER:
			if (d->img_banner) {
				// Banner is loaded.
				pImage = d->img_banner;
				return 0;
			}
			break;
		default:
			// Unsupported image type.
			pImage.reset();
			return -ENOENT;
	}

	if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Save file isn't valid.
		return -EIO;
	}

	// Load the image.
	switch (imageType) {
		case IMG_INT_ICON:
			pImage = d->loadIcon();
			break;
		case IMG_INT_BANNER:
			pImage = d->loadBanner();
			break;
		default:
			// Unsupported.
			pImage.reset();
			return -ENOENT;
	}

	// TODO: -ENOENT if the file doesn't actually have an icon/banner.
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
IconAnimDataConstPtr DreamcastSave::iconAnimData(void) const
{
	RP_D(const DreamcastSave);
	if (!d->iconAnimData) {
		// Load the icon.
		if (!const_cast<DreamcastSavePrivate*>(d)->loadIcon()) {
			// Error loading the icon.
			return nullptr;
		}
		if (!d->iconAnimData) {
			// Still no icon...
			return nullptr;
		}
	}

	if (d->iconAnimData->count <= 1 ||
	    d->iconAnimData->seq_count <= 1)
	{
		// Not an animated icon.
		return nullptr;
	}

	// Return the icon animation data.
	return d->iconAnimData;
}

}
