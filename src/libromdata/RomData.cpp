/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomData.cpp: ROM data base class.                                       *
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

#include "RomData.hpp"
#include "common.h"
#include "file/IRpFile.hpp"
#include "img/rp_image.hpp"
#include "img/IconAnimData.hpp"

// dup()
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData {

/**
 * ROM data base class.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * In addition, subclasses must pass an array of RomFieldDesc structs.
 *
 * @param file ROM file.
 * @param fields Array of ROM Field descriptions.
 * @param count Number of ROM Field descriptions.
 */
RomData::RomData(IRpFile *file, const RomFields::Desc *fields, int count)
	: m_isValid(false)
	, m_file(nullptr)
	, m_fields(new RomFields(fields, count))
	, m_fileType(FTYPE_ROM_IMAGE)
{
	// Clear the internal images field.
	memset(&m_images, 0, sizeof(m_images));
	memset(&m_imgpf, 0, sizeof(m_imgpf));

	if (!file)
		return;

	// dup() the file.
	m_file = file->dup();
}

RomData::~RomData()
{
	this->close();
	delete m_fields;

	// Delete the internal images.
	for (int i = ARRAY_SIZE(m_images)-1; i >= 0; i--) {
		delete m_images[i];
	}
}

/**
 * Is this ROM valid?
 * @return True if it is; false if it isn't.
 */
bool RomData::isValid(void) const
{
	return m_isValid;
}

/**
 * Close the opened file.
 */
void RomData::close(void)
{
	if (m_file) {
		delete m_file;
		m_file = nullptr;
	}
}

/**
 * Get the general file type.
 * @return General file type.
 */
RomData::FileType RomData::fileType(void) const
{
	return m_fileType;
}

/**
 * Get the general file type as a string.
 * @return General file type as a string, or nullptr if unknown.
 */
const rp_char *RomData::fileType_string(void) const
{
	switch (m_fileType) {
		case RomData::FTYPE_ROM_IMAGE:
			return _RP("ROM Image");
		case RomData::FTYPE_DISC_IMAGE:
			return _RP("Disc Image");
		case RomData::FTYPE_SAVE_FILE:
			return _RP("Save File");
		case RomData::FTYPE_EMBEDDED_DISC_IMAGE:
			return _RP("Embedded Disc Image");
		case RomData::FTYPE_APPLICATION_PACKAGE:
			return _RP("Application Package");
		case RomData::FTYPE_NFC_DUMP:
			return _RP("NFC Dump");
		case RomData::FTYPE_UNKNOWN:
		default:
			break;
	}

	return nullptr;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t RomData::supportedImageTypes(void) const
{
	// No images supported by default.
	return 0;
}

/**
 * Load an internal image.
 * Called by RomData::image() if the image data hasn't been loaded yet.
 * @param imageType Image type to load.
 * @return 0 on success; negative POSIX error code on error.
 */
int RomData::loadInternalImage(ImageType imageType)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}

	// No images supported by the base class.
	return -ENOENT;
}

/**
 * Load URLs for an external media type.
 * Called by RomData::extURL() if the URLs haven't been loaded yet.
 * @param imageType Image type to load.
 * @return 0 on success; negative POSIX error code on error.
 */
int RomData::loadURLs(ImageType imageType)
{
	assert(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_EXT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}

	// No images supported by the base class.
	return -ENOENT;
}

/**
 * Get the ROM Fields object.
 * @return ROM Fields object.
 */
const RomFields *RomData::fields(void) const
{
	if (!m_fields->isDataLoaded()) {
		// Data has not been loaded.
		// Load it now.
		int ret = const_cast<RomData*>(this)->loadFieldData();
		if (ret < 0)
			return nullptr;
	}
	return m_fields;
}

/**
 * Verify that the specified image type has been loaded.
 * @param imageType Image type.
 * @return 0 if loaded; negative POSIX error code on error.
 */
int RomData::verifyImageTypeLoaded(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}
	// TODO: Check supportedImageTypes()?

	int ret = 0;
	if (imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX) {
		// This is an internal image.
		// Make sure it's loaded.
		const int idx = imageType - IMG_INT_MIN;
		if (!m_images[idx]) {
			// Internal image has not been loaded.
			// Load it now.
			ret = const_cast<RomData*>(this)->loadInternalImage(imageType);
		}
	} else if (imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX) {
		// This is an external image.
		// Make sure the URL is loaded.
		const int idx = imageType - IMG_EXT_MIN;
		if (m_extURLs[idx].empty()) {
			// List of URLs has not been loaded.
			// Load it now.
			ret = const_cast<RomData*>(this)->loadURLs(imageType);
		}
	} else {
		// Should not get here...
		ret = -ERANGE;
	}

	return ret;
}

/**
 * Get an internal image from the ROM.
 *
 * NOTE: The rp_image is owned by this object.
 * Do NOT delete this object until you're done using this rp_image.
 *
 * @param imageType Image type to load.
 * @return Internal image, or nullptr if the ROM doesn't have one.
 */
const rp_image *RomData::image(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		return nullptr;
	}
	// TODO: Check supportedImageTypes()?

	if (verifyImageTypeLoaded(imageType) != 0)
		return nullptr;
	const int idx = imageType - IMG_INT_MIN;
	return m_images[idx];
}

/**
 * Get a list of URLs for an external media type.
 *
 * NOTE: The std::vector<extURL> is owned by this object.
 * Do NOT delete this object until you're done using this rp_image.
 *
 * @param imageType Image type.
 * @return List of URLs and cache keys, or nullptr if the ROM doesn't have one.
 */
const std::vector<RomData::ExtURL> *RomData::extURLs(ImageType imageType) const
{
	assert(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_EXT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return nullptr;
	}

	if (verifyImageTypeLoaded(imageType) != 0)
		return nullptr;
	const int idx = imageType - IMG_EXT_MIN;
	return &m_extURLs[idx];
}

/**
 * Scrape an image URL from a downloaded HTML page.
 * Needed if IMGPF_EXTURL_NEEDS_HTML_SCRAPING is set.
 * @param html HTML data.
 * @param size Size of HTML data.
 * @return Image URL, or empty string if not found or not supported.
 */
rp_string RomData::scrapeImageURL(const char *html, size_t size) const
{
	// Not supported in the base class.
	((void)html);
	((void)size);
	return rp_string();
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
uint32_t RomData::imgpf(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}
	// TODO: Check supportedImageTypes()?

	if (verifyImageTypeLoaded(imageType) != 0)
		return 0;
	return m_imgpf[imageType];
}

/**
 * Get the animated icon data.
 *
 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
 * object has an animated icon.
 *
 * @return Animated icon data, or nullptr if no animated icon is present.
 */
const IconAnimData *RomData::iconAnimData(void) const
{
	// No animated icon by default.
	return nullptr;
}

}
