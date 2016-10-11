/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCubeSave.hpp: Nintendo GameCube save file reader.                   *
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

#include "GameCubeSave.hpp"
#include "NintendoPublishers.hpp"
#include "gcn_card.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

class GameCubeSavePrivate
{
	public:
		GameCubeSavePrivate();

	private:
		GameCubeSavePrivate(const GameCubeSavePrivate &other);
		GameCubeSavePrivate &operator=(const GameCubeSavePrivate &other);

	public:
		// Date/Time. (RFT_DATETIME)
		static const RomFields::DateTimeDesc last_modified_dt;

		// Monospace string formatting.
		static const RomFields::StringDesc gcn_save_string_monospace;

		// RomFields data.
		static const struct RomFields::Desc gcn_save_fields[];

		// Directory entry from the GCI header.
		card_direntry direntry;

		// TODO: load image function.
};

/** GameCubeSavePrivate **/

GameCubeSavePrivate::GameCubeSavePrivate()
{
	// Clear the directory entry.
	memset(&direntry, 0, sizeof(direntry));
}

// Last Modified timestamp.
const RomFields::DateTimeDesc GameCubeSavePrivate::last_modified_dt = {
	RomFields::RFT_DATETIME_HAS_DATE |
	RomFields::RFT_DATETIME_HAS_TIME |
	RomFields::RFT_DATETIME_IS_UTC	// GameCube doesn't support timezones.
};

// Monospace string formatting.
const RomFields::StringDesc GameCubeSavePrivate::gcn_save_string_monospace = {
	RomFields::StringDesc::STRF_MONOSPACE
};

