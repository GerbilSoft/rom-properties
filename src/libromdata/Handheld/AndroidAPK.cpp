/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AndroidAPK.cpp: Android APK package reader.                             *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "config.librpbase.h"
#include "AndroidAPK.hpp"

// MiniZip
#include <zlib.h>
#include "mz_zip.h"
#include "compat/ioapi.h"
#include "compat/unzip.h"

// Other rom-properties libraries
#include "librpbase/img/RpPng.hpp"
#include "librpfile/MemFile.hpp"
using namespace LibRpBase;
using namespace LibRpText;
using namespace LibRpFile;
using namespace LibRpTexture;

// PugiXML
// NOTE: This file is only compiled if ENABLE_XML is defined.
#include <pugixml.hpp>
using namespace pugi;

// C++ STL classes
#include <limits>
#include <stack>
using std::array;
using std::map;
using std::ostringstream;
using std::stack;
using std::string;
using std::unique_ptr;
using std::vector;

// Uninitialized vector class
#include "uvector.h"

#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRomData {

#ifdef _MSC_VER
// DelayLoad test implementations
#  ifdef ZLIB_IS_DLL
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#  endif /* ZLIB_IS_DLL */
#  ifdef MINIZIP_IS_DLL
// unzClose() can safely take nullptr; it won't do anything.
DELAYLOAD_TEST_FUNCTION_IMPL1(unzClose, nullptr);
#  endif /* MINIZIP_IS_DLL */
#  ifdef XML_IS_DLL
/**
 * Check if PugiXML can be delay-loaded.
 * NOTE: Implemented in EXE_delayload.cpp.
 * @return 0 on success; negative POSIX error code on error.
 */
extern int DelayLoad_test_PugiXML(void);
#  endif /* XML_IS_DLL */
#endif /* _MSC_VER */

