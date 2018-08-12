/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PSF.hpp: PSF audio reader.                                              *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
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

#include "PSF.hpp"
#include "librpbase/RomData_p.hpp"

#include "psf_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <string>
#include <sstream>
#include <vector>
using std::ostringstream;
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(PSF)

class PSFPrivate : public RomDataPrivate
{
	public:
		PSFPrivate(PSF *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(PSFPrivate)

	public:
		// PSF header.
		// NOTE: **NOT** byteswapped in memory.
		PSF_Header psfHeader;
};

/** PSFPrivate **/

PSFPrivate::PSFPrivate(PSF *q, IRpFile *file)
	: super(q, file)
{
	// Clear the PSF header struct.
	memset(&psfHeader, 0, sizeof(psfHeader));
}

/** PSF **/

/**
 * Read an PSF audio file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
PSF::PSF(IRpFile *file)
	: super(new PSFPrivate(this, file))
{
	RP_D(PSF);
	d->className = "PSF";
	d->fileType = FTYPE_AUDIO_FILE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the PSF header.
	d->file->rewind();
	size_t size = d->file->read(&d->psfHeader, sizeof(d->psfHeader));
	if (size != sizeof(d->psfHeader)) {
		delete d->file;
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->psfHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->psfHeader);
	info.ext = nullptr;	// Not needed for PSF.
	info.szFile = 0;	// Not needed for PSF.
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		delete d->file;
		d->file = nullptr;
		return;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int PSF::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(PSF_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const PSF_Header *const psfHeader =
		reinterpret_cast<const PSF_Header*>(info->header.pData);

	// Check the PSF magic number.
	if (memcmp(psfHeader->magic, PSF_MAGIC, sizeof(psfHeader->magic)) != 0) {
		// Not the PSF magic number.
		return -1;
	}

	// This is an PSF file.
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *PSF::systemName(unsigned int type) const
{
	RP_D(const PSF);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// PSF has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"PSF::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Portable Sound Format",
		"PSF",
		"PSF",
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
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *PSF::supportedFileExtensions_static(void)
{
	// NOTE: The .*lib files are not listed, since they
	// contain samples, not songs.
	static const char *const exts[] = {
		".psf", ".minipsf",
		".psf1", ".minipsf1",
		".psf2", ".minipsf2",

		".ssf", ".minissf",
		".dsf", ".minidsf",

		".usf", ".miniusf",
		".gsf", ".minigsf",
		".snsf", ".minisnsf",

		".qsf", ".miniqsf",

		nullptr
	};
	return exts;
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
const char *const *PSF::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		"audio/x-psf",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int PSF::loadFieldData(void)
{
	RP_D(PSF);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// PSF header.
	const PSF_Header *const psfHeader = &d->psfHeader;
	d->fields->reserve(1);	// Maximum of 1 field.

	// System.
	const char *sysname;
	switch (psfHeader->version) {
		default:
			sysname = nullptr;
			break;
		case PSF_VERSION_PLAYSTATION:
			sysname = C_("PSF", "Sony PlayStation");
			break;
		case PSF_VERSION_PLAYSTATION_2:
			sysname = C_("PSF", "Sony PlayStation 2");
			break;
		case PSF_VERSION_SATURN:
			sysname = C_("PSF", "Sega Saturn");
			break;
		case PSF_VERSION_DREAMCAST:
			sysname = C_("PSF", "Sega Dreamcast");
			break;
		case PSF_VERSION_MEGA_DRIVE:
			sysname = C_("PSF", "Sega Mega Drive");
			break;
		case PSF_VERSION_N64:
			sysname = C_("PSF", "Nintendo 64");
			break;
		case PSF_VERSION_GBA:
			sysname = C_("PSF", "Game Boy Advance");
			break;
		case PSF_VERSION_SNES:
			sysname = C_("PSF", "Super NES");
			break;
		case PSF_VERSION_QSOUND:
			sysname = C_("PSF", "Capcom QSound");
			break;
	}

	if (sysname) {
		d->fields->addField_string(C_("PSF", "System"), sysname);
	} else {
		d->fields->addField_string(C_("PSF", "System"),
			rp_sprintf(C_("PSF", "Unknown (0x%02X)"), psfHeader->version));
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

}