// Save file fields.
const struct RomFields::Desc GameCubeSavePrivate::gcn_save_fields[] = {
	// TODO: Banner?
	{_RP("Game ID"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Publisher"), RomFields::RFT_STRING, {nullptr}},
	{_RP("File Name"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Description"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Last Modified"), RomFields::RFT_DATETIME, {&last_modified_dt}},
	{_RP("Mode"), RomFields::RFT_STRING, {&gcn_save_string_monospace}},
	{_RP("Copy Count"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Blocks"), RomFields::RFT_STRING, {nullptr}},
};

/** GameCubeSave **/

/**
 * Read a Nintendo GameCube save file.
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
GameCubeSave::GameCubeSave(IRpFile *file)
	: RomData(file, GameCubeSavePrivate::gcn_save_fields, ARRAY_SIZE(GameCubeSavePrivate::gcn_save_fields))
	, d(new GameCubeSavePrivate())
{
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the save file header.
	uint8_t header[1024];
	m_file->rewind();
	size_t size = m_file->read(&header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this disc image is supported.
	DetectInfo info;
	info.pHeader = header;
	info.szHeader = sizeof(header);
	info.ext = nullptr;	// TODO: GCI, GCS, SAV, etc?
	info.szFile = m_file->fileSize();
	m_isValid = (isRomSupported(&info) >= 0);

	if (m_isValid) {
		// Save the directory entry for later.
		// TODO: GCI only for now; add other types later.
		memcpy(&d->direntry, header, sizeof(d->direntry));
	}
}

GameCubeSave::~GameCubeSave()
{
	delete d;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameCubeSave::isRomSupported_static(const DetectInfo *info)
{
	if (!info || info->szHeader < sizeof(card_direntry)) {
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// GCI files must be a multiple of 8 KB, plus 64 bytes.
	if (((info->szFile - 64) % 8192) != 0) {
		// Incorrect file size.
		return -1;
	}

	// Data length, minus the GCI header.
	const int64_t data_len = info->szFile - 64;

	const card_direntry *direntry = reinterpret_cast<const card_direntry*>(info->pHeader);

	// Game ID should be alphanumeric.
	// TODO: NDDEMO has a NULL in the game ID, but I don't think
	// it has save files.
	for (int i = 6-1; i >= 0; i--) {
		if (!isalnum(direntry->id6[i])) {
			// Non-alphanumeric character.
			return -1;
		}
	}

	// Padding should be 0xFF.
	if (direntry->pad_00 != 0xFF || be16_to_cpu(direntry->pad_01) != 0xFFFF) {
		// Incorrect padding.
		return -1;
	}

	// Verify the block count.
	const unsigned int length = be16_to_cpu(direntry->length);
	if ((int64_t)(length * 8192) != data_len) {
		// Incorrect block count.
		return -1;
	}

	// Comment and icon addresses should both be less than the file size,
	// minus 64 bytes for the GCI header.
	const uint32_t commentaddr = be32_to_cpu(direntry->commentaddr);
	const uint32_t iconaddr = be32_to_cpu(direntry->iconaddr);
	if (commentaddr >= data_len || iconaddr >= data_len) {
		// Comment and/or icon are out of bounds.
		return -1;
	}

	// This is a GCI file.
	return 0;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameCubeSave::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *GameCubeSave::systemName(uint32_t type) const
{
	if (!m_isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	// Bits 2-3: DISC_SYSTEM_MASK (GCN, Wii, Triforce)
	static const rp_char *const sysNames[4] = {
		// FIXME: "NGC" in Japan?
		_RP("Nintendo GameCube"), _RP("GameCube"), _RP("GCN"), nullptr
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
vector<const rp_char*> GameCubeSave::supportedFileExtensions_static(void)
{
	// TODO: Add other save types.
	vector<const rp_char*> ret;
	ret.reserve(1);
	ret.push_back(_RP(".gci"));
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
vector<const rp_char*> GameCubeSave::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t GameCubeSave::supportedImageTypes_static(void)
{
	// TODO
	//return IMGBF_INT_ICON | IMGBF_INT_BANNER;
	return 0;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t GameCubeSave::supportedImageTypes(void) const
{
	return supportedImageTypes_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GameCubeSave::loadFieldData(void)
{
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!m_file || !m_file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid /*|| d->saveType < 0*/) {
		// Unknown save file type.
		return -EIO;
	}

	// Save file header is read in the constructor.

	// Game ID.
	// Replace any non-printable characters with underscores.
	// (NDDEMO has ID6 "00\0E01".)
	char id6[7];
	for (int i = 0; i < 6; i++) {
		id6[i] = (isprint(d->direntry.id6[i])
			? d->direntry.id6[i]
			: '_');
	}
	id6[6] = 0;
	m_fields->addData_string(latin1_to_rp_string(id6, 6));

	// Look up the publisher.
	const rp_char *publisher = NintendoPublishers::lookup(d->direntry.company);
	m_fields->addData_string(publisher ? publisher : _RP("Unknown"));

	// Filename.
	// TODO: Remove trailing nulls and/or spaces.
	// (Implicit length version of cp1252_sjis_to_rp_string()?)
	m_fields->addData_string(cp1252_sjis_to_rp_string(
		d->direntry.filename, sizeof(d->direntry.filename)));

	// Description.
	// TODO: Handle formats other than GCI.
	char desc_buf[64];
	m_file->seek(64 + be32_to_cpu(d->direntry.commentaddr));
	size_t size = m_file->read(desc_buf, sizeof(desc_buf));
	if (size != sizeof(desc_buf)) {
		// Error reading the description.
		m_fields->addData_invalid();
	} else {
		// Add the description.
		// NOTE: Two-line field; may not work right on Windows...
		rp_string desc = cp1252_sjis_to_rp_string(desc_buf, 32);
		desc += _RP_CHR('\n');
		desc += cp1252_sjis_to_rp_string(&desc_buf[32], 32);
		m_fields->addData_string(desc);
	}

	// Last Modified timestamp.
	m_fields->addData_dateTime((int64_t)be32_to_cpu(d->direntry.lastmodified) + GC_UNIX_TIME_DIFF);

	// File mode.
	rp_char file_mode[5];
	file_mode[0] = ((d->direntry.permission & CARD_ATTRIB_GLOBAL) ? _RP_CHR('G') : _RP_CHR('-'));
	file_mode[1] = ((d->direntry.permission & CARD_ATTRIB_NOMOVE) ? _RP_CHR('M') : _RP_CHR('-'));
	file_mode[2] = ((d->direntry.permission & CARD_ATTRIB_NOCOPY) ? _RP_CHR('C') : _RP_CHR('-'));
	file_mode[3] = ((d->direntry.permission & CARD_ATTRIB_PUBLIC) ? _RP_CHR('P') : _RP_CHR('-'));
	file_mode[4] = 0;
	m_fields->addData_string(file_mode);

	// Copy count.
	m_fields->addData_string_numeric(d->direntry.copytimes);
	// Blocks.
	m_fields->addData_string_numeric(be16_to_cpu(d->direntry.length));

	// Finished reading the field data.
	return (int)m_fields->count();
}

/**
 * Load an internal image.
 * Called by RomData::image() if the image data hasn't been loaded yet.
 * @param imageType Image type to load.
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCubeSave::loadInternalImage(ImageType imageType)
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
		// ROM image isn't valid.
		return -EIO;
	}

	// TODO
	return -ENOSYS;
}

}
