/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiWAD.cpp: Nintendo Wii WAD file reader.                               *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "WiiWAD.hpp"
#include "librpbase/RomData_p.hpp"

#include "gcn_structs.h"
#include "wii_structs.h"
#include "wii_wad.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/SystemRegion.hpp"
#include "librpbase/file/IRpFile.hpp"

#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <memory>
#include <sstream>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class WiiWADPrivate : public RomDataPrivate
{
	public:
		WiiWADPrivate(WiiWAD *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(WiiWADPrivate)

	public:
		// WAD header.
		Wii_WAD_Header wadHeader;
		Wii_Content_Bin_Header contentHeader;

		/**
		 * Round a value to the next highest multiple of 64.
		 * @param value Value.
		 * @return Next highest multiple of 64.
		 */
		template<typename T>
		static inline T toNext64(T val)
		{
			return (val + (T)63) & ~((T)63);
		}
};

/** WiiWADPrivate **/

WiiWADPrivate::WiiWADPrivate(WiiWAD *q, IRpFile *file)
	: super(q, file)
{
	// Clear the various structs.
	memset(&wadHeader, 0, sizeof(wadHeader));
	memset(&contentHeader, 0, sizeof(contentHeader));
}

/** WiiWAD **/

/**
 * Read a Nintendo Wii WAD file.
 *
 * A WAD file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the WAD file.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open disc image.
 */
WiiWAD::WiiWAD(IRpFile *file)
	: super(new WiiWADPrivate(this, file))
{
	// This class handles application packages.
	RP_D(WiiWAD);
	d->className = "WiiWAD";
	d->fileType = FTYPE_APPLICATION_PACKAGE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the WAD header.
	d->file->rewind();
	size_t size = d->file->read(&d->wadHeader, sizeof(d->wadHeader));
	if (size != sizeof(d->wadHeader))
		return;

	// Check if this WAD file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->wadHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->wadHeader);
	info.ext = nullptr;	// Not needed for WiiWAD.
	info.szFile = file->size();
	d->isValid = (isRomSupported_static(&info) >= 0);

	// TODO: Decryption.
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiWAD::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(Wii_WAD_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for the correct header fields.
	const Wii_WAD_Header *wadHeader = reinterpret_cast<const Wii_WAD_Header*>(info->header.pData);
	if (wadHeader->header_size != cpu_to_be32(sizeof(*wadHeader))) {
		// WAD header size is incorrect.
		return -1;
	}

	// Check WAD type.
	if (wadHeader->type != cpu_to_be32(WII_WAD_TYPE_Is) &&
	    wadHeader->type != cpu_to_be32(WII_WAD_TYPE_ib) &&
	    wadHeader->type != cpu_to_be32(WII_WAD_TYPE_Bk))
	{
		// WAD type is incorrect.
		return -1;
	}

	// Verify the ticket size.
	// TODO: Also the TMD size.
	if (be32_to_cpu(wadHeader->ticket_size) < sizeof(RVL_Ticket)) {
		// Ticket is too small.
		return -1;
	}
	
	// Check the file size to ensure we have at least the IMET section.
	unsigned int expected_size = WiiWADPrivate::toNext64(be32_to_cpu(wadHeader->header_size)) +
				WiiWADPrivate::toNext64(be32_to_cpu(wadHeader->cert_chain_size)) +
				WiiWADPrivate::toNext64(be32_to_cpu(wadHeader->ticket_size)) +
				WiiWADPrivate::toNext64(be32_to_cpu(wadHeader->tmd_size)) +
				sizeof(Wii_Content_Bin_Header);
	if (expected_size > info->szFile) {
		// File is too small.
		return -1;
	}

	// This appears to be a Wii WAD file.
	return 0;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiWAD::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *WiiWAD::systemName(unsigned int type) const
{
	RP_D(const WiiWAD);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Wii has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"WiiWAD::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Nintendo Wii", "Wii", "Wii", nullptr
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
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *WiiWAD::supportedFileExtensions_static(void)
{
	static const char *const exts[] = { ".wad" };
	return exts;
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *WiiWAD::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int WiiWAD::loadFieldData(void)
{
	RP_D(WiiWAD);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// WAD header is read in the constructor.
	const Wii_WAD_Header *const wadHeader = &d->wadHeader;
	d->fields->reserve(10);	// Maximum of 10 fields.

	// TODO: Decrypt content.bin to get the actual data.

	// Finished reading the field data.
	return (int)d->fields->count();
}

}
