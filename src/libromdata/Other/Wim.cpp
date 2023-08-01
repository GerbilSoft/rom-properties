/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Wim.cpp: Microsoft WIM header reader                                    *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth and ecumber.                     *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Wim.hpp"
#include "Wim_structs.h"
// TinyXML2
#include "tinyxml2.h"
#include "librptext/conversion.hpp"
#include <cuchar>
#include <ctime>
using namespace LibRpText;

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;
using namespace tinyxml2;

// C++ STL classes.
using std::string;

// https://stackoverflow.com/a/74650247
#define windows_time_to_unix_epoch(x) ((x) - 116444736000000000LL) / 10000000LL
// for convenience
#define rowloop_current_image images[i]
#define rowloop_current_windowsinfo images[i].windowsinfo
#define rowloop_current_windowsver images[i].windowsinfo.version


namespace LibRomData {
class WimPrivate final : public RomDataPrivate
{
    public:
		WimPrivate(LibRpFile::IRpFile* file);  
		int addFields_XML();
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
		XMLDocument document;
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


typedef enum _WimWindowsArchitecture {
	x86 = 0,
	ARM32 = 5,
	IA64 = 6,
	AMD64 = 9,
	ARM64 = 12,
} WimWindowsArchitecture;

struct WimWindowsLanguages {
		std::string language;
		std::string default_language;
};

struct WimWindowsVersion {
		uint8_t majorversion = 0;
		uint8_t minorversion = 0;
		uint32_t buildnumber = 0;
		uint32_t spbuildnumber = 0;
		uint8_t splevel = 0; // only in windows 7+, added some time around build 6608-6730
};

struct WimWindowsInfo {
		WimWindowsArchitecture arch = WimWindowsArchitecture::x86;
		std::string productname, editionid, installationtype, hal, producttype, productsuite;
		WimWindowsLanguages languages;
		WimWindowsVersion version;
		std::string systemroot;
};

struct WimIndex {
		// if you have more than 2^32 indices in a wim
		// you probably have bigger issues
		uint32_t index = 0;
		uint64_t dircount = 0;
		uint64_t filecount = 0;
		uint64_t totalbytes = 0;
		uint64_t creationtime = 0;
		uint64_t lastmodificationtime = 0;
		WimWindowsInfo windowsinfo;
		std::string name, description, flags, dispname, dispdescription;
		bool containswindowsimage;
};

int WimPrivate::addFields_XML() 
{
	//assert(file);
	file->rewind();
	// the eighth byte of the "size" is used for flags so we have to AND it
	const uint64_t size = (wimHeader.xml_resource.size & 0x00FFFFFFFFFFFFFF);
	char* xml_data = new char[size+2];
	memset(xml_data, 0, size+2);
	file->seek(wimHeader.xml_resource.offset_of_xml); 
	// if seek is invalid
	if (file->tell() != wimHeader.xml_resource.offset_of_xml)
		return 1;

	size_t real_bytes_read = file->read(xml_data, size);
	
	assert(real_bytes_read == size);

	if (size >= 0x7FFFFFFF) /* congrats, you have broken the utf8 converter */ return 2;

	// the xml inside wims are utf-16 but tinyxml only supports utf-8
	// this means we have to do some conversion
	std::string utf8_xml = LibRpText::utf16_to_utf8(reinterpret_cast<char16_t*>(xml_data), (int)ceil((size) / 2));

	delete[](xml_data);
#if TINYXML2_MAJOR_VERSION >= 2
	document.Clear();
#endif /* TINYXML2_MAJOR_VERSION >= 2 */
	int xerr = document.Parse(utf8_xml.c_str(), utf8_xml.size());
	assert(xerr == XML_SUCCESS);
	if (xerr != XML_SUCCESS)
	{
#if TINYXML2_MAJOR_VERSION >= 2
		document.Clear();
#endif /* TINYXML2_MAJOR_VERSION >= 2 */
		return 3;
	} 

	const XMLElement* const wim_element = document.FirstChildElement("WIM");

	if (!wim_element) {
		// No wim element.
		// TODO: Better error code.
#if TINYXML2_MAJOR_VERSION >= 2
		document.Clear();
#endif /* TINYXML2_MAJOR_VERSION >= 2 */
		return -EIO;
	}

	std::vector<WimIndex> images(0);
	images.reserve(wimHeader.number_of_images);
	const XMLElement* currentimage = wim_element->FirstChildElement("IMAGE");

	for (uint32_t i = 0; i <= wimHeader.number_of_images - 1; i++) {
		WimIndex currentindex;

		currentindex.index = i + 1;

		// the last modification time is split into a high part and a
		// low part so we shift and add them together
		uint32_t lastmodtime_high = std::stoul(
			currentimage->FirstChildElement("CREATIONTIME")->FirstChildElement("HIGHPART")->GetText(),
			nullptr, 16);
		uint32_t lastmodtime_low = std::stoul(
			currentimage->FirstChildElement("CREATIONTIME")->FirstChildElement("LOWPART")->GetText(),
			nullptr, 16);
		currentindex.lastmodificationtime = ((uint64_t)lastmodtime_high << 32) + lastmodtime_low;
		const XMLElement* windowsinfo = currentimage->FirstChildElement("WINDOWS");
		if (windowsinfo) {
			currentindex.containswindowsimage = true;
			currentindex.windowsinfo.arch =
				static_cast<WimWindowsArchitecture>(windowsinfo->FirstChildElement("ARCH")->IntText(0));
			windowsinfo->FirstChildElement("EDITIONID") != nullptr
				? currentindex.windowsinfo.editionid = windowsinfo->FirstChildElement("EDITIONID")->GetText()
				: currentindex.windowsinfo.editionid = "(Unknown)";
			windowsinfo->FirstChildElement("LANGUAGES") != nullptr
				? currentindex.windowsinfo.languages.language = windowsinfo->FirstChildElement("LANGUAGES")->FirstChildElement("LANGUAGE")->GetText()
				: currentindex.windowsinfo.languages.language = "(Unknown)";
			const XMLElement* version = windowsinfo->FirstChildElement("VERSION");
			currentindex.windowsinfo.version.majorversion = version->FirstChildElement("MAJOR")->IntText(0);
			currentindex.windowsinfo.version.minorversion = version->FirstChildElement("MINOR")->IntText(0);
			currentindex.windowsinfo.version.buildnumber = version->FirstChildElement("BUILD")->IntText(0);
			currentindex.windowsinfo.version.spbuildnumber =
				version->FirstChildElement("SPBUILD")->IntText(0);
		} else
			currentindex.containswindowsimage = false;

		// some wims don't have these fields, so we
		// need to set up fallbacks - the hierarchy goes
		// display name -> name -> "(None)"
		currentimage->FirstChildElement("NAME") != nullptr
			? currentindex.name = currentimage->FirstChildElement("NAME")->GetText()
			: currentindex.name = "(None)";
		currentimage->FirstChildElement("DESCRIPTION") != nullptr
			? currentindex.description = currentimage->FirstChildElement("DESCRIPTION")->GetText()
			: currentindex.description = "(None)";
		currentimage->FirstChildElement("DISPLAYNAME") != nullptr
			? currentindex.dispname = currentimage->FirstChildElement("DISPLAYNAME")->GetText()
			: currentindex.dispname = "(None)";
		currentimage->FirstChildElement("DISPLAYDESCRIPTION") != nullptr
			? currentindex.dispdescription = currentimage->FirstChildElement("DISPLAYDESCRIPTION")->GetText()
			: currentindex.dispdescription = "(None)";

		images.push_back(currentindex);
		currentimage = currentimage->NextSiblingElement();
	}

	auto vv_data = new RomFields::ListData_t();
	vv_data->reserve(10);

	char timestamp[20];
	tm* tm_struct;
	// loop for the rows
	for (uint32_t i = 0; i <= wimHeader.number_of_images-1; i++) {
		
		vv_data->resize(vv_data->size()+1);
		auto &data_row = vv_data->at(vv_data->size()-1);
		data_row.emplace_back(rp_sprintf("%d", i + 1));
		data_row.emplace_back(rowloop_current_image.name);
		data_row.emplace_back(rowloop_current_image.description);
		data_row.emplace_back(rowloop_current_image.dispname);
		data_row.emplace_back(rowloop_current_image.dispdescription);
		time_t time = windows_time_to_unix_epoch(rowloop_current_image.lastmodificationtime);
		tm_struct = localtime(&time);
		std::strftime(timestamp, 20, "%Y-%m-%d %R", tm_struct);
		data_row.emplace_back(timestamp);

		if (images[i].containswindowsimage == false)
			continue;
		
		data_row.emplace_back(
			rp_sprintf("%d.%d.%d.%d", rowloop_current_windowsver.majorversion,
				rowloop_current_windowsver.minorversion, rowloop_current_windowsver.buildnumber,
				rowloop_current_windowsver.spbuildnumber));
		data_row.emplace_back(rowloop_current_windowsinfo.editionid);
		std::string archstring = "";
		switch (rowloop_current_windowsinfo.arch)
		{
			case WimWindowsArchitecture::x86:
				archstring = "x86";
				break;
			case WimWindowsArchitecture::ARM32:
				archstring = "ARM32";
				break;
			case WimWindowsArchitecture::IA64:
				archstring = "IA64";
				break;
			case WimWindowsArchitecture::AMD64:
				archstring = "x64";
				break;
			case WimWindowsArchitecture::ARM64:
				archstring = "ARM64";
				break;
		}
		data_row.emplace_back(archstring);
		data_row.emplace_back(rowloop_current_windowsinfo.languages.language);
	}	

	static const char* const field_names[] = {
		NOP_C_("RomData", "#"),
		NOP_C_("RomData", "Name"),
		NOP_C_("RomData", "Description"),
		NOP_C_("RomData", "Display Name"),
		NOP_C_("RomData", "Display Desc."),
		NOP_C_("RomData", "Last Modified"),
		NOP_C_("RomData", "OS Version"),
		NOP_C_("RomData", "Edition"),
		NOP_C_("RomData", "Architecture"),
		NOP_C_("RomData", "Language"),
	};
	std::vector<std::string>* const v_field_names = RomFields::strArrayToVector_i18n(
		"RomData", field_names, ARRAY_SIZE(field_names));

	RomFields::AFLD_PARAMS params;
	params.flags = RomFields::RFT_LISTDATA_SEPARATE_ROW;
	params.headers = v_field_names;
	params.data.single = vv_data;
	// TODO: Header alignment?
	fields.addField_listData(C_("RomData", "Images"), &params);

	return 0;
}

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
        wimData->version.minor_version >= 13 ? version_type = Wim113_014 : version_type = Wim109_112;
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
	d->fileType = FileType::DiskImage;

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
		"Microsoft WIM",
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
	// if the version number is 14, add an indicator that it is an ESD
	d->wimHeader.version.minor_version == 14
		? snprintf(buffer, 14, "%d.%02d (ESD)", d->wimHeader.version.major_version,
			  d->wimHeader.version.minor_version)
		: snprintf(
			  buffer, 8, "%d.%02d", d->wimHeader.version.major_version, d->wimHeader.version.minor_version);
	d->fields.addField_string(C_("RomData", "WIM Version"), buffer, RomFields::STRF_TRIM_END);

	if (version_type != Wim113_014) 
		return 0;

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
	snprintf(buffer, 4, "%d", d->wimHeader.number_of_images);
	d->fields.addField_string(C_("RomData", "Total Images"), buffer, RomFields::STRF_TRIM_END);

	d->addFields_XML();

	return 0;
}

}