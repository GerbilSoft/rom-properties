/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ISO.hpp: ISO-9660 disc image parser.                                    *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
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

#include "ISO.hpp"
#include "librpbase/RomData_p.hpp"

#include "../iso_structs.h"

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
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(ISO)

class ISOPrivate : public LibRpBase::RomDataPrivate
{
	public:
		ISOPrivate(ISO *q, LibRpBase::IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(ISOPrivate)

	public:
		// ISO primary volume descriptor.
		ISO_Primary_Volume_Descriptor pvd;
};

/** ISOPrivate **/

ISOPrivate::ISOPrivate(ISO *q, IRpFile *file)
	: super(q, file)
{
	// Clear the disc header structs.
	memset(&pvd, 0, sizeof(pvd));
}

/** ISO **/

/**
 * Read an ISO-9660 executable.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
ISO::ISO(IRpFile *file)
	: super(new ISOPrivate(this, file))
{
	// This class handles disc images.
	RP_D(ISO);
	d->className = "ISO";
	d->fileType = FTYPE_DISC_IMAGE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// TODO: Check for 2352-byte sectors.

	// Read the PVD.
	size_t size = d->file->seekAndRead(ISO_PVD_ADDRESS, &d->pvd, sizeof(d->pvd));
	if (size != sizeof(d->pvd)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if the PVD is valid.
	// NOTE: Not using isRomSupported_static(), since this function
	// only checks the file extension.
	if (d->pvd.header.type != ISO_VDT_PRIMARY ||
	    d->pvd.header.version != ISO_VD_VERSION ||
	    memcmp(d->pvd.header.identifier, ISO_MAGIC, sizeof(d->pvd.header.identifier)) != 0)
	{
		// Not a PVD.
		d->isValid = false;
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// This is a valid PVD.
	d->isValid = true;

	// TODO: Read more descriptors.
	// TODO: Search for Xbox disc images.
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int ISO::isRomSupported_static(const DetectInfo *info)
{
	// NOTE: Only checking for supported file extensions.
	printf("ext: %s\n", info->ext);
	assert(info->ext != nullptr);
	if (!info->ext) {
		// No file extension specified...
		return -1;
	}

	const char *const *exts = supportedFileExtensions_static();
	for (; *exts != nullptr; exts++) {
		if (!strcasecmp(info->ext, *exts)) {
			// Found a match.
			return 0;
		}
	}

	// No match.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *ISO::systemName(unsigned int type) const
{
	RP_D(const ISO);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// ISO-9660 has the same name worldwide, so we can
	// ignore the region selection.
	// TODO: Identify the OS, or list that in the fields instead?
	static_assert(SYSNAME_TYPE_MASK == 3,
		"ISO::systemName() array index optimization needs to be updated.");

	// TODO: UDF, XDVDFS, HFS, others?
	static const char *const sysNames[4] = {
		"ISO-9660", "ISO", "ISO", nullptr
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
const char *const *ISO::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".iso",		// ISO
		".bin",		// BIN (2352-byte)
		".xiso",	// Xbox ISO image
		// TODO: More?

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
const char *const *ISO::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types from FreeDesktop.org..
		"application/x-iso9660-image",

		// TODO: BIN (2352), XDVDFS?

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int ISO::loadFieldData(void)
{
	RP_D(ISO);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unsupported file.
		return -EIO;
	}

	// ISO-9660 Primary Volume Descriptor.
	// TODO: Other descriptors?
	const ISO_Primary_Volume_Descriptor *const pvd = &d->pvd;
	d->fields->reserve(11);	// Maximum of 11 fields.

	// NOTE: All fields are space-padded. (0x20, ' ')
	// TODO: ascii_to_utf8()?

	// ISO-9660 PVD
	d->fields->setTabName(0, "ISO-9660 PVD");

	// System ID
	d->fields->addField_string(C_("ISO", "System ID"),
		latin1_to_utf8(pvd->sysID, sizeof(pvd->sysID)),
		RomFields::STRF_TRIM_END);
	
	// Volume ID
	d->fields->addField_string(C_("ISO", "Volume ID"),
		latin1_to_utf8(pvd->volID, sizeof(pvd->volID)),
		RomFields::STRF_TRIM_END);

	// Size of volume
	d->fields->addField_string(C_("ISO", "Volume Size"),
		formatFileSize(
			static_cast<int64_t>(pvd->volume_space_size.he) *
			static_cast<int64_t>(pvd->logical_block_size.he)));

	// TODO: Show block size?

	// Disc number
	if (pvd->volume_seq_number.he != 0 && pvd->volume_set_size.he > 1) {
		const char *const disc_number_title = C_("RomData", "Disc #");
		d->fields->addField_string(disc_number_title,
			// tr: Disc X of Y (for multi-disc games)
			rp_sprintf_p(C_("RomData|Disc", "%1$u of %2$u"),
				pvd->volume_seq_number.he,
				pvd->volume_set_size.he));
	}

	// Volume set ID
	d->fields->addField_string(C_("ISO", "Volume Set"),
		latin1_to_utf8(pvd->volume_set_id, sizeof(pvd->volume_set_id)),
		RomFields::STRF_TRIM_END);

	// Publisher
	d->fields->addField_string(C_("ISO", "Publisher"),
		latin1_to_utf8(pvd->publisher, sizeof(pvd->publisher)),
		RomFields::STRF_TRIM_END);

	// Data Preparer
	d->fields->addField_string(C_("ISO", "Data Preparer"),
		latin1_to_utf8(pvd->data_preparer, sizeof(pvd->data_preparer)),
		RomFields::STRF_TRIM_END);

	// Application
	d->fields->addField_string(C_("ISO", "Application"),
		latin1_to_utf8(pvd->application, sizeof(pvd->application)),
		RomFields::STRF_TRIM_END);

	// Copyright file
	d->fields->addField_string(C_("ISO", "Copyright File"),
		latin1_to_utf8(pvd->copyright_file, sizeof(pvd->copyright_file)),
		RomFields::STRF_TRIM_END);

	// Abstract file
	d->fields->addField_string(C_("ISO", "Abstract File"),
		latin1_to_utf8(pvd->abstract_file, sizeof(pvd->abstract_file)),
		RomFields::STRF_TRIM_END);

	// Bibliographic file
	d->fields->addField_string(C_("ISO", "Bibliographic File"),
		latin1_to_utf8(pvd->bibliographic_file, sizeof(pvd->bibliographic_file)),
		RomFields::STRF_TRIM_END);

	// TODO: Timestamps.

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

}
