/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomData.cpp: ROM data base class.                                       *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth                                  *
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
#include "RomData_p.hpp"

#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"
#include "img/rp_image.hpp"
#include "img/IconAnimData.hpp"

// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData {

/** RomDataPrivate **/

/**
 * Initialize a RomDataPrivate storage class.
 *
 * @param q RomData class.
 * @param file ROM file.
 * @param fields Array of ROM Field descriptions.
 * @param count Number of ROM Field descriptions.
 */
RomDataPrivate::RomDataPrivate(RomData *q, IRpFile *file)
	: q_ptr(q)
	, ref_count(1)
	, isValid(false)
	, file(nullptr)
	, fields(new RomFields())
	, fileType(RomData::FTYPE_ROM_IMAGE)
{
	// Clear the internal images field.
	memset(&images, 0, sizeof(images));
	memset(&imgpf, 0, sizeof(imgpf));

	if (!file)
		return;

	// dup() the file.
	this->file = file->dup();
}

/**
 * Initialize a RomDataPrivate storage class.
 *
 * @param q RomData class.
 * @param file ROM file.
 * @param fields Array of ROM Field descriptions.
 * @param count Number of ROM Field descriptions.
 */
RomDataPrivate::RomDataPrivate(RomData *q, IRpFile *file, const RomFields::Desc *fields, int count)
	: q_ptr(q)
	, ref_count(1)
	, isValid(false)
	, file(nullptr)
	, fields(new RomFields(fields, count))
	, fileType(RomData::FTYPE_ROM_IMAGE)
{
	// Clear the internal images field.
	memset(&images, 0, sizeof(images));
	memset(&imgpf, 0, sizeof(imgpf));

	if (!file)
		return;

	// dup() the file.
	this->file = file->dup();
}

RomDataPrivate::~RomDataPrivate()
{
	delete fields;

	// Delete the internal images.
	for (int i = ARRAY_SIZE(images)-1; i >= 0; i--) {
		delete images[i];
	}

	// Close the file if it's still open.
	delete this->file;
	this->file = nullptr;
}

/** Convenience functions. **/

static inline int calc_frac_part(int64_t size, int64_t mask)
{
	float f = (float)(size & (mask - 1)) / (float)mask;
	int frac_part = (int)(f * 1000.0f);

	// MSVC added round() and roundf() in MSVC 2013.
	// Use our own rounding code instead.
	int round_adj = (frac_part % 10 > 5);
	frac_part /= 10;
	frac_part += round_adj;
	return frac_part;
}

/**
 * Format a file size.
 * @param size File size.
 * @return Formatted file size.
 */
