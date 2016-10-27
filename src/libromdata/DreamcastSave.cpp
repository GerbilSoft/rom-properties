/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DreamcastSave.cpp: Sega Dreamcast save file reader.                     *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#include "DreamcastSave.hpp"
#include "dc_structs.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

#include "img/rp_image.hpp"
#include "img/ImageDecoder.hpp"
#include "img/IconAnimData.hpp"

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

class DreamcastSavePrivate
{
	public:
		explicit DreamcastSavePrivate(DreamcastSave *q);
		~DreamcastSavePrivate();

	private:
		DreamcastSavePrivate(const DreamcastSavePrivate &other);
		DreamcastSavePrivate &operator=(const DreamcastSavePrivate &other);
	private:
		DreamcastSave *const q;

	public:
		// RomFields data.

		// Monospace string formatting.
		static const RomFields::StringDesc dc_save_string_monospace;

		// RomFields data.
		static const struct RomFields::Desc dc_save_fields[];

	public:
		// VMS header.
		DC_VMS_Header vms_header;

		// Animated icon data.
		// NOTE: The first frame is owned by the RomData superclass.
		IconAnimData *iconAnimData;

		/**
		 * Load the save file's icons.
		 *
		 * This will load all of the animated icon frames,
		 * though only the first frame will be returned.
		 *
		 * @return Icon, or nullptr on error.
		 */
		rp_image *loadIcon(void);

		/**
		 * Load the save file's banner.
		 * @return Banner, or nullptr on error.
		 */
		rp_image *loadBanner(void);
};

/** DreamcastSavePrivate **/

// Monospace string formatting.
const RomFields::StringDesc DreamcastSavePrivate::dc_save_string_monospace = {
	RomFields::StringDesc::STRF_MONOSPACE
};

