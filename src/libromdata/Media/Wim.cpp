/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Wim.cpp: Microsoft WIM header reader                                    *
 *                                                                         *
 * Copyright (c) 2023 by ecumber.                                          *
 * Copyright (c) 2019-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"
#include "libromdata/config.libromdata.h"

#include "Wim.hpp"
#include "wim_structs.h"

#ifdef ENABLE_XML
// PugiXML
#  include "pugixml.hpp"
using namespace pugi;
#endif /* ENABLE_XML */

// Other rom-properties libraries
#include "librpbase/timeconv.h"
using namespace LibRpBase;
using namespace LibRpFile;

// C includes
#include <uchar.h>

// C includes (C++ namespace)
#include <ctime>

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

class WimPrivate final : public RomDataPrivate
{
public:
	explicit WimPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(WimPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 3+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// WIM header
	WIM_Header wimHeader;

	// WIM version
	// NOTE: WIMs pre-1.13 are being detected but won't
	// be read due to the format being different.
	WIM_Version_Type versionType;

#ifdef ENABLE_XML
public:
	/**
	 * Add fields from the WIM image's XML manifest.
	 * @return 0 on success; non-zero on error.
	 */
	int addFields_XML();
#endif /* ENABLE_XML */
};

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

struct WimWindowsLanguages {
	string language;
	//string default_language;	//not used right now
};

struct WimWindowsVersion {
	unsigned int majorversion = 0;
	unsigned int minorversion = 0;
	unsigned int buildnumber = 0;
	unsigned int spbuildnumber = 0;
	unsigned int splevel = 0; // only in windows 7+, added some time around build 6608-6730
};

struct WimWindowsInfo {
	WimWindowsArchitecture arch = Wim_Arch_x86;
	string productname;
	string editionid;
	//string installationtype;	// not used right now
	//string hal;			// not used right now
	//string producttype;		// not used right now
	//string productsuite;		// not used right now
	WimWindowsLanguages languages;
	WimWindowsVersion version;
	string systemroot;
};

struct WimIndex {
	// if you have more than 2^32 indices in a wim
	// you probably have bigger issues
	uint32_t index = 0;
	uint64_t dircount = 0;
	uint64_t filecount = 0;
	uint64_t totalbytes = 0;
	//time_t creationtime = 0;	// not used right now
	time_t lastmodificationtime = 0;
	WimWindowsInfo windowsinfo;
	string name, description;
	//string flags;			// not used right now
	string dispname, dispdescription;
	bool containswindowsimage = false;
};

#ifdef ENABLE_XML

/**
 * Add fields from the WIM image's XML manifest.
 * @return 0 on success; non-zero on error.
 */
int WimPrivate::addFields_XML() 
{
	if (!file || !file->isOpen()) {
		return -EIO;
	}

	// Sanity check: Minimum of 1 image; allow up to 256 images.
	unsigned int number_of_images = wimHeader.number_of_images;
	assert(number_of_images > 0);
	assert(number_of_images <= 256);
	if (number_of_images == 0) {
		// No images...
		return -ENOENT;
	} else if (number_of_images > 256) {
		number_of_images = 256;
	}

	// the eighth byte of the "size" is used for flags so we have to AND it
	static constexpr uint64_t XML_MAX_SIZE = 16U*1024U*1024U;
	const uint64_t size64 = (wimHeader.xml_resource.size & 0x00FFFFFFFFFFFFFFULL);
	assert(size64 <= XML_MAX_SIZE);
	if (size64 > XML_MAX_SIZE) {
		// XML is larger than 16 MB, which doesn't make any sense.
		return -ENOMEM;
	}

	// XML data is UTF-16LE, so the size should be a multiple of 2.
	// TODO: Error out if it isn't?
	size_t xml_size = static_cast<size_t>(size64);
	assert(xml_size % 2 == 0);
	xml_size &= ~1ULL;

	// Read the WIM XML data.
	file->rewind();
	file->seek(wimHeader.xml_resource.offset_of_xml); 
	// if seek is invalid
	if (file->tell() != static_cast<off64_t>(wimHeader.xml_resource.offset_of_xml)) {
		return -EIO;
	}

	// PugiXML memory allocation functions
	allocation_function xml_alloc = get_memory_allocation_function();
	deallocation_function xml_dealloc = get_memory_deallocation_function();

	char16_t *const xml_data = static_cast<char16_t*>(xml_alloc(xml_size));
	if (!xml_data) {
		// malloc() failure!
		return -ENOMEM;
	}
	size_t size = file->read(xml_data, static_cast<size_t>(xml_size));
	if (size != xml_size) {
		// Read error.
		xml_dealloc(xml_data);
		return -EIO;
	}

	xml_document doc;
	xml_parse_result result = doc.load_buffer_inplace_own(xml_data, xml_size, parse_default, encoding_utf16_le);
	if (!result) {
		return 3;
	}

	xml_node wim_element = doc.child("WIM");
	if (!wim_element) {
		// No wim element.
		// TODO: Better error code.
		return -EIO;
	}

	vector<WimIndex> images;
	images.reserve(number_of_images);
	xml_node currentimage = wim_element.child("IMAGE");

	const char *const s_unknown = C_("Wim", "(unknown)");
	for (uint32_t i = 0; i < wimHeader.number_of_images; i++) {
		assert(currentimage != nullptr);
		if (!currentimage) {
			break;
		}

		WimIndex currentindex;
		currentindex.index = i + 1;

		// the last modification time is split into a high part and a
		// low part so we shift and add them together
		xml_node creationTime = currentimage.child("CREATIONTIME");
		if (creationTime) {
			xml_text highPart = creationTime.child("HIGHPART").text();
			xml_text lowPart = creationTime.child("LOWPART").text();
			if (highPart && lowPart) {
				// Parse HIGHPART and LOWPART, then combine them like FILETIME.
				const uint32_t lastmodtime_high = strtoul(highPart.get(), nullptr, 16);
				const uint32_t lastmodtime_low = strtoul(lowPart.get(), nullptr, 16);
				currentindex.lastmodificationtime = WindowsSplitTimeToUnixTime(lastmodtime_high, lastmodtime_low);
			}
		}

		xml_node windowsinfo = currentimage.child("WINDOWS");
		if (windowsinfo) {
			currentindex.containswindowsimage = true;
			currentindex.windowsinfo.arch = static_cast<WimWindowsArchitecture>(
				windowsinfo.child("ARCH").text().as_int(0));
			currentindex.windowsinfo.editionid = windowsinfo.child("EDITIONID").text().as_string(s_unknown);

			xml_node languages = windowsinfo.child("LANGUAGES");
			if (languages) {
				// NOTE: Only retrieving the first language.
				currentindex.windowsinfo.languages.language = languages.child("LANGUAGE").text().get();
			}
			if (currentindex.windowsinfo.languages.language.empty()) {
				currentindex.windowsinfo.languages.language = s_unknown;
			}

			xml_node version = windowsinfo.child("VERSION");
			if (version) {
				currentindex.windowsinfo.version.majorversion = version.child("MAJOR").text().as_int(0);
				currentindex.windowsinfo.version.minorversion = version.child("MINOR").text().as_int(0);
				currentindex.windowsinfo.version.buildnumber = version.child("BUILD").text().as_int(0);
				currentindex.windowsinfo.version.spbuildnumber = version.child("SPBUILD").text().as_int(0);
			}
		} else {
			currentindex.containswindowsimage = false;
		}

		// some wims don't have these fields, so we
		// need to set up fallbacks - the hierarchy goes
		// display name -> name -> "(None)"
		const char *const s_none = C_("Wim", "(none)");
		currentindex.name = currentimage.child("NAME").text().as_string(s_none);
		currentindex.description = currentimage.child("DESCRIPTION").text().as_string(s_none);
		currentindex.dispname = currentimage.child("DISPLAYNAME").text().as_string(s_none);
		currentindex.dispdescription = currentimage.child("DISPLAYDESCRIPTION").text().as_string(s_none);

		images.push_back(std::move(currentindex));
		currentimage = currentimage.next_sibling();
	}

	auto *const vv_data = new RomFields::ListData_t();
	vv_data->reserve(number_of_images);

	// loop for the rows
	unsigned int idx = 1;
	for (const auto &image : images) {
		vv_data->resize(vv_data->size()+1);
		auto &data_row = vv_data->at(vv_data->size()-1);
		data_row.reserve(10);
		data_row.push_back(fmt::to_string(idx++));
		data_row.push_back(image.name);
		data_row.push_back(image.description);
		data_row.push_back(image.dispname);
		data_row.push_back(image.dispdescription);

		// Pack the 64-bit time_t into a string.
		RomFields::TimeString_t time_string;
		time_string.time = image.lastmodificationtime;
		data_row.emplace_back(time_string.str, sizeof(time_string.str));

		if (image.containswindowsimage == false) {
			// No Windows image. Add empty strings to complete the row.
			for (unsigned int i = 4; i > 0; i--) {
				data_row.emplace_back();
			}
			continue;
		}

		const auto &windowsver = image.windowsinfo.version;
		data_row.push_back(
			fmt::format(FSTR("{:d}.{:d}.{:d}.{:d}"),
				windowsver.majorversion, windowsver.minorversion,
				windowsver.buildnumber, windowsver.spbuildnumber));

		const auto &windowsinfo = image.windowsinfo;
		data_row.push_back(windowsinfo.editionid);
		const char *archstring;
		switch (windowsinfo.arch) {
			default:
				archstring = nullptr;
				break;
			case Wim_Arch_x86:
				archstring = "x86";
				break;
			case Wim_Arch_ARM32:
				archstring = "ARM32";
				break;
			case Wim_Arch_IA64:
				archstring = "IA64";
				break;
			case Wim_Arch_AMD64:
				archstring = "x64";
				break;
			case Wim_Arch_ARM64:
				archstring = "ARM64";
				break;
		}
		if (archstring) {
			data_row.emplace_back(archstring);
		} else {
			data_row.push_back(fmt::format(FRUN(C_("RomData", "Unknown ({:d})")),
				static_cast<int>(windowsinfo.arch)));
		}
		data_row.push_back(windowsinfo.languages.language);
	}	

	static const array<const char*, 10> field_names = {{
		NOP_C_("Wim|Images", "#"),
		NOP_C_("Wim|Images", "Name"),
		NOP_C_("Wim|Images", "Description"),
		NOP_C_("Wim|Images", "Display Name"),
		NOP_C_("Wim|Images", "Display Desc."),
		NOP_C_("Wim|Images", "Last Modified"),
		NOP_C_("Wim|Images", "OS Version"),
		NOP_C_("Wim|Images", "Edition"),
		NOP_C_("Wim|Images", "Architecture"),
		NOP_C_("Wim|Images", "Language"),
	}};
	vector<string> *const v_field_names = RomFields::strArrayToVector_i18n("Wim|Images", field_names);

	RomFields::AFLD_PARAMS params;
	params.flags = RomFields::RFT_LISTDATA_SEPARATE_ROW;
	params.col_attrs.is_timestamp = (1U << 5);
	params.col_attrs.dtflags = static_cast<RomFields::DateTimeFlags>(
		RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_HAS_TIME);
	params.headers = v_field_names;
	params.data.single = vv_data;
	// TODO: Header alignment?
	fields.addField_listData(C_("Wim", "Images"), &params);

	return 0;
}
#endif /* ENABLE_XML */

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
	d->addFields_XML();
#endif /* ENABLE_XML */

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

} // namespace LibRomData
