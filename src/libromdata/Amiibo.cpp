/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Amiibo.cpp: Nintendo amiibo NFC dump reader.                            *
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

#include "Amiibo.hpp"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>
#include <cstddef>

// C++ includes.
#include <vector>
using std::vector;

namespace LibRomData {

class AmiiboPrivate
{
	public:
		AmiiboPrivate() { }

	private:
		AmiiboPrivate(const AmiiboPrivate &other);
		AmiiboPrivate &operator=(const AmiiboPrivate &other);

	public:
		/** RomFields **/

		// Credits string formatting.
		static const RomFields::StringDesc nfp_string_credits;

		// ROM fields.
		static const struct RomFields::Desc nfp_fields[];

	public:
		// NFC data. (TODO)
		//NFP_Data nfpData;
};

/** AmiiboPrivate **/

// Credits string formatting.
const RomFields::StringDesc AmiiboPrivate::nfp_string_credits = {
	RomFields::StringDesc::STRF_CREDITS
};

// ROM fields.
const struct RomFields::Desc AmiiboPrivate::nfp_fields[] = {
	// TODO: amiibo data.

	// Credits
	{_RP("Credits"), RomFields::RFT_STRING, {&nfp_string_credits}},
};

/** Amiibo **/

/**
 * Read a Nintendo amiibo NFC dump.
 *
 * An NFC dump must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the NFC dump.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open NFC dump.
 */
Amiibo::Amiibo(IRpFile *file)
	: super(file, AmiiboPrivate::nfp_fields, ARRAY_SIZE(AmiiboPrivate::nfp_fields))
	, d(new AmiiboPrivate())
{
	// This class handles NFC dumps.
	m_fileType = FTYPE_NFC_DUMP;

	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// TODO: Verify the NFP data.
	m_isValid = true;
#if 0
	// Read the ROM header.
	GBA_RomHeader romHeader;
	m_file->rewind();
	size_t size = m_file->read(&romHeader, sizeof(romHeader));
	if (size != sizeof(romHeader))
		return;

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(romHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&romHeader);
	info.ext = nullptr;	// Not needed for GBA.
	info.szFile = 0;	// Not needed for GBA.
	m_isValid = (isRomSupported_static(&info) >= 0);

	if (m_isValid) {
		// Save the ROM header.
		memcpy(&d->romHeader, &romHeader, sizeof(d->romHeader));
	}
#endif
}

Amiibo::~Amiibo()
{
	delete d;
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Amiibo::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 540 ||
	    info->szFile != 540)
	{
		// Either no detection information was specified,
		// the header is too small, or the file is the
		// wrong size.
		return -1;
	}

	// TODO: Verify the file.
	return 0;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Amiibo::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *Amiibo::systemName(uint32_t type) const
{
	if (!m_isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// The "correct" name is "Nintendo Figurine Platform".
	// It's unknown whether or not Nintendo will release
	// NFC-enabled figurines that aren't amiibo.

	// NFP has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Amiibo::systemName() array index optimization needs to be updated.");

	static const rp_char *const sysNames[4] = {
		_RP("Nintendo Figurine Platform"),
		_RP("Nintendo Figurine Platform"),
		_RP("NFP"),
		nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> Amiibo::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		//_RP(".bin"),	// TODO: Enable this?
		// NOTE: The following extensions are listed
		// for testing purposes on Windows, and may
		// be removed later.
		_RP(".nfc"),
		_RP(".nfp"),
	};
	return vector<const rp_char*>(exts, exts + ARRAY_SIZE(exts));
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> Amiibo::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Amiibo::loadFieldData(void)
{
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!m_file) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// TODO: Actual NFC data.

	// Credits.
	m_fields->addData_string(
		_RP("amiibo images provided by <a href=\"http://amiibo.life/\">amiibo.life</a>,\n the Unofficial amiibo Database."));

	// Finished reading the field data.
	return (int)m_fields->count();
}

}
