/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomData_p.hpp: ROM data base class. (PRIVATE CLASS)                     *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cstdint>

// C++ includes
#include <vector>

#include "RomData.hpp"

// TODO: Remove from here and add to each RomData subclass?
#include "RomFields.hpp"
#include "RomMetaData.hpp"

namespace LibRpFile {
	class IRpFile;
}
namespace LibRpTexture {
	class rp_image;
}

namespace LibRpBase {

class RomFields;
class RomMetaData;

struct RomDataInfo {
	const char *className;		// Class name for user configuration (ASCII)
	const char *const *exts;	// Supported file extensions
	const char *const *mimeTypes;	// Supported MIME types
};

class NOVTABLE RomDataPrivate
{
protected:
	/**
	 * Initialize a RomDataPrivate storage class.
	 * NOTE: Can only be called by a subclass's constructor.
	 * @param file ROM file
	 * @param pRomDataInfo RomData subclass information
	 */
	RomDataPrivate(const LibRpFile::IRpFilePtr &file, const RomDataInfo *pRomDataInfo);
public:
	virtual ~RomDataPrivate() = default;

private:
	RP_DISABLE_COPY(RomDataPrivate)

public:
	/** These fields must be set by RomData subclasses in their constructors. **/
	const RomDataInfo *pRomDataInfo;// RomData subclass information
	const char *mimeType;		// MIME type (ASCII) (default is nullptr)
	RomData::FileType fileType;	// File type (default is FileType::ROM_Image)
	bool isValid;			// Subclass must set this to true if the ROM is valid.
	bool isPAL;			// Is PAL?
public:
	/** These fields are set by RomData's own constructor. **/
	bool isCompressed;		// True if the file is compressed. (transparent decompression)
	LibRpFile::IRpFilePtr file;	// Open file
	std::string filename;		// Filename (UTF-8)
#ifdef _WIN32
	std::wstring filenameW;		// Filename (UTF-16; Windows only)
#endif /* _WIN32 */
public:
	RomFields fields;		// ROM fields
	RomMetaData metaData;		// ROM metadata

public:
	/** Convenience functions **/

	/** External image URL functions **/

	/**
	 * Get the GameTDB URL for a given game.
	 * @param system System name.
	 * @param type Image type.
	 * @param region Region name.
	 * @param gameID Game ID.
	 * @param ext File extension, e.g. ".png" or ".jpg".
	 * @return GameTDB URL.
	 */
	static std::string getURL_GameTDB(
		const char *system, const char *type,
		const char *region, const char *gameID,
		const char *ext);

	/**
	 * Get the GameTDB cache key for a given game.
	 * @param system System name.
	 * @param type Image type.
	 * @param region Region name.
	 * @param gameID Game ID.
	 * @param ext File extension, e.g. ".png" or ".jpg".
	 * @return GameTDB cache key.
	 */
	static std::string getCacheKey_GameTDB(
		const char *system, const char *type,
		const char *region, const char *gameID,
		const char *ext);

	/**
	 * Get the RPDB URL for a given game.
	 * @param system System name.
	 * @param type Image type.
	 * @param region Region name. (May be nullptr if no region is needed.)
	 * @param gameID Game ID.
	 * @param ext File extension, e.g. ".png" or ".jpg".
	 * @return RPDB URL.
	 */
	static std::string getURL_RPDB(
		const char *system, const char *type,
		const char *region, const char *gameID,
		const char *ext);

	/**
	 * Get the RPDB cache key for a given game.
	 * @param system System name.
	 * @param type Image type.
	 * @param region Region name. (May be nullptr if no region is needed.)
	 * @param gameID Game ID.
	 * @param ext File extension, e.g. ".png" or ".jpg".
	 * @return RPDB cache key.
	 */
	static std::string getCacheKey_RPDB(
		const char *system, const char *type,
		const char *region, const char *gameID,
		const char *ext);

	/**
	 * Select the best size for an image.
	 * @param sizeDefs Image size definitions.
	 * @param size Requested thumbnail dimension. (assuming a square thumbnail)
	 * @return Image size definition, or nullptr on error.
	 */
	static const RomData::ImageSizeDef *selectBestSize(const std::vector<RomData::ImageSizeDef> &sizeDefs, int size);

	/** Time conversion functions **/

	/**
	 * Convert an ASCII release date in YYYYMMDD format to Unix time_t.
	 * This format is used by Sega Saturn and Dreamcast.
	 * @param ascii_date ASCII release date. (Must be 8 characters.)
	 * @return Unix time_t, or -1 on error.
	 */
	static time_t ascii_yyyymmdd_to_unix_time(const char *ascii_date);

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
	static time_t bcd_to_unix_time(const uint8_t *bcd_tm, size_t size);

	/**
	 * Convert an ISO PVD timestamp to UNIX time.
	 * @param pvd_time PVD timestamp (16-char buffer)
	 * @param tz_offset PVD timezone offset
	 * @return UNIX time, or -1 if invalid or not set.
	 */
	static time_t pvd_time_to_unix_time(const char pvd_time[16], int8_t tz_offset);

	/** Functions for RomData subclasses that handle directories **/

	/**
	 * Is a directory supported by this class?
	 * This version checks that *all* of the specified files are found.
	 *
	 * @tparam CharType Character type (char for UTF-8; wchar_t for Windows UTF-16)
	 * @param path Directory to check
	 * @param filenames_to_check Array of filenames to check
	 * @param size Size of filenames_to_check
	 * @return True if all of the files are found; false if any files are missing.
	 */
	template<typename CharType>
	static bool T_isDirSupported_allFiles_static(const CharType *path, const CharType *const *filenames_to_check, size_t size);

	/**
	 * Is a directory supported by this class?
	 * This version checks that *all* of the specified files are found.
	 *
	 * @tparam CharType Character type (char for UTF-8; wchar_t for Windows UTF-16)
	 * @tparam size std::array<> size
	 * @param path Directory to check
	 * @param filenames_to_check Array of filenames to check
	 * @return True if all of the files are found; false if any files are missing.
	 */
	template<typename CharType, size_t size>
	static bool T_isDirSupported_allFiles_static(const CharType *path, const std::array<const CharType*, size> &filenames_to_check)
	{
		return T_isDirSupported_allFiles_static(path, filenames_to_check.data(), filenames_to_check.size());
	}

	/**
	 * Is a directory supported by this class?
	 * This version checks that *any* of the specified files are found.
	 *
	 * @tparam CharType Character type (char for UTF-8; wchar_t for Windows UTF-16)
	 * @param path Directory to check
	 * @param filenames_to_check Array of filenames to check
	 * @param size Size of filenames_to_check
	 * @return True if any of the files are found; false if all files are missing.
	 */
	template<typename CharType>
	static bool T_isDirSupported_anyFile_static(const CharType *path, const CharType *const *filenames_to_check, size_t size);

	/**
	 * Is a directory supported by this class?
	 * This version checks that *any* of the specified files are found.
	 *
	 * @tparam CharType Character type (char for UTF-8; wchar_t for Windows UTF-16)
	 * @tparam size std::array<> size
	 * @param path Directory to check
	 * @param filenames_to_check Array of filenames to check
	 * @return True if any of the files are found; false if all files are missing.
	 */
	template<typename CharType, size_t size>
	static bool T_isDirSupported_anyFile_static(const CharType *path, const std::array<const CharType*, size> &filenames_to_check)
	{
		return T_isDirSupported_anyFile_static(path, filenames_to_check.data(), filenames_to_check.size());
	}
};

}
