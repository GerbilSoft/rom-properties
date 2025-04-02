/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Wim.cpp: Microsoft WIM header reader                                    *
 *                                                                         *
 * Copyright (c) 2023 by ecumber.                                          *
 * Copyright (c) 2019-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "Wim.hpp"
#include "Wim_p.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;

// C includes
#include <uchar.h>

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(Wim)

const array<const char*, 3+1> WimPrivate::exts = {{
	".wim",
	".esd",
	".swm",
	// TODO: More?

	nullptr
}};
const array<const char*, 1+1> WimPrivate::mimeTypes = {{
	// Unofficial MIME types.
	"application/x-ms-wim",

	nullptr
}};
const RomDataInfo WimPrivate::romDataInfo = {
	"WIM", exts.data(), mimeTypes.data()
};

WimPrivate::WimPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, versionType(Wim_Unknown)
{ 
	// Clear the WIM header struct.
	memset(&wimHeader, 0, sizeof(wimHeader));
}

/**
 * Read a WIM image.
 *
 * A WIM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the WIM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid WIM image.
 *
 * @param file Open WIM image.
 */
Wim::Wim(const IRpFilePtr &file)
	: super(new WimPrivate(file))
{
	RP_D(Wim);
	d->mimeType = "application/x-ms-wim";
	d->fileType = FileType::DiskImage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Seek to the beginning of the header.
	d->file->rewind();

	// Read the Wim header.
	size_t size = d->file->read(&d->wimHeader, sizeof(d->wimHeader));
	if (size != sizeof(d->wimHeader)) {
		d->file.reset();
		return;
	}

	// Check if this Wim is supported.
	const DetectInfo info = {
		{0, sizeof(d->wimHeader), reinterpret_cast<const uint8_t*>(&d->wimHeader)},
		nullptr,	// ext (not needed for Wim)
		0		// szFile (not needed for Wim)
	};
	d->versionType = static_cast<WIM_Version_Type>(isRomSupported_static(&info));

	d->isValid = (static_cast<int>(d->versionType) >= 0);
	if (!d->isValid) {
		d->file.reset();
		return;
	}

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	// Byteswap the WIM header.
	d->wimHeader.header_size	= le32_to_cpu(d->wimHeader.header_size);
	d->wimHeader.flags		= le32_to_cpu(d->wimHeader.flags);
	d->wimHeader.chunk_size		= le32_to_cpu(d->wimHeader.chunk_size);
	d->wimHeader.part_number	= le16_to_cpu(d->wimHeader.part_number);
	d->wimHeader.total_parts	= le16_to_cpu(d->wimHeader.total_parts);
	d->wimHeader.number_of_images	= le32_to_cpu(d->wimHeader.number_of_images);
	d->wimHeader.bootable_index	= le32_to_cpu(d->wimHeader.bootable_index);

	// Byteswap WIM_File_Resource objects.
	auto do_wfr_byteswap = [](WIM_File_Resource &wfr) {
		wfr.size          = le64_to_cpu(wfr.size);
		wfr.offset_of_xml = le64_to_cpu(wfr.offset_of_xml);
		//wfr.not_important = le64_to_cpu(wfr.not_important);
	};
	do_wfr_byteswap(d->wimHeader.offset_table);
	do_wfr_byteswap(d->wimHeader.xml_resource);
	do_wfr_byteswap(d->wimHeader.boot_metadata_resource);
	do_wfr_byteswap(d->wimHeader.integrity_resource);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

	if (d->versionType == WIM_Version_Type::Wim107_108) {
		// the version offset is different so we have to get creative
		d->wimHeader.version.major_version = d->wimHeader.magic[6];
		d->wimHeader.version.minor_version = d->wimHeader.magic[5];
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Wim::isRomSupported_static(const DetectInfo* info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	if (!info || !info->header.pData) {
		// No detection information.
		return -1;
	}

	const WIM_Header *const wimData = reinterpret_cast<const WIM_Header*>(info->header.pData);
	int ret = Wim_Unknown;

	// TODO: WLPWM_MAGIC
	if (!memcmp(wimData->magic, MSWIM_MAGIC, 8)) {
		// at least a wim 1.09, check the version
		// we do not necessarily need to check the
		// major version because it is always
		// either 1 or 0 (in the case of esds)
		if (wimData->version.minor_version >= 13) {
			ret = Wim113_014;
		} else {
			ret = Wim109_112;
		}
	} else if (!memcmp(wimData->magic, MSWIMOLD_MAGIC, 4)) {
		// NOTE: This magic number is too generic.
		// Verify the file extension.
		if (info->ext && !strcasecmp(info->ext, ".wim")) {
			ret = Wim107_108;
		}
	}

	return ret;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char* Wim::systemName(unsigned int type) const
{
	RP_D(const Wim);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	static_assert(SYSNAME_TYPE_MASK == 3,
		"Wim::systemName() array index optimization needs to be updated.");

	static const array<const char*, 4> sysNames = {{
		"Microsoft WIM", "WIM Image", "WIM", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Wim::loadFieldData(void)
{
	RP_D(Wim);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	d->fields.reserve(6);	// Maximum of 6 fields. (5 if XML is disabled)

	// If the version number is 14, add an indicator that it is an ESD.
	d->fields.addField_string(C_("Wim", "WIM Version"),
		fmt::format(FSTR("{:d}.{:0>2d}{:s}"),
			d->wimHeader.version.major_version, d->wimHeader.version.minor_version,
			(d->wimHeader.version.minor_version == 14) ? " (ESD)" : ""),
		RomFields::STRF_TRIM_END);

	if (d->versionType != Wim113_014) {
		// The rest of the fields require Wim113_014 or later.
		return 0;
	}

	static const array<const char*, 7> wim_flag_names = {{
		nullptr,
		NOP_C_("Wim|Flags", "Compressed"),
		NOP_C_("Wim|Flags", "Read-only"),
		NOP_C_("Wim|Flags", "Spanned"),
		NOP_C_("Wim|Flags", "Resource Only"),
		NOP_C_("Wim|Flags", "Metadata Only"),
		NOP_C_("Wim|Flags", "Write in progress"),
	}};

	const uint32_t wimflags = d->wimHeader.flags;

	vector<string> *const v_wim_flag_names = RomFields::strArrayToVector_i18n("RomData", wim_flag_names);
	d->fields.addField_bitfield(C_("RomData", "Flags"),
		v_wim_flag_names, 3, wimflags);

	// loop through each compression flag and test if it is set
	int i;
	for (i = compress_xpress; i <= compress_xpress2; i = i << 1) {
		if (wimflags & i)
			break;
	}

	const char *compression_method;
	switch (i)  {
		case compress_xpress:
			compression_method = "XPRESS";
			break;
		case compress_lzx:
			compression_method = "LZX";
			break;
		case compress_lzms:
			compression_method = "LZMS";
			break;
		case compress_xpress2:
			compression_method = "XPRESS2";
			break;
		default:
			// if it has compression but the algorithm isn't 
			// accounted for, say it's unknown
			compression_method = (wimflags & has_compression)
				? C_("RomData", "Unknown")
				: C_("RomData", "None");
			break;
	}
	d->fields.addField_string(C_("Wim", "Compression Method"), compression_method);
	d->fields.addField_string(C_("Wim", "Part Number"),
		fmt::format(FSTR("{:d}/{:d}"), d->wimHeader.part_number, d->wimHeader.total_parts));
	d->fields.addField_string_numeric(C_("Wim", "Total Images"), d->wimHeader.number_of_images);

#ifdef ENABLE_XML
	// Add fields from the WIM image's XML manifest.
	int ret = d->addFields_XML();
	if (ret != 0) {
		d->fields.addField_string(C_("RomData", "Warning"),
			C_("RomData", "XML parsing failed."),
			RomFields::STRF_WARNING);
	}
#else /* !ENABLE_XML */
	d->fields.addField_string(C_("RomData", "Warning"),
		C_("RomData", "XML parsing is disabled in this build."),
		RomFields::STRF_WARNING);
#endif /* ENABLE_XML */

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

} // namespace LibRomData
