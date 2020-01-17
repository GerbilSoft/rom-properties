/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomData.cpp: ROM data base class.                                       *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth                                  *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RomData.hpp"
#include "RomData_p.hpp"

#include "libi18n/i18n.h"

// librpthreads
#include "librpthreads/Atomics.h"

// C++ STL classes.
using std::string;
using std::vector;

using LibRpTexture::rp_image;

namespace LibRpBase {

/** RomDataPrivate **/

/**
 * Initialize a RomDataPrivate storage class.
 *
 * @param q RomData class.
 * @param file ROM file.
 */
RomDataPrivate::RomDataPrivate(RomData *q, IRpFile *file)
	: q_ptr(q)
	, ref_cnt(1)
	, isValid(false)
	, file(nullptr)
	, fields(new RomFields())
	, metaData(nullptr)
	, className(nullptr)
	, fileType(RomData::FTYPE_ROM_IMAGE)
{
	// Initialize i18n.
	rp_i18n_init();

	if (file) {
		// Reference the file.
		this->file = file->ref();
	}
}

RomDataPrivate::~RomDataPrivate()
{
	delete fields;
	delete metaData;

	// Unreference the file.
	if (this->file) {
		this->file->unref();
	}
}

/** Convenience functions. **/

/**
 * Get the GameTDB URL for a given game.
 * @param system System name.
 * @param type Image type.
 * @param region Region name.
 * @param gameID Game ID.
 * @param ext File extension, e.g. ".png" or ".jpg".
 * TODO: PAL multi-region selection?
 * @return GameTDB URL.
 */
string RomDataPrivate::getURL_GameTDB(
	const char *system, const char *type,
	const char *region, const char *gameID,
	const char *ext)
{
	return rp_sprintf("http://art.gametdb.com/%s/%s/%s/%s%s",
		system, type, region, gameID, ext);
}

/**
 * Get the GameTDB cache key for a given game.
 * @param system System name.
 * @param type Image type.
 * @param region Region name.
 * @param gameID Game ID.
 * @param ext File extension, e.g. ".png" or ".jpg".
 * TODO: PAL multi-region selection?
 * @return GameTDB cache key.
 */
string RomDataPrivate::getCacheKey_GameTDB(
	const char *system, const char *type,
	const char *region, const char *gameID,
	const char *ext)
{
	return rp_sprintf("%s/%s/%s/%s%s",
		system, type, region, gameID, ext);
}

/**
 * Select the best size for an image.
 * @param sizeDefs Image size definitions.
 * @param size Requested thumbnail dimension. (assuming a square thumbnail)
 * @return Image size definition, or nullptr on error.
 */
const RomData::ImageSizeDef *RomDataPrivate::selectBestSize(const std::vector<RomData::ImageSizeDef> &sizeDefs, int size)
{
	if (sizeDefs.empty() || size < RomData::IMAGE_SIZE_MIN_VALUE) {
		// No sizes, or invalid size value.
		return nullptr;
	} else if (sizeDefs.size() == 1) {
		// Only one size.
		return &sizeDefs[0];
	}

	// Check for a "special" size value.
	switch (size) {
		case RomData::IMAGE_SIZE_DEFAULT:
			// Default image.
			return &sizeDefs[0];

		case RomData::IMAGE_SIZE_SMALLEST: {
			// Find the smallest image.
			const RomData::ImageSizeDef *ret = &sizeDefs[0];
			int sz = std::min(ret->width, ret->height);
			for (auto iter = sizeDefs.begin()+1; iter != sizeDefs.end(); ++iter) {
				const RomData::ImageSizeDef *sizeDef = &(*iter);
				if (sizeDef->width < sz || sizeDef->height < sz) {
					ret = sizeDef;
					sz = std::min(sizeDef->width, sizeDef->height);
				}
			}
			return ret;
		}

		case RomData::IMAGE_SIZE_LARGEST: {
			// Find the largest image.
			const RomData::ImageSizeDef *ret = &sizeDefs[0];
			int sz = std::max(ret->width, ret->height);
			for (auto iter = sizeDefs.begin()+1; iter != sizeDefs.end(); ++iter) {
				const RomData::ImageSizeDef *sizeDef = &(*iter);
				if (sizeDef->width > sz || sizeDef->height > sz) {
					ret = sizeDef;
					sz = std::max(sizeDef->width, sizeDef->height);
				}
			}
			return ret;
		}

		default:
			break;
	}

	// Find the largest image that has at least one dimension that
	// is >= the requested size. If no image is >= the requested
	// size, use the largest image.
	// TODO: Check width/height separately?
	const RomData::ImageSizeDef *ret = &sizeDefs[0];
	int sz = std::max(ret->width, ret->height);
	if (sz == size) {
		// Found a match already.
		return ret;
	}
	for (auto iter = sizeDefs.cbegin()+1; iter != sizeDefs.cend(); ++iter) {
		const RomData::ImageSizeDef *sizeDef = &(*iter);
		const int szchk = std::max(sizeDef->width, sizeDef->height);
		if (sz >= size) {
			// We already found an image >= size.
			// Only use this image if its largest dimension is
			// >= size and < sz.
			if (szchk >= size && szchk < sz) {
				// Found a better match.
				sz = szchk;
				ret = sizeDef;
			}
		} else {
			// Use this image if its largest dimension is > sz.
			if (szchk > sz) {
				// Found a better match.
				sz = szchk;
				ret = sizeDef;
			}
		}

		if (sz == size) {
			// Exact match!
			// TODO: Verify width/height separately?
			break;
		}
	}

	return ret;
}

/**
 * Convert an ASCII release date in YYYYMMDD format to Unix time_t.
 * This format is used by Sega Saturn and Dreamcast.
 * @param ascii_date ASCII release date. (Must be 8 characters.)
 * @return Unix time_t, or -1 on error.
 */
time_t RomDataPrivate::ascii_yyyymmdd_to_unix_time(const char *ascii_date)
{
	// Release date format: "YYYYMMDD"

	// Convert the date to an unsigned integer first.
	unsigned int ymd = 0;
	for (unsigned int i = 0; i < 8; i++) {
		if (unlikely(!ISDIGIT(ascii_date[i]))) {
			// Invalid digit.
			return -1;
		}
		ymd *= 10;
		ymd += (ascii_date[i] & 0xF);
	}

	// Sanity checks:
	// - Must be higher than 19000101.
	// - Must be lower than 99991231.
	if (unlikely(ymd < 19000101 || ymd > 99991231)) {
		// Invalid date.
		return -1;
	}

	// Convert to Unix time.
	// NOTE: struct tm has some oddities:
	// - tm_year: year - 1900
	// - tm_mon: 0 == January
	struct tm ymdtime;

	ymdtime.tm_year = (ymd / 10000) - 1900;
	ymdtime.tm_mon  = ((ymd / 100) % 100) - 1;
	ymdtime.tm_mday = ymd % 100;

	// Time portion is empty.
	ymdtime.tm_hour = 0;
	ymdtime.tm_min = 0;
	ymdtime.tm_sec = 0;

	// tm_wday and tm_yday are output variables.
	ymdtime.tm_wday = 0;
	ymdtime.tm_yday = 0;
	ymdtime.tm_isdst = 0;

	// If conversion fails, this will return -1.
	return timegm(&ymdtime);
}

/**
 * Convert a BCD timestamp to Unix time.
 * @param bcd_tm BCD timestamp. (YY YY MM DD HH mm ss)
 * @param size Size of BCD timestamp. (4 or 7)
 * @return Unix time, or -1 if an error occurred.
 *
 * NOTE: -1 is a valid Unix timestamp (1970/01/01), but is
 * not likely to be valid, since the first programmable
 * video game consoles were released in the late 1970s.
 */
time_t RomDataPrivate::bcd_to_unix_time(const uint8_t *bcd_tm, size_t size)
{
	// Convert BCD time to Unix time.
	// NOTE: struct tm has some oddities:
	// - tm_year: year - 1900
	// - tm_mon: 0 == January
	struct tm bcdtime;

	// TODO: Check for invalid BCD values.
	if (size >= 4) {
		bcdtime.tm_year = ((bcd_tm[0] >> 4) * 1000) +
				  ((bcd_tm[0] & 0x0F) * 100) +
				  ((bcd_tm[1] >> 4) * 10) +
				   (bcd_tm[1] & 0x0F) - 1900;
		bcdtime.tm_mon  = ((bcd_tm[2] >> 4) * 10) +
				   (bcd_tm[2] & 0x0F) - 1;
		bcdtime.tm_mday = ((bcd_tm[3] >> 4) * 10) +
				   (bcd_tm[3] & 0x0F);
		if (size >= 7) {
			bcdtime.tm_hour = ((bcd_tm[4] >> 4) * 10) +
					   (bcd_tm[4] & 0x0F);
			bcdtime.tm_min  = ((bcd_tm[5] >> 4) * 10) +
					   (bcd_tm[5] & 0x0F);
			bcdtime.tm_sec  = ((bcd_tm[6] >> 4) * 10) +
					   (bcd_tm[6] & 0x0F);
		} else {
			// No HH/mm/ss portion.
			bcdtime.tm_hour = 0;
			bcdtime.tm_min  = 0;
			bcdtime.tm_sec  = 0;
		}
	} else {
		// Invalid BCD time.
		return -1;
	}

	// tm_wday and tm_yday are output variables.
	bcdtime.tm_wday = 0;
	bcdtime.tm_yday = 0;
	bcdtime.tm_isdst = 0;

	// If conversion fails, this will return -1.
	return timegm(&bcdtime);
}

/**
 * Convert an ISO PVD timestamp to UNIX time.
 * @param pvd_time PVD timestamp (16-char buffer)
 * @param tz_offset PVD timezone offset
 * @return UNIX time, or -1 if invalid or not set.
 */
time_t RomDataPrivate::pvd_time_to_unix_time(const char pvd_time[16], int8_t tz_offset)
{
	// TODO: Verify tz_offset range? [-48,+52]
	assert(pvd_time != nullptr);
	if (!pvd_time)
		return -1;

	// PVD time is in ASCII format:
	// YYYYMMDDHHmmssccz
	// - YYYY: Year
	// - MM: Month
	// - DD: Day
	// - HH: Hour
	// - mm: Minute
	// - ss: Second
	// - cc: Centisecond (not supported in UNIX time)
	// - z: (int8) Timezone offset in 15min intervals: [0, 100] -> [-48, 52]
	//   - -48: GMT-1200
	//   -  52: GMT+1300

	// NOTE: pvd_time is NOT null-terminated, so we need to
	// copy it to a temporary buffer.
	char buf[17];
	memcpy(buf, pvd_time, 16);
	buf[16] = '\0';

	struct tm pvdtime;
	int csec;
	int ret = sscanf(buf, "%04d%02d%02d%02d%02d%02d%02d",
		&pvdtime.tm_year, &pvdtime.tm_mon, &pvdtime.tm_mday,
		&pvdtime.tm_hour, &pvdtime.tm_min, &pvdtime.tm_sec, &csec);
	if (ret != 7) {
		// Some argument wasn't parsed correctly.
		return -1;
	}

	// If year is 0, the entry is probably all zeroes.
	if (pvdtime.tm_year == 0) {
		return -1;
	}

	// Adjust values for struct tm.
	pvdtime.tm_year -= 1900;	// struct tm: year - 1900
	pvdtime.tm_mon--;		// struct tm: 0-11

	// tm_wday and tm_yday are output variables.
	pvdtime.tm_wday = 0;
	pvdtime.tm_yday = 0;
	pvdtime.tm_isdst = 0;

	// If conversion fails, this will return -1.
	time_t unixtime = timegm(&pvdtime);
	if (unixtime == -1)
		return -1;

	// Convert to UTC using the timezone offset.
	// NOTE: Timezone offset is negative for west of GMT,
	// so we need to subtract it from the UNIX timestamp.
	// TODO: Return the timezone offset separately.
	unixtime -= (static_cast<int>(tz_offset) * (15*60));
	return unixtime;
}

/** RomData **/

/**
 * ROM data base class.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * @param file ROM file.
 */
RomData::RomData(IRpFile *file)
	: d_ptr(new RomDataPrivate(this, file))
{ }

/**
 * ROM data base class.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
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
RomData *RomData::ref(void)
{
	RP_D(RomData);
	ATOMIC_INC_FETCH(&d->ref_cnt);
	return this;
}

/**
 * Unreference this RomData* object.
 * If ref_cnt reaches 0, the RomData* object is deleted.
 */
void RomData::unref(void)
{
	RP_D(RomData);
	assert(d->ref_cnt > 0);
	if (ATOMIC_DEC_FETCH(&d->ref_cnt) <= 0) {
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
 * Is the file open?
 * @return True if the file is open; false if it isn't.
 */
bool RomData::isOpen(void) const
{
	RP_D(const RomData);
	return (d->file != nullptr);
}

/**
 * Close the opened file.
 */
void RomData::close(void)
{
	// Unreference the file.
	RP_D(RomData);
	if (d->file) {
		d->file->unref();
		d->file = nullptr;
	}
}

/**
 * Get the class name for the user configuration.
 * @return Class name. (ASCII) (nullptr on error)
 */
const char *RomData::className(void) const
{
	RP_D(const RomData);
	assert(d->className != nullptr);
	return d->className;
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
const char *RomData::fileType_string(void) const
{
	RP_D(const RomData);
	assert(d->fileType >= FTYPE_UNKNOWN && d->fileType < FTYPE_LAST);
	if (d->fileType <= FTYPE_UNKNOWN || d->fileType >= FTYPE_LAST) {
		return nullptr;
	}

	static const char *const fileType_names[] = {
		// FTYPE_UNKNOWN
		nullptr,
		// tr: FTYPE_ROM_IMAGE
		NOP_C_("RomData|FileType", "ROM Image"),
		// tr: FTYPE_DISC_IMAGE
		NOP_C_("RomData|FileType", "Disc Image"),
		// tr: FTYPE_SAVE_FILE
		NOP_C_("RomData|FileType", "Save File"),
		// tr: FTYPE_EMBEDDED_DISC_IMAGE
		NOP_C_("RomData|FileType", "Embedded Disc Image"),
		// tr: FTYPE_APPLICATION_PACKAGE
		NOP_C_("RomData|FileType", "Application Package"),
		// tr: FTYPE_NFC_DUMP
		NOP_C_("RomData|FileType", "NFC Dump"),
		// tr: FTYPE_DISK_IMAGE
		NOP_C_("RomData|FileType", "Disk Image"),
		// tr: FTYPE_EXECUTABLE
		NOP_C_("RomData|FileType", "Executable"),
		// tr: FTYPE_DLL
		NOP_C_("RomData|FileType", "Dynamic Link Library"),
		// tr: FTYPE_DEVICE_DRIVER
		NOP_C_("RomData|FileType", "Device Driver"),
		// tr: FTYPE_RESOURCE_LIBRARY
		NOP_C_("RomData|FileType", "Resource Library"),
		// tr: FTYPE_ICON_FILE
		NOP_C_("RomData|FileType", "Icon File"),
		// tr: FTYPE_BANNER_FILE
		NOP_C_("RomData|FileType", "Banner File"),
		// tr: FTYPE_HOMEBREW
		NOP_C_("RomData|FileType", "Homebrew Application"),
		// tr: FTYPE_EMMC_DUMP
		NOP_C_("RomData|FileType", "eMMC Dump"),
		// tr: FTYPE_CONTAINER_FILE
		NOP_C_("RomData|FileType", "Container File"),
		// tr: FTYPE_FIRMWARE_BINARY
		NOP_C_("RomData|FileType", "Firmware Binary"),
		// tr: FTYPE_TEXTURE_FILE
		NOP_C_("RomData|FileType", "Texture File"),
		// tr: FTYPE_RELOCATABLE_OBJECT
		NOP_C_("RomData|FileType", "Relocatable Object File"),
		// tr: FTYPE_SHARED_LIBRARY
		NOP_C_("RomData|FileType", "Shared Library"),
		// tr: FTYPE_CORE_DUMP
		NOP_C_("RomData|FileType", "Core Dump"),
		// tr: FTYPE_AUDIO_FILE
		NOP_C_("RomData|FileType", "Audio File"),
		// tr: FTYPE_BOOT_SECTOR
		NOP_C_("RomData|FileType", "Boot Sector"),
		// tr: FTYPE_BUNDLE (Mac OS X bundle)
		NOP_C_("RomData|FileType", "Bundle"),
		// tr: FTYPE_RESOURCE_FILE
		NOP_C_("RomData|FileType", "Resource File"),
		// tr: FTYPE_PARTITION
		NOP_C_("RomData|FileType", "Partition"),
		// tr: FTYPE_METADATA_FILE
		NOP_C_("RomData|FileType", "Metadata File"),
	};
	static_assert(ARRAY_SIZE(fileType_names) == FTYPE_LAST,
		"fileType_names[] needs to be updated.");
 
	const char *const fileType = fileType_names[d->fileType];
	if (fileType != nullptr) {
		return dpgettext_expr(RP_I18N_DOMAIN, "RomData|FileType", fileType);
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
 * Get a list of all available image sizes for the specified image type.
 *
 * The first item in the returned vector is the "default" size.
 * If the width/height is 0, then an image exists, but the size is unknown.
 *
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
std::vector<RomData::ImageSizeDef> RomData::supportedImageSizes(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);

	// No images supported by default.
	RP_UNUSED(imageType);
	return std::vector<ImageSizeDef>();
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

	// No imgpf by default.
	RP_UNUSED(imageType);
	return 0;
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Pointer to const rp_image* to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int RomData::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	assert(pImage != nullptr);
	if (!pImage) {
		// Invalid parameters.
		return -EINVAL;
	} else if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		*pImage = nullptr;
		return -ERANGE;
	}

	// No images supported by the base class.
	RP_UNUSED(imageType);
	*pImage = nullptr;
	return -ENOENT;
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int RomData::loadMetaData(void)
{
	// Not implemented for the base class.
	return -ENOSYS;
}

/**
 * Get the ROM Fields object.
 * @return ROM Fields object.
 */
const RomFields *RomData::fields(void) const
{
	RP_D(const RomData);
	if (d->fields->empty()) {
		// Data has not been loaded.
		// Load it now.
		int ret = const_cast<RomData*>(this)->loadFieldData();
		if (ret < 0)
			return nullptr;
	}
	return d->fields;
}

/**
 * Get the ROM Metadata object.
 * @return ROM Metadata object.
 */
const RomMetaData *RomData::metaData(void) const
{
	RP_D(const RomData);
	if (!d->metaData || d->metaData->empty()) {
		// Data has not been loaded.
		// Load it now.
		int ret = const_cast<RomData*>(this)->loadMetaData();
		if (ret < 0)
			return nullptr;
	}
	return d->metaData;
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

	// Load the internal image.
	// The subclass maintains ownership of the image.
#ifdef _DEBUG
	// TODO: Verify casting on 32-bit.
	#define INVALID_IMG_PTR ((const rp_image*)((intptr_t)-1LL))
	const rp_image *img = INVALID_IMG_PTR;
#else /* !_DEBUG */
	const rp_image *img;
#endif
	int ret = const_cast<RomData*>(this)->loadInternalImage(imageType, &img);

	// SANITY CHECK: If loadInternalImage() returns 0,
	// img *must* be valid. Otherwise, it must be nullptr.
	assert((ret == 0 && img != nullptr) ||
	       (ret != 0 && img == nullptr));

	// SANITY CHECK: `img` must not be -1LL.
	assert(img != INVALID_IMG_PTR);

	return (ret == 0 ? img : nullptr);
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
int RomData::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	assert(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_EXT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}
	assert(pExtURLs != nullptr);
	if (!pExtURLs) {
		// No vector.
		return -EINVAL;
	}

	// No external URLs by default.
	RP_UNUSED(size);
	pExtURLs->clear();
	return -ENOENT;
}

/**
 * Scrape an image URL from a downloaded HTML page.
 * Needed if IMGPF_EXTURL_NEEDS_HTML_SCRAPING is set.
 * @param html HTML data.
 * @param size Size of HTML data.
 * @return Image URL, or empty string if not found or not supported.
 */
string RomData::scrapeImageURL(const char *html, size_t size) const
{
	// Not supported in the base class.
	RP_UNUSED(html);
	RP_UNUSED(size);
	return string();
}

/**
* Get name of an image type
* @param imageType Image type.
* @return String containing user-friendly name of an image type.
*/
const char *RomData::getImageTypeName(ImageType imageType) {
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		return nullptr;
	}

	static const char *const imageType_names[] = {
		/** Internal **/

		// tr: IMG_INT_ICON
		NOP_C_("RomData|ImageType", "Internal icon"),
		// tr: IMG_INT_BANNER
		NOP_C_("RomData|ImageType", "Internal banner"),
		// tr: IMG_INT_MEDIA
		NOP_C_("RomData|ImageType", "Internal media scan"),
		// tr: IMG_INT_IMAGE
		NOP_C_("RomData|ImageType", "Internal image"),

		/** External **/

		// tr: IMG_EXT_MEDIA
		NOP_C_("RomData|ImageType", "External media scan"),
		// tr: IMG_EXT_COVER
		NOP_C_("RomData|ImageType", "External cover scan"),
		// tr: IMG_EXT_COVER_3D
		NOP_C_("RomData|ImageType", "External cover scan (3D version)"),
		// tr: IMG_EXT_COVER_FULL
		NOP_C_("RomData|ImageType", "External cover scan (front and back)"),
		// tr: IMG_EXT_BOX
		NOP_C_("RomData|ImageType", "External box scan"),
		// tr: IMG_EXT_TITLE_SCREEN
		NOP_C_("RomData|ImageType", "External title screen"),
	};
	static_assert(ARRAY_SIZE(imageType_names) == IMG_EXT_MAX + 1,
		"imageType_names[] needs to be updated.");

	return dpgettext_expr(RP_I18N_DOMAIN, "RomData|ImageType", imageType_names[imageType]);
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

/**
 * Does this ROM image have "dangerous" permissions?
 *
 * @return True if the ROM image has "dangerous" permissions; false if not.
 */
bool RomData::hasDangerousPermissions(void) const
{
	// No dangerous permissions by default.
	return false;
}

}
