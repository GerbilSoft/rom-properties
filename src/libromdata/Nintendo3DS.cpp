/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DS.hpp: Nintendo 3DS ROM reader.                               *
 * Handles CCI/3DS, CIA, and SMDH files.                                   *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "Nintendo3DS.hpp"
#include "RomData_p.hpp"

#include "n3ds_structs.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

#include "img/rp_image.hpp"
#include "img/ImageDecoder.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

class Nintendo3DSPrivate : public RomDataPrivate
{
	public:
		Nintendo3DSPrivate(Nintendo3DS *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(Nintendo3DSPrivate)

	public:
		// RomFields data.
		static const struct RomFields::Desc n3ds_fields[];

		// ROM type.
		enum RomType {
			ROM_TYPE_UNKNOWN = -1,	// Unknown ROM type.

			ROM_TYPE_SMDH    = 0,	// SMDH
			ROM_TYPE_CCI     = 1,	// CCI/3DS (cartridge dump)
			ROM_TYPE_eMMC    = 2,	// eMMC dump
			ROM_TYPE_CIA     = 3,	// CIA
		};
		int romType;

		// SMDH header.
		// NOTE: Must be byteswapped on access.
		N3DS_SMDH_Header_t smdh_header;
		// TODO: Bitflags.
		bool has_smdh_header;

		/**
		 * Load the ROM image's icon.
		 * @return Icon, or nullptr on error.
		 */
		rp_image *loadIcon(void);
};

/** Nintendo3DSPrivate **/

// ROM fields.
const struct RomFields::Desc Nintendo3DSPrivate::n3ds_fields[] = {
	// SMDH
	{_RP("Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Full Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Publisher"), RomFields::RFT_STRING, {nullptr}},
	// TODO (and others)
	//{_RP("Age Ratings"), RomFields::RFT_STRING, {nullptr}},
	//{_RP("Region"}, RomFields::RFT_BITFIELD, {nullptr}},
};

Nintendo3DSPrivate::Nintendo3DSPrivate(Nintendo3DS *q, IRpFile *file)
	: super(q, file, n3ds_fields, ARRAY_SIZE(n3ds_fields))
	, romType(ROM_TYPE_UNKNOWN)
	, has_smdh_header(false)
{
	// Clear the SMDH header.
	memset(&smdh_header, 0, sizeof(smdh_header));
}

/**
 * Load the ROM image's icon.
 * @return Icon, or nullptr on error.
 */
rp_image *Nintendo3DSPrivate::loadIcon(void)
{
	if (!file || !isValid) {
		// Can't load the icon.
		return nullptr;
	}

	// TODO: Small icon?

	// Load the SMDH icon data structure first.
	// How to load this depends on the file type.
	N3DS_SMDH_Icon_t smdh_icon;
	switch (romType) {
		case ROM_TYPE_SMDH: {
			// SMDH file. The icon is located
			// immediately after the SMDH header.
			int ret = file->seek(sizeof(smdh_header));
			if (ret != 0) {
				// Seek failed.
				return nullptr;
			}
			size_t size = file->read(&smdh_icon, sizeof(smdh_icon));
			if (size != sizeof(smdh_icon)) {
				// Read failed.
				return nullptr;
			}

			// SMDH icon loaded.
			break;
		}

		default:
			// Unsupported...
			return nullptr;
	}

	// Convert the large icon to rp_image.
	// NOTE: Assuming RGB565 format.
	// 3dbrew.org says it could be any of various formats,
	// but only RGB565 has been used so far.
	// Reference: https://www.3dbrew.org/wiki/SMDH#Icon_graphics
	return ImageDecoder::fromN3DSTiledRGB565(
		N3DS_SMDH_ICON_LARGE_W, N3DS_SMDH_ICON_LARGE_H,
		smdh_icon.large, sizeof(smdh_icon.large));
}

/** Nintendo3DS **/

/**
 * Read a Nintendo 3DS ROM image.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open disc image.
 */
Nintendo3DS::Nintendo3DS(IRpFile *file)
	: super(new Nintendo3DSPrivate(this, file))
{
	// This class handles several different types of files,
	// so we'll initialize d->fileType later.
	RP_D(Nintendo3DS);
	d->fileType = FTYPE_UNKNOWN;
	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the ROM header.
	uint8_t header[512];
	d->file->rewind();
	size_t size = d->file->read(&header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = reinterpret_cast<const uint8_t*>(header);
	info.ext = nullptr;	// Not needed for 3DS files.
	info.szFile = d->file->size();
	d->romType = isRomSupported_static(&info);

	// Determine what kind of file this is,
	// and load the SMDH header and icon.
	switch (d->romType) {
		case Nintendo3DSPrivate::ROM_TYPE_SMDH:
			// SMDH header.
			if (info.szFile < (int64_t)(sizeof(N3DS_SMDH_Header_t) + sizeof(N3DS_SMDH_Icon_t))) {
				// File is too small.
				return;
			}

			d->file->rewind();
			size = d->file->read(&d->smdh_header, sizeof(d->smdh_header));
			if (size != sizeof(d->smdh_header)) {
				// Error reading the SMDH header.
				d->romType = Nintendo3DSPrivate::ROM_TYPE_UNKNOWN;
				return;
			}

			// SMDH icon data will be read in loadIcon().
			d->fileType = FTYPE_ICON_FILE;
			d->has_smdh_header = true;
			break;

		default:
			// Unknown ROM format.
			d->romType = Nintendo3DSPrivate::ROM_TYPE_UNKNOWN;
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
int Nintendo3DS::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 512)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for SMDH.
	if (!memcmp(info->header.pData, N3DS_SMDH_HEADER_MAGIC, 4) &&
	    info->szFile >= (int64_t)(sizeof(N3DS_SMDH_Header_t) + sizeof(N3DS_SMDH_Icon_t)))
	{
		// We have an SMDH file.
		return Nintendo3DSPrivate::ROM_TYPE_SMDH;
	}

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Nintendo3DS::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *Nintendo3DS::systemName(uint32_t type) const
{
	RP_D(const Nintendo3DS);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	// TODO: *New* Nintendo 3DS for N3DS-exclusive titles.
	static const rp_char *const sysNames[4] = {
		_RP("Nintendo 3DS"), _RP("Nintendo 3DS"), _RP("3DS"), nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> Nintendo3DS::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		_RP(".smdh"),	// SMDH (icon) file.
		_RP(".3dsx"),	// Homebrew application.
		//_RP(".3ds"),	// ROM image. (TODO: Conflicts with 3DS Max)
		_RP(".cci"),	// ROM image.
		_RP(".cia"),	// CTR installable archive.
	};
	return vector<const rp_char*>(exts, exts + ARRAY_SIZE(exts));
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> Nintendo3DS::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Nintendo3DS::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Nintendo3DS::supportedImageTypes(void) const
{
	return supportedImageTypes_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Nintendo3DS::loadFieldData(void)
{
	RP_D(Nintendo3DS);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->romType < 0) {
		// Unknown ROM type.
		return -EIO;
	}

	if (d->has_smdh_header) {
		// SMDH header.
		// TODO: Get the system language.
		d->fields->addData_string(utf16le_to_rp_string(
			d->smdh_header.titles[1].desc_short, sizeof(d->smdh_header.titles[1].desc_short)));
		d->fields->addData_string(utf16le_to_rp_string(
			d->smdh_header.titles[1].desc_long, sizeof(d->smdh_header.titles[1].desc_long)));
		d->fields->addData_string(utf16le_to_rp_string(
			d->smdh_header.titles[1].publisher, sizeof(d->smdh_header.titles[1].publisher)));
	} else {
		// SMDH isn't loaded.
		d->fields->addData_invalid();
		d->fields->addData_invalid();
		d->fields->addData_invalid();
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

/**
 * Load an internal image.
 * Called by RomData::image() if the image data hasn't been loaded yet.
 * @param imageType Image type to load.
 * @return 0 on success; negative POSIX error code on error.
 */
int Nintendo3DS::loadInternalImage(ImageType imageType)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}

	RP_D(Nintendo3DS);
	if (d->images[imageType]) {
		// Icon *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Save file isn't valid.
		return -EIO;
	}

	// Check for supported image types.
	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported by 3DS.
		return -ENOENT;
	}

	// Use nearest-neighbor scaling when resizing.
	d->imgpf[imageType] = IMGPF_RESCALE_NEAREST;
	d->images[imageType] = d->loadIcon();

	// TODO: -ENOENT if the file doesn't actually have an icon.
	return (d->images[imageType] != nullptr ? 0 : -EIO);
}

}
