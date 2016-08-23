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
#include "rp_image.hpp"
#include "common.h"

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
{
	// Clear the internal images field.
	memset(&m_images, 0, sizeof(m_images));

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

	const int idx = imageType - IMG_INT_MIN;
	if (!m_images[idx]) {
		// Internal image has not been loaded.
		// Load it now.
		int ret = const_cast<RomData*>(this)->loadInternalImage(imageType);
		if (ret < 0)
			return nullptr;
	}

	return m_images[idx];
}

/**
 * Get a list of URLs for an external media type.
 *
 * NOTE: The std::vector<rp_string> is owned by this object.
 * Do NOT delete this object until you're done using this rp_image.
 *
 * @param imageType Image type.
 * @return List of URLs, or nullptr if the ROM doesn't have one.
 */
const std::vector<rp_string> *RomData::extURLs(ImageType imageType) const
{
	assert(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_EXT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return nullptr;
	}

	// TODO: Check supportedImageTypes()?

	const int idx = imageType - IMG_EXT_MIN;
	if (m_extURLs[idx].empty()) {
		// List of URLs has not been loaded.
		// Load it now.
		int ret = const_cast<RomData*>(this)->loadURLs(imageType);
		if (ret < 0)
			return nullptr;
	}

	return &m_extURLs[idx];
}

/**
 * Get a cache key for an external media type.
 * @return Cache key, or nullptr if not cacheable.
 */
const rp_char *RomData::cacheKey(ImageType imageType) const
{
	assert(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_EXT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return nullptr;
	}

	// cacheKeys is dependent on extURLs.
	const int idx = imageType - IMG_EXT_MIN;
	if (m_extURLs[idx].empty()) {
		// List of URLs has not been loaded.
		// Load it now.
		int ret = const_cast<RomData*>(this)->loadURLs(imageType);
		if (ret < 0)
			return nullptr;
	}

	return (m_cacheKey[idx].empty() ? nullptr : m_cacheKey[idx].c_str());
}

}
