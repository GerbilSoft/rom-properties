/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * KSS.hpp: KSS audio reader.                                              *
 *                                                                         *
 * Copyright (c) 2018-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "KSS.hpp"
#include "RomData_p.hpp"

#include "kss_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

class KSSPrivate final : public RomDataPrivate
{
public:
	explicit KSSPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
public:
	RP_DISABLE_COPY(KSSPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// KSS header
	// NOTE: **NOT** byteswapped in memory.
	KSS_Header kssHeader;
};

ROMDATA_IMPL(KSS)

/** KSSPrivate **/

/* RomDataInfo */
const array<const char*, 1+1> KSSPrivate::exts = {{
	".kss",

	nullptr
}};
const array<const char*, 1+1> KSSPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"audio/x-kss",

	nullptr
}};
const RomDataInfo KSSPrivate::romDataInfo = {
	"KSS", exts.data(), mimeTypes.data()
};

KSSPrivate::KSSPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the KSS header struct.
	memset(&kssHeader, 0, sizeof(kssHeader));
}

/** KSS **/

/**
 * Read a KSS audio file.
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
KSS::KSS(const IRpFilePtr &file)
	: super(new KSSPrivate(file))
{
	RP_D(KSS);
	d->mimeType = "audio/x-kss";	// unofficial
	d->fileType = FileType::AudioFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the KSS header.
	d->file->rewind();
	size_t size = d->file->read(&d->kssHeader, sizeof(d->kssHeader));
	if (size != sizeof(d->kssHeader)) {
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(d->kssHeader), reinterpret_cast<const uint8_t*>(&d->kssHeader)},
		nullptr,	// ext (not needed for KSS)
		0		// szFile (not needed for KSS)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int KSS::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(KSS_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const KSS_Header *const kssHeader =
		reinterpret_cast<const KSS_Header*>(info->header.pData);

	// Check the KSS magic number.
	// TODO: Does KSSX have the same header or should it be handled differently?
	if (kssHeader->magic == be32_to_cpu(KSS_MAGIC) ||
	    kssHeader->magic == be32_to_cpu(KSSX_MAGIC))
	{
		// Found the KSS magic number.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *KSS::systemName(unsigned int type) const
{
	RP_D(const KSS);
	if (!d->isValid || !isSystemNameTypeValid(type)) {
		return nullptr;
	}

	// KSS has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"KSS::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"Konami Sound System", "KSS", "KSS", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int KSS::loadFieldData(void)
{
	RP_D(KSS);
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

	// KSS header
	const KSS_Header *const kssHeader = &d->kssHeader;
	d->fields.reserve(5);	// Maximum of 5 fields.

	// Version
	// KSCC == v1.03, KSSX == v1.20 [according to `file`]
	bool isKSSX = (kssHeader->magic == be32_to_cpu(KSSX_MAGIC));
	d->fields.addField_string(C_("RomData", "Version"), (isKSSX) ? "v1.20" : "v1.03");

	// Load address
	d->fields.addField_string_numeric(C_("RomData|Audio", "Load Address"),
		le16_to_cpu(kssHeader->load_address),
		RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

	// Init address
	d->fields.addField_string_numeric(C_("RomData|Audio", "Init Address"),
		le16_to_cpu(kssHeader->init_address),
		RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

	// Play address
	d->fields.addField_string_numeric(C_("RomData|Audio", "Play Address"),
		le16_to_cpu(kssHeader->play_address),
		RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

	// Start (offset) number of bank
	// TODO: Should this be printed in decimal?
	d->fields.addField_string_numeric(C_("KSS", "Start # of Bank"),
		kssHeader->start_no_of_bank,
		RomFields::Base::Hex, 2, RomFields::STRF_MONOSPACE);

	// TODO: Number of banked data blocks.
	// TODO: Extra sound chip flags.
	// TODO: KSSX (v1.20) extensions.

	// Finished reading the field data.
	return d->fields.count();
}

} // namespace LibRomData
