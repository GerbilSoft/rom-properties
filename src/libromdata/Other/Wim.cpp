/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Wim.cpp: Microsoft WIM header reader                                    *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Wim.hpp"
#include "Wim_structs.h"

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;

// C++ STL classes.
using std::string;

namespace LibRomData {
class WimPrivate final : public RomDataPrivate
{
    public:
		WimPrivate(LibRpFile::IRpFile* file);  
	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(WimPrivate)
	public:
		/** RomDataInfo **/
		static const char* const exts[];
		static const char* const mimeTypes[];
		static const RomDataInfo romDataInfo;
    public:
		// ROM header.
		WIM_Header wimHeader;
};

static WIM_Version_Type version_type; 	// wims pre-1.13 - these are detected but won't 
                  		  			  	// be read due to the format being different

ROMDATA_IMPL(Wim)

const char* const WimPrivate::exts[] = {
	".wim",
	".esd",
	".swm",
	// TODO: More?
    nullptr
};

const char* const WimPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	"application/x-ms-wim",
	nullptr
};

const RomDataInfo WimPrivate::romDataInfo = {
	"WIM", exts, mimeTypes
};

WimPrivate::WimPrivate(LibRpFile::IRpFile* file)
    : super(file, &romDataInfo)
{ 
    // Clear the WIM header struct.
	memset(&wimHeader, 0, sizeof(wimHeader));
}

int Wim::isRomSupported_static(const DetectInfo* info)
{
    assert(info != nullptr);
	assert(info->header.pData != nullptr);
	if (!info || !info->header.pData) {
		// No detection information.
		return -1;
	}

    const WIM_Header* const wimData = reinterpret_cast<const WIM_Header*>(info->header.pData);

    if (memcmp(wimData->header, "MSWIM\0\0", 8) == 0)
    {
        // at least a wim 1.09, check the version
        // we do not necessarily need to check the
        // major version because it is always 
        // either 1 or 0 (in the case of esds)

		// this could result in faulty detection but 
		// it's unlikely to happen
        if (wimData->version.minor_version >= 13) {
            version_type = Wim113_014;
        }
        else version_type = Wim109_112;
    }
    else if (memcmp(wimData->header, "\x7e\0\0", 4) == 0) 
    {
        version_type = Wim107_108;
    }
    else 
    {
        // not a wim
        return -1;
    }
    return 0;
}

Wim::Wim(LibRpFile::IRpFile* file)
	: super(new WimPrivate(file))
{
	RP_D(Wim);
    d->mimeType = "application/x-ms-wim";

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Seek to the beginning of the header.
	d->file->rewind();

	// Read the ROM header.
	size_t size = d->file->read(&d->wimHeader, sizeof(d->wimHeader));
	if (size != sizeof(d->wimHeader)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this ROM is supported.
	const DetectInfo info = {
		{0, sizeof(d->wimHeader), reinterpret_cast<const uint8_t*>(&d->wimHeader)},
		nullptr,	// ext (not needed for Wim)
		0		// szFile (not needed for Wim)
	};
    d->isValid = (isRomSupported_static(&info) >= 0);

    if (!d->isValid) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}
}

const char* Wim::systemName(unsigned int type) const
{
	RP_D(const Wim);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	static_assert(SYSNAME_TYPE_MASK == 3,
		"Wim::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Microsoft WIM Image",
		"WIM Image",
		"WIM",
		nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

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
    d->fields.reserve(6);	// Maximum of 6 fields.
	char buffer[32];

	if (version_type == WIM_Version_Type::Wim107_108) 
	{
		// the version offset is different so we have to get creative
		d->wimHeader.version.major_version = d->wimHeader.header[6];
		d->wimHeader.version.minor_version = d->wimHeader.header[5];
	}

	snprintf(buffer, 8, "%d.%02d", d->wimHeader.version.major_version, d->wimHeader.version.minor_version);
    d->fields.addField_string(C_("RomData", "WIM Version"), buffer, RomFields::STRF_TRIM_END);

	if (version_type != Wim113_014) 
	{
		d->fields.addField_string(C_("RomData", ""), "This WIM is currently not supported because its version is too low.", RomFields::STRF_TRIM_END);
		return 0;
	}

	static const char* const wim_flag_names[] = {
		nullptr,
		NOP_C_("RomData", "Compressed"),
		NOP_C_("RomData", "Read only"),
		NOP_C_("RomData", "Spanned"),
		NOP_C_("RomData", "Resource Only"),
		NOP_C_("RomData", "Metadata Only"),
		NOP_C_("RomData", "Write in progress"),
	};

	uint32_t wimflags = le32_to_cpu(d->wimHeader.flags);

	std::vector<std::string>* const v_wim_flag_names = RomFields::strArrayToVector_i18n(
		"RomData", wim_flag_names, ARRAY_SIZE(wim_flag_names));
	d->fields.addField_bitfield(C_("RomData", "Flags"),
		v_wim_flag_names, 3, wimflags);
	int i;

	// loop through each compression flag and test if it is set
	for (i = compress_xpress; i <= compress_xpress2; i = i << 1) 
	{
		if (wimflags & i)
		{
			break;
		}
	}

	char* compression_method = nullptr;
	switch (i) 
	{
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
			if (wimflags & has_compression) {
				compression_method = "Unknown";
			}
			else {
				compression_method = "None";
			}
			break;
	}
	d->fields.addField_string(C_("RomData", "Compression Method"), compression_method, RomFields::STRF_TRIM_END);
	
	snprintf(buffer, 6, "%d/%d", d->wimHeader.part_number, d->wimHeader.total_parts);
	d->fields.addField_string(C_("RomData", "Part Number"), buffer, RomFields::STRF_TRIM_END);
	return 0;
}

}