class AndroidAPKPrivate final : public RomDataPrivate
{
public:
	explicit AndroidAPKPrivate(const IRpFilePtr &file);
	~AndroidAPKPrivate() override;

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(AndroidAPKPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// Opened .apk file
	unzFile apkFile;

public:
	/**
	 * Open a Zip file for reading.
	 * @param filename Zip filename.
	 * @return Zip file, or nullptr on error.
	 */
	static unzFile openZip(const char *filename);

	/**
	 * Load a file from the opened .jar file.
	 * @param filename Filename to load
	 * @param max_size Maximum size
	 * @return rp::uvector with the file data, or empty vector on error
	 */
	rp::uvector<uint8_t> loadFileFromZip(const char *filename, size_t max_size = std::numeric_limits<size_t>::max());

	/**
	 * Read a little-endian 32-bit word from the data.
	 * @param pData Data.
	 * @param dataLen Length of data. (32-bit maximum)
	 * @param offset Offset to read from.
	 * @return Little-endian 32-bit word.
	 */
	static inline uint32_t LEW(const uint8_t *pData, uint32_t dataLen, size_t offset)
	{
		assert(offset + 3 < dataLen);
		if (offset + 3 >= dataLen) {
			// Out of bounds.
			// TODO: Fail.
			return 0;
		}

		// TODO: Optimize with a 32-bit read?
		return (pData[offset+3] << 24) |
		       (pData[offset+2] << 16) |
		       (pData[offset+1] <<  8) |
		        pData[offset+0];
	}

	/**
	 * Return the string stored in StringTable format at offset strOff.
	 * This offset points to the 16-bit string length, which is followed
	 * by that number of 16-bit (Unicode) characters.
	 */
	static string compXmlStringAt(const uint8_t *pXml, uint32_t xmlLen, uint32_t strOff);

	/**
	 * Compose an XML string.
	 * @param pXml Compressed XML data.
	 * @param xmlLen Length of pXml. (32-bit maximum)
	 * @param sitOff StringIndexTable offset.
	 * @param stOff StringTable offset.
	 * @param strInd String index.
	 * @return XML string.
	 */
	static string compXmlString(const uint8_t *pXml, uint32_t xmlLen, uint32_t sitOff, uint32_t stOff, int strInd);

	/**
	 * Decompress Android binary XML.
	 * @param pXml Android binary XML data
	 * @param xmlLen Size of data
	 * @return PugiXML document, or an empty document on error.
	 */
	ATTR_ACCESS_SIZE(read_only, 1, 2)
	static xml_document decompressAndroidBinaryXml(const uint8_t *pXml, size_t xmlLen);

	/**
	 * Load AndroidManifest.xml from this->apkFile.
	 * this->apkFile must have already been opened.
	 * TODO: Store it in a PugiXML document, but need to check delay-load...
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadAndroidManifestXml(void);

public:
	// Maximum size for various files.
	static constexpr size_t AndroidManifest_xml_FILE_SIZE_MAX = (256U * 1024U);
	static constexpr size_t resources_asrc_FILE_SIZE_MAX = (4096U * 1024U);

	// Binary XML tags
	static constexpr uint32_t endDocTag	= 0x00100101;
	static constexpr uint32_t startTag	= 0x00100102;
	static constexpr uint32_t endTag	= 0x00100103;

	// AndroidManifest.xml document
	// NOTE: Using a pointer to prevent delay-load issues.
	unique_ptr<xml_document> manifest_xml;
};

ROMDATA_IMPL(AndroidAPK)

/** AndroidAPKPrivate **/

/* RomDataInfo */
const array<const char*, 1+1> AndroidAPKPrivate::exts = {{
	".apk",

	nullptr
}};
const array<const char*, 1+1> AndroidAPKPrivate::mimeTypes = {{
	// Vendor-specific MIME types from FreeDesktop.org.
	"application/vnd.android.package-archive",	// .jad

	nullptr
}};
const RomDataInfo AndroidAPKPrivate::romDataInfo = {
	"AndroidAPK", exts.data(), mimeTypes.data()
};


AndroidAPKPrivate::AndroidAPKPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, apkFile(nullptr)
{}

AndroidAPKPrivate::~AndroidAPKPrivate()
{
	if (apkFile) {
		unzClose(apkFile);
	}
}

/**
 * Open a Zip file for reading.
 * @param filename Zip filename.
 * @return Zip file, or nullptr on error.
 */
unzFile AndroidAPKPrivate::openZip(const char *filename)
{
#ifdef _WIN32
	// NOTE: MiniZip-NG 3.0.2's compatibility functions
	// take UTF-8 on Windows, not UTF-16.
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64(&ffunc);
	return unzOpen2_64(filename, &ffunc);
#else /* !_WIN32 */
	return unzOpen(filename);
#endif /* _WIN32 */
}

/**
 * Load a file from the opened .jar file.
 * @param filename Filename to load
 * @param max_size Maximum size
 * @return rp::uvector with the file data, or empty vector on error
 */
rp::uvector<uint8_t> AndroidAPKPrivate::loadFileFromZip(const char *filename, size_t max_size)
{
	// TODO: This is also used by GcnFstTest. Move to a common utility file?
	int ret = unzLocateFile(apkFile, filename, nullptr);
	if (ret != UNZ_OK) {
		return {};
	}

	// Get file information.
	unz_file_info64 file_info;
	ret = unzGetCurrentFileInfo64(apkFile,
		&file_info, nullptr, 0, nullptr, 0, nullptr, 0);
	if (ret != UNZ_OK || file_info.uncompressed_size >= max_size) {
		// Error getting file information, or the uncompressed size is too big.
		return {};
	}

	ret = unzOpenCurrentFile(apkFile);
	if (ret != UNZ_OK) {
		return {};
	}

	size_t size = static_cast<size_t>(file_info.uncompressed_size);
	rp::uvector<uint8_t> buf;
	buf.resize(size);

	// Read the file.
	// NOTE: zlib and minizip are only guaranteed to be able to
	// read UINT16_MAX (64 KB) at a time, and the updated MiniZip
	// from https://github.com/nmoinvaz/minizip enforces this.
	uint8_t *p = buf.data();
	while (size > 0) {
		int to_read = static_cast<int>(size > UINT16_MAX ? UINT16_MAX : size);
		ret = unzReadCurrentFile(apkFile, p, to_read);
		if (ret != to_read) {
			return {};
		}

		// ret == number of bytes read.
		p += ret;
		size -= ret;
	}

	// Close the file.
	// An error will occur here if the CRC is incorrect.
	ret = unzCloseCurrentFile(apkFile);
	if (ret != UNZ_OK) {
		return {};
	}

	return buf;
}

/**
 * Return the string stored in StringTable format at offset strOff.
 * This offset points to the 16-bit string length, which is followed
 * by that number of 16-bit (Unicode) characters.
 */
string AndroidAPKPrivate::compXmlStringAt(const uint8_t *pXml, uint32_t xmlLen, uint32_t strOff)
{
	assert(strOff + 1 < xmlLen);
	if (strOff + 1 >= xmlLen) {
		// Out of bounds.
		return string();
	}

	const uint16_t len = (pXml[strOff+1] << 8) |
			      pXml[strOff+0];
	assert(strOff + 2 + len < xmlLen);
	if (strOff + 2 + len >= xmlLen) {
		// Out of bounds.
		return string();
	}

	// Convert from UTF-16LE to UTF-8.
	// TODO: Verify that it's always UTF-16LE.
	return utf16le_to_utf8(reinterpret_cast<const char16_t*>(&pXml[strOff + 2]), len);
} // end of compXmlStringAt

/**
 * Compose an XML string.
 * @param pXml Compressed XML data.
 * @param xmlLen Length of pXml. (32-bit maximum)
 * @param sitOff StringIndexTable offset.
 * @param stOff StringTable offset.
 * @param strInd String index.
 * @return XML string.
 */
string AndroidAPKPrivate::compXmlString(const uint8_t *pXml, uint32_t xmlLen, uint32_t sitOff, uint32_t stOff, int strInd)
{
	assert(strInd >= 0);
	if (strInd < 0) {
		// Invalid string index.
		return string();
	}

	const uint32_t addr = sitOff + (strInd * 4);
	assert(addr < xmlLen);
	if (addr >= xmlLen) {
		// Out of bounds.
		return string();
	}

	uint32_t strOff = stOff + LEW(pXml, xmlLen, addr);
	return compXmlStringAt(pXml, xmlLen, strOff);
}

/**
 * Decompress Android binary XML.
 * @param pXml Android binary XML data
 * @param xmlLen Size of data
 * @return PugiXML document, or an empty document on error.
 */
xml_document AndroidAPKPrivate::decompressAndroidBinaryXml(const uint8_t *pXml, size_t xmlLen)
{
	// Reference:
	// - https://stackoverflow.com/questions/2097813/how-to-parse-the-androidmanifest-xml-file-inside-an-apk-package
	// - https://stackoverflow.com/a/4761689
	// TODO: Test on lots of Android packages to find any issues.
	// TODO: Split into a separate class to convert Android binary XML to text XML?
	// TODO: Instead of creating text, parse directly into a PugiXML document?

	// Compressed XML file/bytes starts with 24x bytes of data,
	// 9 32 bit words in little endian order (LSB first):
	//   0th word is 03 00 08 00
	//   3rd word SEEMS TO BE:  Offset at then of StringTable
	//   4th word is: Number of strings in string table
	// WARNING: Sometime I indiscriminently display or refer to word in 
	//   little endian storage format, or in integer format (ie MSB first).	
	const uint32_t numbStrings = LEW(pXml, xmlLen, 4*4);

	// StringIndexTable starts at offset 24x, an array of 32 bit LE offsets
	// of the length/string data in the StringTable.
	static const uint32_t sitOff = 0x24;  // Offset of start of StringIndexTable

	// StringTable, each string is represented with a 16 bit little endian 
	// character count, followed by that number of 16 bit (LE) (Unicode) chars.
	const uint32_t stOff = sitOff + numbStrings*4;  // StringTable follows StrIndexTable

	// XMLTags, The XML tag tree starts after some unknown content after the
	// StringTable.  There is some unknown data after the StringTable, scan
	// forward from this point to the flag for the start of an XML start tag.
	uint32_t xmlTagOff = LEW(pXml, xmlLen, 3*4);  // Start from the offset in the 3rd word.
	// Scan forward until we find the bytes: 0x02011000(x00100102 in normal int)
	for (unsigned int ii = xmlTagOff; ii < xmlLen-4; ii += 4) {
		if (LEW(pXml, xmlLen, ii) == startTag) { 
			xmlTagOff = ii;
			break;
		}
	} // end of hack, scanning for start of first start tag

	// XML tags and attributes:
	// Every XML start and end tag consists of 6 32 bit words:
	//   0th word: 02011000 for startTag and 03011000 for endTag 
	//   1st word: a flag?, like 38000000
	//   2nd word: Line of where this tag appeared in the original source file
	//   3rd word: FFFFFFFF ??
	//   4th word: StringIndex of NameSpace name, or FFFFFFFF for default NS
	//   5th word: StringIndex of Element Name
	//   (Note: 01011000 in 0th word means end of XML document, endDocTag)

	// Start tags (not end tags) contain 3 more words:
	//   6th word: 14001400 meaning?? 
	//   7th word: Number of Attributes that follow this tag(follow word 8th)
	//   8th word: 00000000 meaning??

	// Attributes consist of 5 words: 
	//   0th word: StringIndex of Attribute Name's Namespace, or FFFFFFFF
	//   1st word: StringIndex of Attribute Name
	//   2nd word: StringIndex of Attribute Value, or FFFFFFF if ResourceId used
	//   3rd word: Flags?
	//   4th word: str ind of attr value again, or ResourceId of value

	// TMP, dump string table to tr for debugging
	//tr.addSelect("strings", null);
	//for (int ii=0; ii<numbStrings; ii++) {
	//  // Length of string starts at StringTable plus offset in StrIndTable
	//  String str = compXmlString(xml, sitOff, stOff, ii);
	//  tr.add(String.valueOf(ii), str);
	//}
	//tr.parent();

	// Create a PugiXML document.
	xml_document doc;
	// Stack of tags currently being processed.
	stack<xml_node> tags;
	tags.push(doc);
	xml_node cur_node = doc;	// current XML node

	// Step through the XML tree element tags and attributes
	uint32_t off = xmlTagOff;
	while (off < xmlLen) {
		uint32_t tag0 = LEW(pXml, xmlLen, off);
		//uint32_t tag1 = LEW(pXml, xmlLen, off+1*4);
		uint32_t lineNo = LEW(pXml, xmlLen, off+2*4);
		//uint32_t tag3 = LEW(pXml, xmlLen, off+3*4);
		uint32_t nameNsSi = LEW(pXml, xmlLen, off+4*4);
		int nameSi = LEW(pXml, xmlLen, off+5*4);

		if (tag0 == startTag) { // XML START TAG
			uint32_t tag6 = LEW(pXml, xmlLen, off+6*4);  // Expected to be 14001400
			uint32_t numbAttrs = LEW(pXml, xmlLen, off+7*4);  // Number of Attributes to follow
			//uint32_t tag8 = LEW(pXml, xmlLen, off+8*4);  // Expected to be 00000000
			off += 9*4;  // Skip over 6+3 words of startTag data

			// Create the tag.
			xml_node xmlTag = cur_node.append_child(compXmlString(pXml, xmlLen, sitOff, stOff, nameSi));
			tags.push(xmlTag);
			cur_node = xmlTag;
			//tr.addSelect(name, null);

			// Look for the Attributes
			for (uint32_t ii = 0; ii < numbAttrs; ii++) {
				uint32_t attrNameNsSi = LEW(pXml, xmlLen, off);  // AttrName Namespace Str Ind, or FFFFFFFF
				int attrNameSi = LEW(pXml, xmlLen, off+1*4);  // AttrName String Index
				int attrValueSi = LEW(pXml, xmlLen, off+2*4); // AttrValue Str Ind, or FFFFFFFF
				uint32_t attrFlags = LEW(pXml, xmlLen, off+3*4);  
				uint32_t attrResId = LEW(pXml, xmlLen, off+4*4);  // AttrValue ResourceId or dup AttrValue StrInd
				off += 5*4;  // Skip over the 5 words of an attribute

				xml_attribute xmlAttr = xmlTag.append_attribute(compXmlString(pXml, xmlLen, sitOff, stOff, attrNameSi));
				if (attrValueSi != -1) {
					// Value is inline
					xmlAttr.set_value(compXmlString(pXml, xmlLen, sitOff, stOff, attrValueSi));
				} else {
					// Value is in the resource file
					// TODO: Get the resource.
					char buf[48];
					snprintf(buf, sizeof(buf), "resourceID 0x%x", attrResId);
					xmlAttr.set_value(buf);
				}
				//tr.add(attrName, attrValue);
			}
			// TODO: If no child elements, skip the '\n' and use />.
		} else if (tag0 == endTag) { // XML END TAG
			// End of the current tag.
			assert(tags.size() >= 2);
			if (tags.size() < 2) {
				// Less than 2 tags on the stack.
				// The first tag is the document, so this means there is a
				// stray end tag somewhere...
				return {};
			}
			tags.pop();
			cur_node = tags.top();
			off += 6*4;  // Skip over 6 words of endTag data
			//tr.parent();  // Step back up the NobTree
		} else if (tag0 == endDocTag) {  // END OF XML DOC TAG
			break;
		} else {
			assert(!"Unrecognized tag code!");
			return {};
		}
	} // end of while loop scanning tags and attributes of XML tree

	assert(tags.size() == 1);
	if (tags.size() != 1) {
		// The tag stack is incorrect.
		// We should only have one tag left: the root document node.
		return {};
	}

	// XML document decompressed.
	return doc;
}

/**
 * Load AndroidManifest.xml from this->apkFile.
 * this->apkFile must have already been opened.
 * TODO: Store it in a PugiXML document, but need to check delay-load...
 * @return 0 on success; negative POSIX error code on error.
 */
int AndroidAPKPrivate::loadAndroidManifestXml(void)
{
	if (manifest_xml) {
		// AndroidManifest.xml is already loaded.
		return 0;
	}

	// The .apk file must have been opened already.
	assert(apkFile != nullptr);
	if (!apkFile) {
		return -EIO;
	}

	// Load AndroidManifest.xml.
	// TODO: May need to load resources too.
	rp::uvector<uint8_t> AndroidManifest_xml_buf = loadFileFromZip(
		"AndroidManifest.xml", AndroidManifest_xml_FILE_SIZE_MAX);
	if (AndroidManifest_xml_buf.empty()) {
		// Unable to load AndroidManifest.xml.
		return -ENOENT;
	}

	xml_document xml = decompressAndroidBinaryXml(AndroidManifest_xml_buf.data(), AndroidManifest_xml_buf.size());
	if (xml.empty()) {
		// Empty???
		manifest_xml.reset();
		return -EIO;
	}

	manifest_xml.reset(new xml_document);
	*manifest_xml = std::move(xml);
	return 0;
}

/** AndroidAPK **/

/**
 * Read a AndroidAPK .jar or .jad file.
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
AndroidAPK::AndroidAPK(const IRpFilePtr &file)
	: super(new AndroidAPKPrivate(file))
{
	// This class handles application packages.
	RP_D(AndroidAPK);
	d->mimeType = "application/vnd.android.package-archive";	// vendor-specific
	d->fileType = FileType::ApplicationPackage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

#ifdef _MSC_VER
	// Delay load verification.
#  ifdef ZLIB_IS_DLL
	// Only if zlib is a DLL.
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		// Android packages cannot be read without MiniZip.
		d->isValid = false;
		d->file.reset();
		return;
	}
#  else /* !ZLIB_IS_DLL */
	// zlib isn't in a DLL, but we need to ensure that the
	// CRC table is initialized anyway.
	get_crc_table();
#  endif /* ZLIB_IS_DLL */

#  ifdef MINIZIP_IS_DLL
	// Only if MiniZip is a DLL.
	if (DelayLoad_test_unzClose() != 0) {
		// Delay load failed.
		// Android packages cannot be read without MiniZip.
		d->isValid = false;
		d->file.reset();
		return;
	}
#  endif /* MINIZIP_IS_DLL */

#  ifdef XML_IS_DLL
	int ret_dl = DelayLoad_test_PugiXML();
	if (ret_dl != 0) {
		// Delay load failed.
		return ret_dl;
	}
#  endif /* XML_IS_DLL */
#endif /* _MSC_VER */

	// Seek to the beginning of the file.
	d->file->rewind();

	// Read the file header. (at least 32 bytes)
	uint8_t header[32];
	size_t size = d->file->read(header, sizeof(header));
	if (size < 32) {
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const char *const filename = file->filename();
	const DetectInfo info = {
		{0, sizeof(header), header},
		FileSystem::file_ext(filename),	// ext
		0				// szFile (not needed for AndroidAPK)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// Attempt to open as a .zip file first.
	// TODO: Custom MiniZip functions to use IRpFile so we can use IStream?
	d->apkFile = d->openZip(file->filename());
	if (!d->apkFile) {
		// Cannot open as a .zip file.
		d->isValid = false;
		d->file.reset();
		return;
	}

	// Attempt to load AndroidManifest.xml.
	if (d->loadAndroidManifestXml() != 0) {
		// Failed to load AndroidManifest.xml.
		d->isValid = false;
		d->file.reset();
		return;
	}
}

/**
 * Close the opened file.
 */
void AndroidAPK::close(void)
{
	RP_D(AndroidAPK);
	if (d->apkFile) {
		unzClose(d->apkFile);
		d->apkFile = nullptr;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int AndroidAPK::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	if (!info || !info->header.pData) {
		return -1;
	}

	// .apk check: If this is a .zip file, we can try to open it.
	if (info->header.size >= 4) {
		const uint32_t *const pData32 =
			reinterpret_cast<const uint32_t*>(info->header.pData);
		if (pData32[0] == cpu_to_be32(0x504B0304)) {
			// This appears to be a .zip file. (PK\003\004)
			// TODO: Also check for these?:
			// - PK\005\006 (empty)
			// - PK\007\008 (spanned)
			return 0;
		}
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @return System name, or nullptr if not supported.
 */
const char *AndroidAPK::systemName(unsigned int type) const
{
	RP_D(const AndroidAPK);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// AndroidAPK has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"AndroidAPK::systemName() array index optimization needs to be updated.");

	static const array<const char*, 4> sysNames = {{
		"Google Android", "Android", "Android", nullptr,
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int AndroidAPK::loadFieldData(void)
{
	RP_D(AndroidAPK);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || !d->manifest_xml) {
		// APK isn't valid, and/or AndroidManifest.xml could not be loaded.
		return -EIO;
	}

	d->fields.reserve(5);	// Maximum of 5 fields.

	// Get fields from the XML file.
	xml_node manifest_node = d->manifest_xml->child("manifest");
	if (!manifest_node) {
		// No "<manifest>" node???
		return static_cast<int>(d->fields.count());
	}

	// Application information
	xml_node application_node = manifest_node.child("application");
	if (application_node) {
		const char *const label = application_node.attribute("label").as_string(nullptr);
		if (label && label[0] != '\0') {
			d->fields.addField_string(C_("AndroidAPK", "Title"), label);
		}

		const char *const name = application_node.attribute("name").as_string(nullptr);
		if (name && name[0] != '\0') {
			d->fields.addField_string(C_("AndroidAPK", "Package Name"), name);
		}

		const char *const description = application_node.attribute("description").as_string(nullptr);
		if (description && description[0] != '\0') {
			d->fields.addField_string(C_("AndroidAPK", "Description"), description);
		}
	}

	// Copied from Nintendo3DS. (TODO: Centralize it?)
#ifdef _WIN32
	// Windows: 6 visible rows per RFT_LISTDATA.
	static constexpr int rows_visible = 6;
#else /* !_WIN32 */
	// Linux: 4 visible rows per RFT_LISTDATA.
	static constexpr int rows_visible = 4;
#endif /* _WIN32 */

	// Features
	// TODO: Normalize/localize feature names?
	// TODO: How to handle "Required"?
	xml_node feature_node = manifest_node.child("uses-feature");
	if (feature_node) {
		auto *const vv_features = new RomFields::ListData_t;
		do {
			const char *const feature = feature_node.attribute("name").as_string(nullptr);
			if (feature && feature[0] != '\0') {
				vector<string> v_feature;
				v_feature.push_back(feature);
				vv_features->push_back(std::move(v_feature));
			}

			// Next feature
			feature_node = feature_node.next_sibling("uses-feature");
		} while (feature_node);

		if (!vv_features->empty()) {
			RomFields::AFLD_PARAMS params(0, rows_visible);
			params.headers = nullptr;
			params.data.single = vv_features;
			d->fields.addField_listData(C_("AndroidAPK", "Features"), &params);
		} else {
			delete vv_features;
		}
	}

	// Permissions
	// TODO: Normalize/localize permission names?
	// TODO: maxSdkVersion?
	// TODO: Also handle "uses-permission-sdk-23"?
	xml_node permission_node = manifest_node.child("uses-permission");
	if (permission_node) {
		auto *const vv_permissions = new RomFields::ListData_t;
		do {
			const char *const permission = permission_node.attribute("name").as_string(nullptr);
			if (permission && permission[0] != '\0') {
				vector<string> v_permission;
				v_permission.push_back(permission);
				vv_permissions->push_back(std::move(v_permission));
			}

			// Next permission
			permission_node = permission_node.next_sibling("uses-permission");
		} while (permission_node);

		if (!vv_permissions->empty()) {
			RomFields::AFLD_PARAMS params(0, rows_visible);
			params.headers = nullptr;
			params.data.single = vv_permissions;
			d->fields.addField_listData(C_("AndroidAPK", "Permissions"), &params);
		} else {
			delete vv_permissions;
		}
	}

	return static_cast<int>(d->fields.count());
}

} // namespace LibRomData
