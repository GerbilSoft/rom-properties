/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GBS.hpp: GBS audio reader.                                              *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GBS.hpp"
#include "gbs_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;
using LibRpFile::IRpFile;

// C++ STL classes
using std::string;
using std::vector;

namespace LibRomData {

class GBSPrivate : public RomDataPrivate
{
	public:
		GBSPrivate(GBS *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(GBSPrivate)

	public:
		// Audio format.
		enum class AudioFormat {
			Unknown = -1,

			GBS = 0,
			GBR = 1,

			Max
		};
		AudioFormat audioFormat;

		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// GBS header.
		// NOTE: **NOT** byteswapped in memory.
		union {
			GBS_Header gbs;
			GBR_Header gbr;
		} header;
};

ROMDATA_IMPL(GBS)

/** GBSPrivate **/

/* RomDataInfo */
const char *const GBSPrivate::exts[] = {
	".gbs",
	".gbr",

	nullptr
};
const char *const GBSPrivate::mimeTypes[] = {
	// NOTE: Ordering matches AudioFormat.

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"audio/x-gbs",
	"audio/x-gbr",

	nullptr
};
const RomDataInfo GBSPrivate::romDataInfo = {
	"GBS", exts, mimeTypes
};

GBSPrivate::GBSPrivate(GBS *q, IRpFile *file)
	: super(q, file, &romDataInfo)
	, audioFormat(AudioFormat::Unknown)
{
	// Clear the header struct.
	memset(&header, 0, sizeof(header));
}

/** GBS **/

/**
 * Read a GBS audio file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
GBS::GBS(IRpFile *file)
	: super(new GBSPrivate(this, file))
{
	RP_D(GBS);
	d->fileType = FileType::AudioFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the GBS header.
	d->file->rewind();
	size_t size = d->file->read(&d->header, sizeof(d->header));
	if (size != sizeof(d->header)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(d->header), reinterpret_cast<const uint8_t*>(&d->header)},
		nullptr,	// ext (not needed for GBS)
		0		// szFile (not needed for GBS)
	};
	d->audioFormat = static_cast<GBSPrivate::AudioFormat>(isRomSupported_static(&info));

	if ((int)d->audioFormat < 0) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	} else if ((int)d->audioFormat < ARRAY_SIZE_I(d->mimeTypes)-1) {
		d->mimeType = d->mimeTypes[(int)d->audioFormat];
		d->isValid = true;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GBS::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(GBS_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(GBSPrivate::AudioFormat::Unknown);
	}

	const GBS_Header *const gbsHeader =
		reinterpret_cast<const GBS_Header*>(info->header.pData);

	// Check the GBS magic number.
	if (gbsHeader->magic == cpu_to_be32(GBS_MAGIC)) {
		// Found the GBS magic number.
		return static_cast<int>(GBSPrivate::AudioFormat::GBS);
	} else if (gbsHeader->magic == cpu_to_be32(GBR_MAGIC)) {
		// Found the GBR magic number.
		return static_cast<int>(GBSPrivate::AudioFormat::GBR);
	}

	// Not suported.
	return static_cast<int>(GBSPrivate::AudioFormat::Unknown);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *GBS::systemName(unsigned int type) const
{
	RP_D(const GBS);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GBS has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"GBS::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	// Bit 2: GBS or GBR.
	static const char *const sysNames[2][4] = {
		{"Game Boy Sound System", "GBS", "GBS", nullptr},
		{"Game Boy Ripped", "GBR", "GBR", nullptr},
	};

	return sysNames[((int)d->audioFormat) & 1][type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GBS::loadFieldData(void)
{
	RP_D(GBS);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// TODO: Does GBR have titles?
	switch (d->audioFormat) {
		default:
			assert(!"GBS: Invalid audio format.");
			break;

		case GBSPrivate::AudioFormat::GBS: {
			// GBS header.
			const GBS_Header *const gbs = &d->header.gbs;

			d->fields.reserve(9);	// Maximum of 9 fields.
			d->fields.setTabName(0, "GBS");

			// NOTE: The GBS specification says ASCII, but I'm assuming
			// the text is cp1252 and/or Shift-JIS.

			// Title
			if (gbs->title[0] != 0) {
				d->fields.addField_string(C_("RomData|Audio", "Title"),
					cp1252_sjis_to_utf8(gbs->title, sizeof(gbs->title)));
			}

			// Composer
			if (gbs->composer[0] != 0) {
				d->fields.addField_string(C_("RomData|Audio", "Composer"),
					cp1252_sjis_to_utf8(gbs->composer, sizeof(gbs->composer)));
			}

			// Copyright
			if (gbs->copyright[0] != 0) {
				d->fields.addField_string(C_("RomData|Audio", "Copyright"),
					cp1252_sjis_to_utf8(gbs->copyright, sizeof(gbs->copyright)));
			}

			// Number of tracks
			d->fields.addField_string_numeric(C_("RomData|Audio", "Track Count"),
				gbs->track_count);

			// Default track number
			d->fields.addField_string_numeric(C_("RomData|Audio", "Default Track #"),
				gbs->default_track);

			// Load address
			d->fields.addField_string_numeric(C_("GBS", "Load Address"),
				le16_to_cpu(gbs->load_address),
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

			// Init address
			d->fields.addField_string_numeric(C_("GBS", "Init Address"),
				le16_to_cpu(gbs->init_address),
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

			// Play address
			d->fields.addField_string_numeric(C_("GBS", "Play Address"),
				le16_to_cpu(gbs->play_address),
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

			// Play address
			d->fields.addField_string_numeric(C_("GBS", "Stack Pointer"),
				le16_to_cpu(gbs->stack_pointer),
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);
			break;
		}

		case GBSPrivate::AudioFormat::GBR: {
			// GBR header.
			// TODO: Does GBR support text fields?
			const GBR_Header *const gbr = &d->header.gbr;

			d->fields.reserve(3);	// Maximum of 3 fields.
			d->fields.setTabName(0, "GBR");

			// Init address
			d->fields.addField_string_numeric(C_("GBS", "Init Address"),
				le16_to_cpu(gbr->init_address),
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

			// VSync address
			d->fields.addField_string_numeric(C_("GBS", "VSync Address"),
				le16_to_cpu(gbr->vsync_address),
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

			// Timer address
			d->fields.addField_string_numeric(C_("GBS", "Timer Address"),
				le16_to_cpu(gbr->timer_address),
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);
			break;
		}
	}

	// TODO: Timer modulo and control?

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int GBS::loadMetaData(void)
{
	RP_D(GBS);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// NOTE: Metadata isn't currently supported for GBR.
	if (d->audioFormat != GBSPrivate::AudioFormat::GBS) {
		return -ENOENT;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(3);	// Maximum of 3 metadata properties.

	// GBS header.
	const GBS_Header *const gbs = &d->header.gbs;

	// Title.
	if (gbs->title[0] != 0) {
		d->metaData->addMetaData_string(Property::Title,
			cp1252_sjis_to_utf8(gbs->title, sizeof(gbs->title)));
	}

	// Composer.
	if (gbs->composer[0] != 0) {
		d->metaData->addMetaData_string(Property::Composer,
			cp1252_sjis_to_utf8(gbs->composer, sizeof(gbs->composer)));
	}

	// Copyright.
	if (gbs->copyright[0] != 0) {
		d->metaData->addMetaData_string(Property::Copyright,
			cp1252_sjis_to_utf8(gbs->copyright, sizeof(gbs->copyright)));
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

}