// Save file fields.
const struct RomFields::Desc DreamcastSavePrivate::dc_save_fields[] = {
	// TODO: VMI fields.
	{_RP("VMS Description"), RomFields::RFT_STRING, {nullptr}},
	{_RP("DC Description"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Application"), RomFields::RFT_STRING, {nullptr}},
	{_RP("CRC"), RomFields::RFT_STRING, {&dc_save_string_monospace}},
};

DreamcastSavePrivate::DreamcastSavePrivate(DreamcastSave *q)
	: q(q)
	, iconAnimData(nullptr)
{
	// Clear the VMS header..
	memset(&vms_header, 0, sizeof(vms_header));
}

DreamcastSavePrivate::~DreamcastSavePrivate()
{
	if (iconAnimData) {
		// Delete all except the first animated icon frame.
		// (The first frame is owned by the RomData superclass.)
		for (int i = iconAnimData->count-1; i >= 1; i--) {
			delete iconAnimData->frames[i];
		}
		delete iconAnimData;
	}
}

/**
 * Load the save file's icons.
 *
 * This will load all of the animated icon frames,
 * though only the first frame will be returned.
 *
 * @return Icon, or nullptr on error.
 */
rp_image *DreamcastSavePrivate::loadIcon(void)
{
	if (!q->m_file || !q->m_isValid) {
		// Can't load the icon.
		return nullptr;
	}

	if (iconAnimData) {
		// Icon has already been loaded.
		return const_cast<rp_image*>(iconAnimData->frames[0]);
	}

	// TODO
	return nullptr;
}

/**
 * Load the save file's banner.
 * @return Banner, or nullptr on error.
 */
rp_image *DreamcastSavePrivate::loadBanner(void)
{
	if (!q->m_file || !q->m_isValid) {
		// Can't load the banner.
		return nullptr;
	}

	// TODO
	return nullptr;
}

/** DreamcastSave **/

/**
 * Read a Nintendo Dreamcast save file.
 *
 * A save file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open disc image.
 */
DreamcastSave::DreamcastSave(IRpFile *file)
	: RomData(file, DreamcastSavePrivate::dc_save_fields, ARRAY_SIZE(DreamcastSavePrivate::dc_save_fields))
	, d(new DreamcastSavePrivate(this))
{
	// This class handles save files.
	m_fileType = FTYPE_SAVE_FILE;

	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// TODO: VMI+VMS. Currently handling VMS only.

	// Read the save file header.
	static_assert(sizeof(DC_VMS_Header) == DC_VMS_Header_SIZE,
		"DC_VMS_Header is the wrong size. (Should be 96 bytes.)");
	m_file->rewind();
	size_t size = m_file->read(&d->vms_header, sizeof(d->vms_header));
	if (size != sizeof(d->vms_header))
		return;

	// TODO: Verify the file extension and header fields?
	m_isValid = true;
}

DreamcastSave::~DreamcastSave()
{
	delete d;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int DreamcastSave::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	if (!info) {
		// Either no detection information was specified,
		// or the file extension is missing.
		return -1;
	}

	// TODO: Handle ".vmi" files.

	// VMS files don't have a magic number.
	// Make sure the extension is ".vms" and the size
	// is a multiple of 512.
	if (info->szFile % 512 != 0) {
		// Incorrect file size.
		return -1;
	}

	if (info->ext && info->ext[0] != 0) {
		// Check the file extension.
		if (!rp_strcasecmp(info->ext, _RP(".vms"))) {
			// It's a match!
			return 0;
		}
	}

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int DreamcastSave::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *DreamcastSave::systemName(uint32_t type) const
{
	if (!m_isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	static const rp_char *const sysNames[4] = {
		_RP("Sega Dreamcast"), _RP("Dreamcast"), _RP("DC"), nullptr
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
vector<const rp_char*> DreamcastSave::supportedFileExtensions_static(void)
{
	vector<const rp_char*> ret;
	ret.reserve(1);
	ret.push_back(_RP(".vms"));
	// TODO: ".vmi", ".dcs"?
	return ret;
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
vector<const rp_char*> DreamcastSave::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
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
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t DreamcastSave::supportedImageTypes(void) const
{
	return supportedImageTypes_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int DreamcastSave::loadFieldData(void)
{
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!m_file || !m_file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid) {
		// Unknown save file type.
		return -EIO;
	}

	// DC VMS header.
	// TODO: VMI header?
	const DC_VMS_Header *const vms_header = &d->vms_header;

	// VMS description.
	m_fields->addData_string(cp1252_sjis_to_rp_string(vms_header->vms_description, sizeof(vms_header->vms_description)));

	// DC description.
	m_fields->addData_string(cp1252_sjis_to_rp_string(vms_header->dc_description, sizeof(vms_header->dc_description)));

	// Application.
	m_fields->addData_string(cp1252_sjis_to_rp_string(vms_header->application, sizeof(vms_header->application)));

	// CRC.
	// NOTE: Seems to be 0 for all of the SA2 theme files.
	m_fields->addData_string_numeric(le16_to_cpu(vms_header->crc), RomFields::FB_HEX, 4);

	// Finished reading the field data.
	return (int)m_fields->count();
}

/**
 * Load an internal image.
 * Called by RomData::image() if the image data hasn't been loaded yet.
 * @param imageType Image type to load.
 * @return 0 on success; negative POSIX error code on error.
 */
int DreamcastSave::loadInternalImage(ImageType imageType)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}
	if (m_images[imageType]) {
		// Icon *has* been loaded...
		return 0;
	} else if (!m_file) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid) {
		// Save file isn't valid.
		return -EIO;
	}

	// Check for supported image types.
	switch (imageType) {
		case IMG_INT_ICON:
			// Icon.
			m_imgpf[imageType] = IMGPF_RESCALE_NEAREST;
			m_images[imageType] = d->loadIcon();
			if (d->iconAnimData && d->iconAnimData->count > 1) {
				// Animated icon.
				m_imgpf[imageType] |= IMGPF_ICON_ANIMATED;
			}
			break;

		case IMG_INT_BANNER:
			// Banner.
			m_imgpf[imageType] = IMGPF_RESCALE_NEAREST;
			m_images[imageType] = d->loadBanner();
			break;

		default:
			// Unsupported.
			return -ENOENT;
	}

	// TODO: -ENOENT if the file doesn't actually have an icon/banner.
	return (m_images[imageType] != nullptr ? 0 : -EIO);
}

/**
 * Get the animated icon data.
 *
 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
 * object has an animated icon.
 *
 * @return Animated icon data, or nullptr if no animated icon is present.
 */
const IconAnimData *DreamcastSave::iconAnimData(void) const
{
	if (!d->iconAnimData) {
		// Load the icon.
		if (!d->loadIcon()) {
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