rp_string RomDataPrivate::formatFileSize(int64_t size)
{
	// TODO: Thousands formatting?
	char buf[64];

	const char *suffix;
	// frac_part is always 0 to 100.
	// If whole_part >= 10, frac_part is divided by 10.
	int whole_part, frac_part;

	// TODO: Optimize this?
	// TODO: Localize this?
	if (size < 0) {
		// Invalid size. Print the value as-is.
		suffix = "";
		whole_part = (int)size;
		frac_part = 0;
	} else if (size < (2LL << 10)) {
		suffix = (size == 1 ? " byte" : " bytes");
		whole_part = (int)size;
		frac_part = 0;
	} else if (size < (2LL << 20)) {
		suffix = " KB";
		whole_part = (int)(size >> 10);
		frac_part = calc_frac_part(size, (1LL << 10));
	} else if (size < (2LL << 30)) {
		suffix = " MB";
		whole_part = (int)(size >> 20);
		frac_part = calc_frac_part(size, (1LL << 20));
	} else if (size < (2LL << 40)) {
		suffix = " GB";
		whole_part = (int)(size >> 30);
		frac_part = calc_frac_part(size, (1LL << 30));
	} else if (size < (2LL << 50)) {
		suffix = " TB";
		whole_part = (int)(size >> 40);
		frac_part = calc_frac_part(size, (1LL << 40));
	} else if (size < (2LL << 60)) {
		suffix = " PB";
		whole_part = (int)(size >> 50);
		frac_part = calc_frac_part(size, (1LL << 50));
	} else /*if (size < (2ULL << 70))*/ {
		suffix = " EB";
		whole_part = (int)(size >> 60);
		frac_part = calc_frac_part(size, (1LL << 60));
	}

	int len;
	if (size < (2LL << 10)) {
		// Bytes or negative value. No fractional part.
		len = snprintf(buf, sizeof(buf), "%d%s", whole_part, suffix);
	} else {
		// TODO: Localized decimal point?
		int frac_digits = 2;
		if (whole_part >= 10) {
			int round_adj = (frac_part % 10 > 5);
			frac_part /= 10;
			frac_part += round_adj;
			frac_digits = 1;
		}
		len = snprintf(buf, sizeof(buf), "%d.%0*d%s",
			whole_part, frac_digits, frac_part, suffix);
	}

	if (len > (int)sizeof(buf))
		len = (int)sizeof(buf);
	return (len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
}

/** RomData **/

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
RomData::RomData(IRpFile *file)
	: d_ptr(new RomDataPrivate(this, file))
{ }

/**
 * ROM data base class.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * In addition, subclasses must pass an array of RomFieldDesc structs
 * using an allocated RomDataPrivate subclass.
 *
 * @param d RomDataPrivate subclass.
 */
RomData::RomData(RomDataPrivate *d)
	: d_ptr(d)
{ }

RomData::~RomData()
{
	delete d_ptr;
}

/**
 * Take a reference to this RomData* object.
 * @return this
 */
LibRomData::RomData *RomData::ref(void)
{
	// TODO: Atomic addition.
	RP_D(RomData);
	++d->ref_count;
	return this;
}

/**
 * Unreference this RomData* object.
 * If ref_count reaches 0, the RomData* object is deleted.
 */
void RomData::unref(void)
{
	// TODO: Atomic subtraction.
	RP_D(RomData);
	assert(d->ref_count > 0);
	if (--d->ref_count <= 0) {
		// All references removed.
		delete this;
	}
}

/**
 * Is this ROM valid?
 * @return True if it is; false if it isn't.
 */
bool RomData::isValid(void) const
{
	RP_D(const RomData);
	return d->isValid;
}

/**
 * Close the opened file.
 */
void RomData::close(void)
{
	RP_D(RomData);
	delete d->file;
	d->file = nullptr;
}

/**
 * Get the general file type.
 * @return General file type.
 */
RomData::FileType RomData::fileType(void) const
{
	RP_D(const RomData);
	return d->fileType;
}

/**
 * Get the general file type as a string.
 * @return General file type as a string, or nullptr if unknown.
 */
const rp_char *RomData::fileType_string(void) const
{
	RP_D(const RomData);
	assert(d->fileType >= FTYPE_UNKNOWN && d->fileType < FTYPE_LAST);
	if (d->fileType <= FTYPE_UNKNOWN || d->fileType >= FTYPE_LAST) {
		return nullptr;
	}

	static const rp_char *const fileType_names[] = {
		nullptr,			// FTYPE_UNKNOWN
		_RP("ROM Image"),		// FTYPE_ROM_IMAGE
		_RP("Disc Image"),		// FTYPE_DISC_IMAGE
		_RP("Save File"),		// FTYPE_SAVE_FILE
		_RP("Embedded Disc Image"),	// FTYPE_EMBEDDED_DISC_IMAGE
		_RP("Application Package"),	// FTYPE_APPLICATION_PACKAGE
		_RP("NFC Dump"),		// FTYPE_NFC_DUMP
		_RP("Disk Image"),		// FTYPE_DISK_IMAGE
		_RP("Executable"),		// FTYPE_EXECUTABLE
	};
	static_assert(ARRAY_SIZE(fileType_names) == FTYPE_LAST,
		"fileType_names[] needs to be updated.");

	return fileType_names[d->fileType];
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
	RP_D(const RomData);
	if (!d->fields->isDataLoaded()) {
		// Data has not been loaded.
		// Load it now.
		int ret = const_cast<RomData*>(this)->loadFieldData();
		if (ret < 0)
			return nullptr;
	}
	return d->fields;
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

	RP_D(const RomData);
	int ret = 0;
	if (imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX) {
		// This is an internal image.
		// Make sure it's loaded.
		const int idx = imageType - IMG_INT_MIN;
		if (!d->images[idx]) {
			// Internal image has not been loaded.
			// Load it now.
			ret = const_cast<RomData*>(this)->loadInternalImage(imageType);
		}
	} else if (imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX) {
		// This is an external image.
		// Make sure the URL is loaded.
		const int idx = imageType - IMG_EXT_MIN;
		if (d->extURLs[idx].empty()) {
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
	RP_D(const RomData);
	return d->images[idx];
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
	RP_D(const RomData);
	return &d->extURLs[idx];
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
	RP_D(const RomData);
	return d->imgpf[imageType];
}

/**
* Get name of an image type
* @param imageType Image type.
* @return String containing user-friendly name of an image type.
*/
const rp_char *RomData::getImageTypeName(ImageType imageType) {
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		return nullptr;
	}

	static const rp_char *const image_type_names[] = {
		// Internal
		_RP("Internal icon"),			// IMG_INT_ICON
		_RP("Internal banner"),			// IMG_INT_BANNER
		_RP("Internal media scan"),		// IMG_INT_MEDIA
		// External
		_RP("External media scan"),		// IMG_EXT_MEDIA
		_RP("External box scan"),		// IMG_EXT_BOX
		_RP("External box scan (both sides)"),	// IMG_EXT_BOX_FULL
		_RP("External box scan (3D version)"),	// IMG_EXT_BOX_3D
	};
	static_assert(ARRAY_SIZE(image_type_names) == IMG_EXT_MAX + 1,
		"image_type_names[] needs to be updated.");

	return image_type_names[imageType];
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
