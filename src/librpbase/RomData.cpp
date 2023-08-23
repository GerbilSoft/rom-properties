/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomData.cpp: ROM data base class.                                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth                                  *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RomData.hpp"
#include "RomData_p.hpp"

// Other rom-properties libraries
#include "libi18n/i18n.h"
#include "libcachecommon/CacheKeys.hpp"
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// C++ STL classes
using std::string;
using std::vector;

namespace LibRpBase {

/** RomDataPrivate **/

/**
 * Initialize a RomDataPrivate storage class.
 * @param file ROM file
 * @param pRomDataInfo RomData subclass information
 */
RomDataPrivate::RomDataPrivate(const IRpFilePtr &file, const RomDataInfo *pRomDataInfo)
	: pRomDataInfo(pRomDataInfo)
	, mimeType(nullptr)
	, fileType(RomData::FileType::ROM_Image)
	, isValid(false)
	, file(file)
	, filename(nullptr)
#ifdef _WIN32
	, filenameW(nullptr)
#endif /* _WIN32 */
	, isCompressed(false)
	, metaData(nullptr)
{
	assert(pRomDataInfo != nullptr);

	// Initialize i18n.
	rp_i18n_init();

	if (this->file) {
		// A file was specified. Copy important information.
		this->isCompressed = this->file->isCompressed();

#ifdef _WIN32
		// If this is RpFile, get the UTF-16 filename directly.
		RpFile *const rpFile = dynamic_cast<RpFile*>(this->file.get());
		if (rpFile) {
			const wchar_t *const filenameW = rpFile->filenameW();
			if (filenameW) {
				this->filenameW = wcsdup(filenameW);
			}
		}
#endif /* _WIN32 */

		// TODO: Don't set if filenameW was set?
		const char *const filename = this->file->filename();
		if (filename) {
			this->filename = strdup(filename);
		}
	}
}

RomDataPrivate::~RomDataPrivate()
{
	delete metaData;
	free(filename);
#ifdef _WIN32
	free(filenameW);
#endif /* _WIN32 */
}

/** Convenience functions. **/

/**
 * Get the GameTDB URL for a given game.
 * @param system System name.
 * @param type Image type.
 * @param region Region name.
 * @param gameID Game ID.
 * @param ext File extension, e.g. ".png" or ".jpg".
 * @return GameTDB URL.
 */
string RomDataPrivate::getURL_GameTDB(
	const char *system, const char *type,
	const char *region, const char *gameID,
	const char *ext)
{
	return rp_sprintf("https://art.gametdb.com/%s/%s/%s/%s%s",
		system, type, region, gameID, ext);
}

/**
 * Get the GameTDB cache key for a given game.
 * @param system System name.
 * @param type Image type.
 * @param region Region name.
 * @param gameID Game ID.
 * @param ext File extension, e.g. ".png" or ".jpg".
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
 * Get the RPDB URL for a given game.
 * @param system System name.
 * @param type Image type.
 * @param region Region name. (May be nullptr if no region is needed.)
 * @param gameID Game ID.
 * @param ext File extension, e.g. ".png" or ".jpg".
 * @return RPDB URL.
 */
string RomDataPrivate::getURL_RPDB(
	const char *system, const char *type,
	const char *region, const char *gameID,
	const char *ext)
{
	// Game ID may need to be urlencoded.
	const string gameID_urlencode = LibCacheCommon::urlencode(gameID);
	return rp_sprintf("https://rpdb.gerbilsoft.com/%s/%s/%s%s%s%s",
		system, type, (region ? region : ""), (region ? "/" : ""),
		gameID_urlencode.c_str(), ext);
}

/**
 * Get the RPDB cache key for a given game.
 * @param system System name.
 * @param type Image type.
 * @param region Region name. (May be nullptr if no region is needed.)
 * @param gameID Game ID.
 * @param ext File extension, e.g. ".png" or ".jpg".
 * @return RPDB cache key.
 */
string RomDataPrivate::getCacheKey_RPDB(
	const char *system, const char *type,
	const char *region, const char *gameID,
	const char *ext)
{
	return rp_sprintf("%s/%s/%s%s%s%s",
		system, type, (region ? region : ""), (region ? "/" : ""),
		gameID, ext);
}

/**
 * Select the best size for an image.
 * @param sizeDefs Image size definitions.
 * @param size Requested thumbnail dimension. (assuming a square thumbnail)
 * @return Image size definition, or nullptr on error.
 */
const RomData::ImageSizeDef *RomDataPrivate::selectBestSize(const vector<RomData::ImageSizeDef> &sizeDefs, int size)
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
			const auto sizeDefs_cend = sizeDefs.cend();
			for (auto iter = sizeDefs.cbegin()+1; iter != sizeDefs_cend; ++iter) {
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
			const auto sizeDefs_cend = sizeDefs.cend();
			for (auto iter = sizeDefs.cbegin()+1; iter != sizeDefs_cend; ++iter) {
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
	const auto sizeDefs_cend = sizeDefs.cend();
	for (auto iter = sizeDefs.cbegin()+1; iter != sizeDefs_cend; ++iter) {
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

	// Check for invalid BCD values.
	for (unsigned int i = 0; i < size; i++) {
		if ((bcd_tm[i] & 0x0F) > 9 || (bcd_tm[i] & 0xF0) > 0x90) {
			// Invalid BCD value.
			return -1;
		}
	}

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
	char chr;
	int ret = sscanf(buf, "%04d%02d%02d%02d%02d%02d%02d%c",
		&pvdtime.tm_year, &pvdtime.tm_mon, &pvdtime.tm_mday,
		&pvdtime.tm_hour, &pvdtime.tm_min, &pvdtime.tm_sec, &csec,
		&chr);
	if (ret != 7) {
		// Some argument wasn't parsed correctly.
		return -1;
	}

	// If year is 0, the entry is probably all zeroes.
	if (pvdtime.tm_year == 0) {
		return -1;
	}

	// Adjust values for struct tm.
	pvdtime.tm_year -= 1900;	// year - 1900
	pvdtime.tm_mon--;		// 0 == January

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
	// NOTE: Restricting to [-52, 52] as per the Linux kernel's isofs module.
	// TODO: Return the timezone offset separately.
	if (-52 <= tz_offset && tz_offset <= 52) {
		unixtime -= (static_cast<int>(tz_offset) * (15*60));
	}
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
	return ((bool)d->file);
}

/**
 * Close the opened file.
 */
void RomData::close(void)
{
	// Unreference the file.
	RP_D(RomData);
	d->file.reset();
}

/**
 * Get a reference to the internal file.
 * @return Reference to file, or nullptr on error.
 */
IRpFilePtr RomData::ref_file(void) const
{
	RP_D(const RomData);
	return d->file;
}

/**
 * Get the filename that was loaded.
 * @return Filename, or nullptr on error.
 */
const char *RomData::filename(void) const
{
	// TODO: filenameW() variant on Windows?
	RP_D(const RomData);
	return (d->filename != nullptr && d->filename[0] != '\0') ? d->filename : nullptr;
}

/**
 * Is the file compressed? (transparent decompression)
 * If it is, then ROM operations won't be allowed.
 * @return True if compressed; false if not.
 */
bool RomData::isCompressed(void) const
{
	RP_D(const RomData);
	return d->isCompressed;
}

/**
 * Get the class name for the user configuration.
 * @return Class name. (ASCII) (nullptr on error)
 */
const char *RomData::className(void) const
{
	RP_D(const RomData);
	return d->pRomDataInfo->className;
}

/**
 * Get the general file type.
 * @return General file type.
 */
RomData::FileType RomData::fileType(void) const
{
	RP_D(const RomData);
	assert(d->fileType > FileType::Unknown && d->fileType < FileType::Max);
	return d->fileType;
}

/**
 * FileType to string conversion function.
 * @param fileType File type
 * @return FileType as a string, or nullptr if unknown.
 */
const char *RomData::fileType_to_string(FileType fileType)
{
	assert(fileType >= FileType::Unknown && fileType < FileType::Max);
	if (unlikely(fileType < FileType::Unknown || fileType >= FileType::Max)) {
		fileType = FileType::Unknown;
	}

	static const char *const fileType_names[] = {
		// FileType::Unknown
		NOP_C_("RomData|FileType", "(unknown file type)"),
		// tr: FileType::ROM_Image
		NOP_C_("RomData|FileType", "ROM Image"),
		// tr: FileType::DiscImage
		NOP_C_("RomData|FileType", "Disc Image"),
		// tr: FileType::SaveFile
		NOP_C_("RomData|FileType", "Save File"),
		// tr: FileType::EmbeddedDiscImage
		NOP_C_("RomData|FileType", "Embedded Disc Image"),
		// tr: FileType::ApplicationPackage
		NOP_C_("RomData|FileType", "Application Package"),
		// tr: FileType::NFC_Dump
		NOP_C_("RomData|FileType", "NFC Dump"),
		// tr: FileType::DiskImage
		NOP_C_("RomData|FileType", "Disk Image"),
		// tr: FileType::Executable
		NOP_C_("RomData|FileType", "Executable"),
		// tr: FileType::DLL
		NOP_C_("RomData|FileType", "Dynamic Link Library"),
		// tr: FileType::DeviceDriver
		NOP_C_("RomData|FileType", "Device Driver"),
		// tr: FileType::ResourceLibrary
		NOP_C_("RomData|FileType", "Resource Library"),
		// tr: FileType::IconFile
		NOP_C_("RomData|FileType", "Icon File"),
		// tr: FileType::BannerFile
		NOP_C_("RomData|FileType", "Banner File"),
		// tr: FileType::Homebrew
		NOP_C_("RomData|FileType", "Homebrew Application"),
		// tr: FileType::eMMC_Dump
		NOP_C_("RomData|FileType", "eMMC Dump"),
		// tr: FileType::ContainerFile
		NOP_C_("RomData|FileType", "Container File"),
		// tr: FileType::FirmwareBinary
		NOP_C_("RomData|FileType", "Firmware Binary"),
		// tr: FileType::TextureFile
		NOP_C_("RomData|FileType", "Texture File"),
		// tr: FileType::RelocatableObject
		NOP_C_("RomData|FileType", "Relocatable Object File"),
		// tr: FileType::SharedLibrary
		NOP_C_("RomData|FileType", "Shared Library"),
		// tr: FileType::CoreDump
		NOP_C_("RomData|FileType", "Core Dump"),
		// tr: FileType::AudioFile
		NOP_C_("RomData|FileType", "Audio File"),
		// tr: FileType::BootSector
		NOP_C_("RomData|FileType", "Boot Sector"),
		// tr: FileType::Bundle (Mac OS X bundle)
		NOP_C_("RomData|FileType", "Bundle"),
		// tr: FileType::ResourceFile
		NOP_C_("RomData|FileType", "Resource File"),
		// tr: FileType::Partition
		NOP_C_("RomData|FileType", "Partition"),
		// tr: FileType::MetadataFile
		NOP_C_("RomData|FileType", "Metadata File"),
		// tr: FileType::PatchFile
		NOP_C_("RomData|FileType", "Patch File"),
	};
	static_assert(ARRAY_SIZE(fileType_names) == (int)FileType::Max,
		"fileType_names[] needs to be updated.");
 
	const char *const s_fileType = fileType_names[(int)fileType];
	assert(s_fileType != nullptr);
	return dpgettext_expr(RP_I18N_DOMAIN, "RomData|FileType", s_fileType);
}

/**
 * Get the general file type as a string.
 * @return General file type as a string, or nullptr if unknown.
 */
const char *RomData::fileType_string(void) const
{
	RP_D(const RomData);
	assert(d->fileType >= FileType::Unknown && d->fileType < FileType::Max);
	return fileType_to_string(d->fileType);
}

/**
 * Get the file's MIME type.
 * @return MIME type, or nullptr if unknown.
 */
const char *RomData::mimeType(void) const
{
	RP_D(const RomData);
	return d->mimeType;
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *RomData::supportedFileExtensions(void) const
{
	RP_D(const RomData);
	return d->pRomDataInfo->exts;
}

/**
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *RomData::supportedMimeTypes(void) const
{
	RP_D(const RomData);
	return d->pRomDataInfo->mimeTypes;
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
vector<RomData::ImageSizeDef> RomData::supportedImageSizes(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);

	// No images supported by default.
	RP_UNUSED(imageType);
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
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int RomData::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		pImage.reset();
		return -EINVAL;
	}

	// No images supported by the base class.
	pImage.reset();
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
	return -ENOTSUP;
}

/**
 * Get the ROM Fields object.
 * @return ROM Fields object.
 */
const RomFields *RomData::fields(void) const
{
	RP_D(const RomData);
	if (d->fields.empty()) {
		// Data has not been loaded.
		// Load it now.
		int ret = const_cast<RomData*>(this)->loadFieldData();
		if (ret < 0)
			return nullptr;
	}
	return &d->fields;
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
 * The retrieved image must be ref()'d by the caller if the
 * caller stores it instead of using it immediately.
 *
 * @param imageType Image type to load.
 * @return Internal image, or nullptr if the ROM doesn't have one.
 */
rp_image_const_ptr RomData::image(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		return nullptr;
	}
	// TODO: Check supportedImageTypes()?

	// Load the internal image.
	rp_image_const_ptr img;
	int ret = const_cast<RomData*>(this)->loadInternalImage(imageType, img);

	// SANITY CHECK: If loadInternalImage() returns 0,
	// img *must* be valid. Otherwise, it must be nullptr.
	assert((ret == 0 && (bool)img) ||
	       (ret != 0 && !img));

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
		return -EINVAL;
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
IconAnimDataConstPtr RomData::iconAnimData(void) const
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

/**
 * Get the list of operations that can be performed on this ROM.
 * @return List of operations.
 */
vector<RomData::RomOp> RomData::romOps(void) const
{
	RP_D(const RomData);
	vector<RomOp> v_ops = romOps_int();

	if (d->isCompressed) {
		// Some RomOps can't be run on a compressed file.
		// Disable those that can't.
		// TODO: Indicate why they're disabled?
		for (RomData::RomOp &op : v_ops) {
			if (op.flags & RomOp::ROF_REQ_WRITABLE) {
				op.flags &= ~RomOp::ROF_ENABLED;
			}
		}
	}

	return v_ops;
}

/**
 * Perform a ROM operation.
 * @param id		[in] Operation index.
 * @param pParams	[in/out] Parameters and results. (for e.g. UI updates)
 * @return 0 on success; negative POSIX error code on error.
 */
int RomData::doRomOp(int id, RomOpParams *pParams)
{
	RP_D(RomData);
	// TODO: Function to retrieve only a single RomOp.
	const vector<RomOp> v_ops = romOps_int();
	assert(id >= 0);
	assert(id < (int)v_ops.size());
	assert(pParams != nullptr);
	if (id < 0 || id >= (int)v_ops.size() || !pParams) {
		return -EINVAL;
	}

	bool closeFileAfter;
	if (d->file) {
		closeFileAfter = false;
	} else {
		// Reopen the file.
		closeFileAfter = true;
		RpFile *file;
#ifdef _WIN32
		if (d->filenameW) {
			file = new RpFile(d->filenameW, RpFile::FM_OPEN_WRITE);
		} else
#endif /* _WIN32 */
		{
			file = new RpFile(d->filename, RpFile::FM_OPEN_WRITE);
		}

		if (!file->isOpen()) {
			// Error opening the file.
			int ret = -file->lastError();
			if (ret == 0) {
				ret = -EIO;
			}
			pParams->status = ret;
			pParams->msg = C_("RomData", "Unable to reopen the file for writing.");
			delete file;
			return ret;
		}
		d->file.reset(file);
	}

	// If the ROM operation requires a writable file,
	// make sure it's writable.
	if (v_ops[id].flags & RomOp::ROF_REQ_WRITABLE) {
		// Writable file is required.
		if (d->file->isCompressed()) {
			// Cannot write to a compressed file.
			pParams->status = -EIO;
			pParams->msg = C_("RomData", "Cannot perform this ROM operation on a compressed file.");
			if (closeFileAfter) {
				d->file.reset();
			}
			return -EIO;
		}

		if (!d->file->isWritable()) {
			// File is not writable. We'll need to reopen it.
			int ret = d->file->makeWritable();
			if (ret != 0) {
				// Error making the file writable.
				pParams->status = ret;
				pParams->msg = C_("RomData", "Cannot perform this ROM operation on a read-only file.");
				if (closeFileAfter) {
					d->file.reset();
				}
				return ret;
			}
		}
	}

	int ret = doRomOp_int(id, pParams);
	if (closeFileAfter) {
		d->file.reset();
	}
	return ret;
}

/**
 * Get the list of operations that can be performed on this ROM.
 * Internal function; called by RomData::romOps().
 * @return List of operations.
 */
vector<RomData::RomOp> RomData::romOps_int(void) const
{
	// Default implementation has no ROM operations.
	return {};
}

/**
 * Perform a ROM operation.
 * Internal function; called by RomData::doRomOp().
 * @param id		[in] Operation index.
 * @param pParams	[in/out] Parameters and results. (for e.g. UI updates)
 * @return 0 on success; positive for "field updated" (subtract 1 for index); negative POSIX error code on error.
 */
int RomData::doRomOp_int(int id, RomOpParams *pParams)
{
	// Default implementation has no ROM operations.
	RP_UNUSED(id);
	pParams->status = -ENOTSUP;
	pParams->msg = C_("RomData", "RomData object does not support any ROM operations.");
	return -ENOTSUP;
}

/**
 * Check for "viewed" achievements.
 * @return Number of achievements unlocked.
 */
int RomData::checkViewedAchievements(void) const
{
	// No "viewed" achievements by default.
	return 0;
}

}
