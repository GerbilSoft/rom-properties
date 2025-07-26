/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUAncast.cpp: Nintendo Wii U "Ancast" image reader.                   *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WiiUAncast.hpp"
#include "wiiu_ancast_structs.h"

// "Happy Wii U" PNG images
#include "../res/img/happy-wii-u.h"

// Other rom-properties libraries
#include "librpbase/img/RpPng.hpp"
#include "librpfile/MemFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpTexture;

// C++ STL classes
using std::array;
using std::vector;

namespace LibRomData {

class WiiUAncastPrivate final : public RomDataPrivate
{
public:
	explicit WiiUAncastPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(WiiUAncastPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 3+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	enum class AncastType {
		Unknown	= -1,

		ARM	= 0,	// ARM "Ancast" image
		PowerPC	= 1,	// PowerPC "Ancast" image

		Max
	};
	AncastType ancastType;

	// Icon
	rp_image_ptr img_icon;

	/**
	 * Load the icon.
	 * @return Icon, or nullptr on error.
	 */
	rp_image_const_ptr loadIcon(void);

	// "Ancast" image header
	// NOTE: Must be byteswapped on access.
	union {
		WiiU_Ancast_Header_SigCommon_t sigCommon;
		WiiU_Ancast_Header_ARM_t arm;
		WiiU_Ancast_Header_PPC_t ppc;
	} ancastHeader;
};

ROMDATA_IMPL(WiiUAncast)
ROMDATA_IMPL_IMG(WiiUAncast)

/** WiiUAncastPrivate **/

/* RomDataInfo */
const array<const char*, 3+1> WiiUAncastPrivate::exts = {{
	".img",		// Wii U fw.img, kernel.img
	".app",		// vWii titles
	".ancast",	// custom

	nullptr
}};
const array<const char*, 1+1> WiiUAncastPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	// TODO: Change from "Ancast" to "Firmware"?
	"application/x-wii-u-ancast",

	nullptr
}};
const RomDataInfo WiiUAncastPrivate::romDataInfo = {
	"WiiUAncast", exts.data(), mimeTypes.data()
};

WiiUAncastPrivate::WiiUAncastPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, ancastType(AncastType::Unknown)
{
	// Clear the various structs.
	memset(&ancastHeader, 0, sizeof(ancastHeader));
}

/**
 * Load the icon.
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr WiiUAncastPrivate::loadIcon(void)
{
	// TODO: Point to a preinstalled icon instead of generating a thumbnail.
	// On Windows, point to an icon resource in the DLL?
	// On Linux, point to a preinstalled icon in /usr/share/icons/.

	if (img_icon) {
		// Icon has already been loaded.
		return img_icon;
	} else if (!this->isValid || static_cast<int>(this->ancastType) < 0) {
		// Can't load the icon.
		return nullptr;
	}

	// Determine the target device and console type,
	// then load the matching PNG image.
	unsigned int target_device = 0;
	unsigned int console_type = 0;

	switch (be32_to_cpu(ancastHeader.sigCommon.sig_type)) {
		default:
			break;

		case WIIU_ANCAST_SIGTYPE_ECDSA: {
			const WiiU_Ancast_Header_PPC_t *const ppc = &ancastHeader.ppc;
			target_device = be32_to_cpu(ppc->target_device);
			console_type = be32_to_cpu(ppc->console_type);
			break;
		}

		case WIIU_ANCAST_SIGTYPE_RSA2048: {
			const WiiU_Ancast_Header_ARM_t *const arm = &ancastHeader.arm;
			target_device = be32_to_cpu(arm->target_device);
			console_type = be32_to_cpu(arm->console_type);
			break;
		}
	}

	// Select the PNG image based on console_type and target_device.
	// TODO: Wii U with "?" for unknown target device or console type.
	// TODO: Indicate missing signature?
	const uint8_t *png_data = nullptr;
	size_t png_size = 0;
	if (likely(console_type != WIIU_ANCAST_CONSOLE_TYPE_DEVEL)) {
		// Retail (prod)
		if (likely(target_device != WIIU_ANCAST_TARGET_DEVICE_ARM_SD)) {
			png_data = happy_wii_u_prod_png;
			png_size = sizeof(happy_wii_u_prod_png);
		} else {
			png_data = happy_wii_u_prod_sdboot_png;
			png_size = sizeof(happy_wii_u_prod_sdboot_png);
		}
	} else {
		// Debug (devel)
		if (likely(target_device != WIIU_ANCAST_TARGET_DEVICE_ARM_SD)) {
			png_data = happy_wii_u_devel_png;
			png_size = sizeof(happy_wii_u_devel_png);
		} else {
			png_data = happy_wii_u_devel_sdboot_png;
			png_size = sizeof(happy_wii_u_devel_sdboot_png);
		}
	}

	// Create a MemFile and decode the image.
	MemFile f_mem(png_data, png_size);
	this->img_icon = RpPng::load(&f_mem);
	return this->img_icon;
}

/** WiiUAncast **/

