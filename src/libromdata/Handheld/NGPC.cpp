/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NGPC.cpp: Neo Geo Pocket (Color) ROM reader.                            *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "NGPC.hpp"
#include "ngpc_structs.h"

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;

// C++ STL classes.
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(NGPC)

class NGPCPrivate : public RomDataPrivate
{
	public:
		NGPCPrivate(NGPC *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(NGPCPrivate)

	public:
		/** RomFields **/

		// ROM type.
		enum NGPC_RomType {
			ROM_UNKNOWN	= -1,	// Unknown ROM type.
			ROM_NGP		= 0,	// Neo Geo Pocket
			ROM_NGPC	= 1,	// Neo Geo Pocket Color

			ROM_MAX
		};
		int romType;

		// MIME type table.
		// Ordering matches NGPC_RomType.
		static const char *const mimeType_tbl[];

	public:
		// ROM header.
		NGPC_RomHeader romHeader;
};

/** NGPCPrivate **/

// MIME type table.
// Ordering matches NGPC_RomType.
const char *const NGPCPrivate::mimeType_tbl[] = {
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-neo-geo-pocket-rom",
	"application/x-neo-geo-pocket-color-rom",

	nullptr
};

NGPCPrivate::NGPCPrivate(NGPC *q, IRpFile *file)
	: super(q, file)
	, romType(ROM_UNKNOWN)
{
	// Clear the various structs.
	memset(&romHeader, 0, sizeof(romHeader));
}

/** NGPC **/

/**
 * Read a Neo Geo Pocket (Color) ROM.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM file.
 */
NGPC::NGPC(IRpFile *file)
	: super(new NGPCPrivate(this, file))
{
	RP_D(NGPC);
	d->className = "NGPC";

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Seek to the beginning of the header.
	d->file->rewind();

	// Read the ROM header.
	d->file->rewind();
	size_t size = d->file->read(&d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this ROM is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->romHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->romHeader);
	info.ext = nullptr;	// Not needed for NGPC.
	info.szFile = 0;	// Not needed for NGPC.
	d->romType = isRomSupported_static(&info);
	d->isValid = (d->romType >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
	}

	// Set the MIME type.
	if (d->romType < ARRAY_SIZE(d->mimeType_tbl)-1) {
		d->mimeType = d->mimeType_tbl[d->romType];
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int NGPC::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(NGPC_RomHeader))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check the copyright/license string.
	const NGPC_RomHeader *const romHeader =
		reinterpret_cast<const NGPC_RomHeader*>(info->header.pData);
	if (!memcmp(romHeader->copyright, NGPC_COPYRIGHT_STR, sizeof(romHeader->copyright)) ||
	    !memcmp(romHeader->copyright, NGPC_LICENSED_STR,  sizeof(romHeader->copyright)))
	{
		// Valid copyright/license string.
		// Check the machine type.
		switch (romHeader->machine_type) {
			case NGPC_MACHINETYPE_MONOCHROME:
				return NGPCPrivate::ROM_NGP;
			case NGPC_MACHINETYPE_COLOR:
				return NGPCPrivate::ROM_NGPC;
			default:
				// Invalid.
				break;
		}
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *NGPC::systemName(unsigned int type) const
{
	RP_D(const NGPC);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// NGPC has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"NGPC::systemName() array index optimization needs to be updated.");
	static_assert(NGPCPrivate::ROM_MAX == 2,
		"NGPC::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	// Bit 2: Machine type. (0 == NGP, 1 == NGPC)
	static const char *const sysNames[2][4] = {
		{"Neo Geo Pocket", "NGP", "NGP", nullptr},
		{"Neo Geo Pocket Color", "NGPC", "NGPC", nullptr}
	};

	// NOTE: This might return an incorrect system name if
	// d->romType is ROM_TYPE_UNKNOWN.
	return sysNames[d->romType & 1][type & SYSNAME_TYPE_MASK];
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
const char *const *NGPC::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".ngp",  ".ngc", ".ngpc",

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
const char *const *NGPC::supportedMimeTypes_static(void)
{
	return NGPCPrivate::mimeType_tbl;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int NGPC::loadFieldData(void)
{
	RP_D(NGPC);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->romType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// NGPC ROM header
	const NGPC_RomHeader *const romHeader = &d->romHeader;
	d->fields->reserve(6);	// Maximum of 6 fields.

	// Title
	// NOTE: It's listed as ASCII. We'll use Latin-1.
	d->fields->addField_string(C_("RomData", "Title"),
		latin1_to_utf8(romHeader->title, sizeof(romHeader->title)),
		RomFields::STRF_TRIM_END);

	// Product ID
	d->fields->addField_string(C_("RomData", "Product ID"),
		rp_sprintf("NEOP%02X%02X", romHeader->id_code[1], romHeader->id_code[0]));

	// Revision
	d->fields->addField_string_numeric(C_("RomData", "Revision"),
		romHeader->version, RomFields::FB_DEC, 2);

	// System
	static const char *const system_bitfield_names[] = {
		"NGP (Monochrome)", "NGP Color"
	};
	vector<string> *const v_system_bitfield_names = RomFields::strArrayToVector(
		system_bitfield_names, ARRAY_SIZE(system_bitfield_names));
	d->fields->addField_bitfield(C_("NGPC", "System"),
		v_system_bitfield_names, 0,
			(d->romType == NGPCPrivate::ROM_NGPC ? 3 : 1));

	// Entry point
	const uint32_t entry_point = le32_to_cpu(romHeader->entry_point);
	d->fields->addField_string_numeric(C_("NGPC", "Entry Point"),
		entry_point, RomFields::FB_HEX, 8, RomFields::STRF_MONOSPACE);

	// Debug enabled?
	const char *s_debug = nullptr;
	switch (entry_point >> 24) {
		case NGPC_DEBUG_MODE_OFF:
			s_debug = C_("NGPC|DebugMode", "Off");
			break;
		case NGPC_DEBUG_MODE_ON:
			s_debug = C_("NGPC|DebugMode", "On");
			break;
		default:
			break;
	}
	if (s_debug) {
		d->fields->addField_string(C_("NGPC", "Debug Mode"), s_debug);
	} else {
		d->fields->addField_string(C_("NGPC", "Debug Mode"),
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"), entry_point >> 24));
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int NGPC::loadMetaData(void)
{
	RP_D(NGPC);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->romType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// NGPC ROM header
	const NGPC_RomHeader *const romHeader = &d->romHeader;

	// Title
	// NOTE: It's listed as ASCII. We'll use Latin-1.
	d->metaData->addMetaData_string(Property::Title,
		latin1_to_utf8(romHeader->title, sizeof(romHeader->title)),
		RomMetaData::STRF_TRIM_END);

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

}