/**
 * Read a Wii U "Ancast" image.
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
WiiUAncast::WiiUAncast(const IRpFilePtr &file)
	: super(new WiiUAncastPrivate(file))
{
	RP_D(WiiUAncast);
	d->mimeType = "application/x-wii-u-ancast";	// unofficial, not on fd.o
	d->fileType = FileType::FirmwareBinary;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the "Ancast" header.
	d->file->rewind();
	size_t size = d->file->read(&d->ancastHeader, sizeof(d->ancastHeader));
	if (size != sizeof(d->ancastHeader)) {
		d->file.reset();
		return;
	}

	// Check if this firmware binary is supported.
	const DetectInfo info = {
		{0, sizeof(d->ancastHeader), reinterpret_cast<const uint8_t*>(&d->ancastHeader)},
		nullptr,	// ext (not needed for WiiUAncast)
		0		// szFile (not needed for WiiUAncast)
	};
	d->ancastType = static_cast<WiiUAncastPrivate::AncastType>(isRomSupported_static(&info));
	if (static_cast<int>(d->ancastType) < 0 || d->ancastHeader.sigCommon.magic != cpu_to_be32(WIIU_ANCAST_HEADER_MAGIC)) {
		// The "Ancast" image may be embedded in some other container.
		uint32_t start_addr = 0;

		if (unlikely(d->ancastHeader.sigCommon.magic == cpu_to_be32(WIIU_TOUCAN_HEADER_MAGIC))) {
			// Toucan image. "Ancast" image is usually stored in the third section.
			const uint32_t *const pData32 = reinterpret_cast<const uint32_t*>(&d->ancastHeader);

			// Get the section count.
			const uint32_t section_count = be32_to_cpu(pData32[0x0004/4]);
			if (section_count < 3) {
				// Not enough sections...
				d->file.reset();
				return;
			}

			// Section headers start at 0x20, and consist of a load address and a length.
			// Add the section 0 and section 1 lengths to get the section 2 start address.
			// NOTE: Requires 32-byte alignment.
			start_addr = 0x20 + (8 * section_count);
			start_addr += be32_to_cpu(pData32[0x0024/4]);
			start_addr += be32_to_cpu(pData32[0x0028/4]);
			start_addr = ALIGN_BYTES(0x20, start_addr);
		} else {
			// vWii "Ancast" images may be embedded in a DOL executable.
			// The "Ancast" image is located in Data0, while a load stub
			// is located in Text0.
			// Data0 start offset is the be32 at 0x1C.
			const uint32_t *pData32 = reinterpret_cast<const uint32_t*>(&d->ancastHeader);
			start_addr = be32_to_cpu(pData32[0x1C/4]);	// Data0 offset
		}

		// Check at the new starting address.
		size = d->file->seekAndRead(start_addr, &d->ancastHeader, sizeof(d->ancastHeader));
		if (size != sizeof(d->ancastHeader)) {
			d->file.reset();
			return;
		}
		d->ancastType = static_cast<WiiUAncastPrivate::AncastType>(isRomSupported_static(&info));
	}

	d->isValid = (static_cast<int>(d->ancastType) >= 0);

	if (!d->isValid) {
		d->file.reset();
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiUAncast::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(WiiU_Ancast_Header_SigCommon_t))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check the "Ancast" magic.
	const WiiU_Ancast_Header_SigCommon_t *sigCommon =
		reinterpret_cast<const WiiU_Ancast_Header_SigCommon_t*>(info->header.pData);
	if (unlikely(sigCommon->magic == cpu_to_be32(WIIU_TOUCAN_HEADER_MAGIC))) {
		// Toucan image. "Ancast" image is usually stored in the third section.
		const uint32_t *const pData32 = reinterpret_cast<const uint32_t*>(info->header.pData);

		// Get the section count.
		const uint32_t section_count = be32_to_cpu(pData32[0x0004/4]);
		if (section_count < 3) {
			// Not enough sections...
			return static_cast<int>(WiiUAncastPrivate::AncastType::Unknown);
		}

		// Section headers start at 0x20, and consist of a load address and a length.
		// Add the section 0 and section 1 lengths to get the section 2 start address.
		// NOTE: Requires 32-byte alignment.
		uint32_t start_addr = 0x20 + (8 * section_count);
		start_addr += be32_to_cpu(pData32[0x0024/4]);
		start_addr += be32_to_cpu(pData32[0x0028/4]);
		start_addr = ALIGN_BYTES(0x20, start_addr);

		// Check at the new starting address.
		sigCommon = reinterpret_cast<const WiiU_Ancast_Header_SigCommon_t*>(&info->header.pData[start_addr]);
		if (sigCommon->magic != cpu_to_be32(WIIU_ANCAST_HEADER_MAGIC)) {
			// Still no magic.
			printf("toucan NOPE\n");
			return static_cast<int>(WiiUAncastPrivate::AncastType::Unknown);
		}
	} else if (sigCommon->magic != cpu_to_be32(WIIU_ANCAST_HEADER_MAGIC)) {
		// vWii "Ancast" images may be embedded in a DOL executable.
		// The "Ancast" image is located in Data0, while a load stub
		// is located in Text0.
		// Data0 start offset is the be32 at 0x1C.
		const uint32_t *const pData32 = reinterpret_cast<const uint32_t*>(info->header.pData);
		uint32_t data0_offset = be32_to_cpu(pData32[0x1C/4]);
		if (data0_offset + sizeof(WiiU_Ancast_Header_SigCommon_t) <= info->header.size) {
			// Check at data0_offset.
			sigCommon = reinterpret_cast<const WiiU_Ancast_Header_SigCommon_t*>(&info->header.pData[data0_offset]);
			if (sigCommon->magic != cpu_to_be32(WIIU_ANCAST_HEADER_MAGIC)) {
				// Still no magic.
				return static_cast<int>(WiiUAncastPrivate::AncastType::Unknown);
			}
		}	
	}

	// Magic number is correct.
	// Verify the NULLs and signature type.
	if (sigCommon->null_0 == 0 && sigCommon->null_1 == 0 &&
	    sigCommon->null_2[0] == 0 && sigCommon->null_2[1] == 0 &&
	    sigCommon->null_2[2] == 0 && sigCommon->null_2[3] == 0)
	{
		switch (be32_to_cpu(sigCommon->sig_type)) {
			default:
				// Invalid signature type.
				break;
			case WIIU_ANCAST_SIGTYPE_ECDSA:
				// ECDSA: Used by PowerPC "Ancast" images.
				return static_cast<int>(WiiUAncastPrivate::AncastType::PowerPC);
			case WIIU_ANCAST_SIGTYPE_RSA2048:
				// RSA-2048: Used by ARM "Ancast" images.
				return static_cast<int>(WiiUAncastPrivate::AncastType::ARM);
		}
	}

	// Not supported.
	return static_cast<int>(WiiUAncastPrivate::AncastType::Unknown);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *WiiUAncast::systemName(unsigned int type) const
{
	RP_D(const WiiUAncast);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// WiiUAncast has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"WiiUAncast::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"Nintendo Wii U", "Wii U", "Wii U", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t WiiUAncast::supportedImageTypes_static(void)
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
vector<RomData::ImageSizeDef> WiiUAncast::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_ICON) {
		// Only icons are supported.
		return {};
	}

	// Our custom Wii U icons are all 48x48.
	return {{nullptr, 48, 48, 0}};
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
uint32_t WiiUAncast::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_INT_ICON: {
			// Use nearest-neighbor scaling when resizing.
			ret = IMGPF_RESCALE_NEAREST;
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
int WiiUAncast::loadFieldData(void)
{
	RP_D(WiiUAncast);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Firmware binary isn't valid.
		return -EIO;
	}

	// Wii U "Ancast" image header (signature common fields)
	// Image header is read in the constructor.
	const WiiU_Ancast_Header_SigCommon_t *const sigCommon = &d->ancastHeader.sigCommon;
	d->fields.reserve(5); // Maximum of 5 fields.

	// CPU
	const char *s_cpu = nullptr;
	switch (d->ancastType) {
		default:
			s_cpu = C_("RomData", "Unknown");
			break;
		case WiiUAncastPrivate::AncastType::ARM:
			s_cpu = "ARM";
			break;
		case WiiUAncastPrivate::AncastType::PowerPC:
			s_cpu = "PowerPC";
			break;
	}
	d->fields.addField_string(C_("WiiUAncast", "CPU"), s_cpu);

	// Get the important fields, depending on signature type.
	const char *s_sigType = nullptr;
	unsigned int target_device = 0;
	unsigned int console_type = 0;
	bool has_signature = false;
	switch (be32_to_cpu(sigCommon->sig_type)) {
		default:
			break;

		case WIIU_ANCAST_SIGTYPE_ECDSA: {
			const WiiU_Ancast_Header_PPC_t *const ppc = &d->ancastHeader.ppc;

			s_sigType = "ECDSA";
			target_device = be32_to_cpu(ppc->target_device);
			console_type = be32_to_cpu(ppc->console_type);

			// Check if the first and last u32s of the signature are non-zero.
			has_signature = ((ppc->signature.u32[0] != 0) &&
			                 (ppc->signature.u32[(sizeof(ppc->signature.u32)/sizeof(uint32_t)) - 1] != 0));
			break;
		}

		case WIIU_ANCAST_SIGTYPE_RSA2048: {
			const WiiU_Ancast_Header_ARM_t *const arm = &d->ancastHeader.arm;

			s_sigType = "RSA-2048";
			target_device = be32_to_cpu(arm->target_device);
			console_type = be32_to_cpu(arm->console_type);

			// Check if the first and last u32s of the signature are non-zero.
			has_signature = ((arm->signature.u32[0] != 0) &&
			                 (arm->signature.u32[(sizeof(arm->signature.u32)/sizeof(uint32_t)) - 1] != 0));
			break;
		}
	}

	if (!s_sigType) {
		// Unknown signature type. Can't check anything else.
		d->fields.addField_string(C_("WiiUAncast", "Signature Type"), C_("RomData", "Unknown"));
		return static_cast<int>(d->fields.count());
	}

	// Signature type
	d->fields.addField_string(C_("WiiUAncast", "Signature Type"), s_sigType);

	// Has signature?
	// Unsigned images will only boot on systems with blank OTP/eFuses,
	// unless an exploit such as de_Fuse or Paid The Beak is used.
	d->fields.addField_string(C_("WiiUAncast", "Has Signature?"),
		has_signature ? C_("RomData", "Yes") : C_("RomData", "No"));

	// Target device
	const char *const s_target_device_title = C_("WiiUAncast", "Target Device");
	struct target_device_tbl_t {
		uint8_t id;
		const char *desc;
	};
	static const array<target_device_tbl_t, 6> target_device_tbl = {{
		// PowerPC
		{WIIU_ANCAST_TARGET_DEVICE_PPC_WIIU,	"Wii U"},
		{WIIU_ANCAST_TARGET_DEVICE_PPC_VWII_12,	"vWii (variant 0x12)"},
		{WIIU_ANCAST_TARGET_DEVICE_PPC_VWII,	"vWii"},
		{WIIU_ANCAST_TARGET_DEVICE_PPC_SPECIAL,	"Special (0x14)"},

		// ARM
		{WIIU_ANCAST_TARGET_DEVICE_ARM_NAND,	"NAND"},
		{WIIU_ANCAST_TARGET_DEVICE_ARM_SD,	"SD"},
	}};

	auto iter = std::find_if(target_device_tbl.cbegin(), target_device_tbl.cend(),
		[target_device](target_device_tbl_t entry) noexcept -> bool {
			return (entry.id == target_device);
		});
	if (iter != target_device_tbl.cend()) {
		d->fields.addField_string(s_target_device_title, iter->desc);
	} else {
		d->fields.addField_string(s_target_device_title,
			fmt::format(FRUN(C_("RomData", "Unknown ({:d})")), target_device));
	}

	// Console type
	const char *const s_console_type_title = C_("WiiUAncast", "Console Type");
	const char *s_console_type;
	switch (console_type) {
		default:
			s_console_type = nullptr;
			break;
		case WIIU_ANCAST_CONSOLE_TYPE_DEVEL:
			s_console_type = "Debug";
			break;
		case WIIU_ANCAST_CONSOLE_TYPE_PROD:
			s_console_type = "Retail";
			break;
	}
	if (s_console_type) {
		d->fields.addField_string(s_console_type_title, s_console_type);
	} else {
		d->fields.addField_string(s_console_type_title,
			fmt::format(FRUN(C_("RomData", "Unknown ({:d})")), console_type));
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiUAncast::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(WiiUAncast);
	ROMDATA_loadInternalImage_single(
		IMG_INT_ICON,	// ourImageType
		d->file,	// file
		d->isValid,	// isValid
		d->ancastType,	// romType
		d->img_icon,	// imgCache
		d->loadIcon);	// func
}

} // namespace LibRomData
