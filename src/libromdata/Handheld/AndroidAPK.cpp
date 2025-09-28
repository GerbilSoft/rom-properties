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
#include "android_apk_structs.h"

// MiniZip
#include <zlib.h>
#include "mz_zip.h"
#include "compat/ioapi.h"
#include "compat/unzip.h"

// Other rom-properties libraries
#include "librpbase/img/RpPng.hpp"
#include "librpbase/SystemRegion.hpp"
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
using std::unordered_map;
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

	// Icon
	rp_image_ptr img_icon;

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
	 * Process an Android resource string pool.
	 * @param data Start of string pool
	 * @param size Size of string pool
	 * @return Processed string pool, or empty vector<string> on error.
	 */
	ATTR_ACCESS_SIZE(read_only, 1, 2)
	static vector<string> processStringPool(const uint8_t *data, size_t size);

	/**
	 * Convert an Android locale or country code to rom-properties language and country codes.
	 * @param alocale Android locale
	 * @param rLC rom-properties language code
	 * @param rCC rom-properties country code
	 */
	static void androidLocaleToRP(uint32_t alocale, uint32_t &rLC, uint32_t &rCC);

	/**
	 * Process RES_TABLE_TYPE_TYPE.
	 * @param data Start of type
	 * @param size Size of type
	 * @param package_id Package ID
	 * @return 0 on success; negative POSIX error code on error.
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	int processType(const uint8_t *data, size_t size, unsigned int package_id);

	/**
	 * Process an Android resource package.
	 * @param data Start of package
	 * @param size Size of package
	 * @return 0 on success; negative POSIX error code on error.
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	int processPackage(const uint8_t *data, size_t size);

	/**
	 * Load Android resource data.
	 * @param pArsc		[in] Android resource data
	 * @param arscLen	[in] Size of resource data
	 * @return 0 on success; negative POSIX error code on error.
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	int loadResourceAsrc(const uint8_t *pArsc, size_t arscLen);

	/**
	 * Get a string from Android resource data.
	 * @param id		[in] Resource ID
	 * @return String, or nullptr if not found.
	 */
	const char *getStringFromResource(uint32_t id);

	/**
	 * Decompress Android binary XML.
	 * Strings that are referencing resources will be printed as "@0x12345678".
	 * @param pXml		[in] Android binary XML data
	 * @param xmlLen	[in] Size of XML data
	 * @return PugiXML document, or an empty document on error.
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	xml_document decompressAndroidBinaryXml(const uint8_t *pXml, size_t xmlLen);

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
	static constexpr size_t resources_arsc_FILE_SIZE_MAX = (4096U * 1024U);
	static constexpr size_t ICON_PNG_FILE_SIZE_MAX = (1024U * 1024U);;

	// Binary XML tags
	static constexpr uint32_t endDocTag	= 0x00100000 | RES_XML_END_NAMESPACE_TYPE;
	static constexpr uint32_t startTag	= 0x00100000 | RES_XML_START_ELEMENT_TYPE;
	static constexpr uint32_t endTag	= 0x00100103 | RES_XML_END_ELEMENT_TYPE;

	// AndroidManifest.xml document
	// NOTE: Using a pointer to prevent delay-load issues.
	unique_ptr<xml_document> manifest_xml;

	// String pools from resource.arsc
	vector<string> valueStringPool;
	vector<string> typeStringPool;
	vector<string> keyStringPool;

	unordered_map<uint32_t, vector<string> > entryMap;

	// Response map
	// - Key: Language ID
	// - Value: String
	typedef unordered_map<uint32_t, vector<string> > response_map_t;

	// Response map (localized)
	// - Key: Resource ID
	// - Value: Map of language IDs to values
	unordered_map<uint32_t, response_map_t> responseMap_i18n;

	static constexpr uint32_t DENSITY_FLAG = (1U << 31);

public:
	/**
	 * Parse a resource ID from the XML.
	 * @param str Resource ID (in format: "@0x12345678")
	 * @return Resource ID, or 0 if not valid
	 */
	static uint32_t parseResourceID(const char *str);

	/**
	 * Add string field data.
	 *
	 * If the string is in the format "@0x12345678", it will be loaded from
	 * resource.arsc if available, with RFT_STRING_MULTI.
	 *
	 * @param name Field name
	 * @param str String
	 * @param flags Formatting flags
	 * @return Field index, or -1 on error.
	 */
	int addField_string_i18n(const char *name, const char *str, unsigned int flags = 0);

	/**
	 * Add string field data.
	 *
	 * If the string is in the format "@0x12345678", it will be loaded from
	 * resource.arsc if available, with RFT_STRING_MULTI.
	 *
	 * @param name Field name
	 * @param str String
	 * @param flags Formatting flags
	 * @return Field index, or -1 on error.
	 */
	int addField_string_i18n(const char *name, const std::string &str, unsigned int flags = 0)
	{
		return addField_string_i18n(name, str.c_str(), flags);
	}

public:
	/**
	 * Load the icon.
	 * @return Icon, or nullptr on error.
	 */
	rp_image_const_ptr loadIcon(void);
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

static inline uint16_t read_u16(const uint8_t *&p)
{
	uint16_t v = p[0] | (p[1] << 8);
	p += sizeof(v);
	return v;
}

static inline uint32_t read_u32(const uint8_t *&p)
{
	uint32_t v = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
	p += sizeof(v);
	return v;
}

/**
 * Process an Android resource string pool.
 * @param data Start of string pool
 * @param size Size of string pool
 * @return Processed string pool, or empty vector<string> on error.
 */
vector<string> AndroidAPKPrivate::processStringPool(const uint8_t *data, size_t size)
{
	vector<string> stringPool;
	const uint8_t *p = data;
	const uint8_t *const pEnd = data + size;

	if (p + sizeof(ResStringPool_header) > pEnd) {
		return {};
	}
	const ResStringPool_header *const pStringPoolHdr = reinterpret_cast<const ResStringPool_header*>(p);
	p += sizeof(ResStringPool_header);

	const bool isUTF8 = (pStringPoolHdr->flags & ResStringPool_header::UTF8_FLAG);

	// Offset table
	const uint32_t *pStrOffsetTbl = reinterpret_cast<const uint32_t*>(p);
	p += (pStringPoolHdr->stringCount * sizeof(uint32_t));
	if (p >= pEnd) {
		return {};
	}

	// Load the strings.
	// NOTE: Assuming UTF-8 for now.
	// NOTE 2: Copying strings into a vector<string> to reduce confusion,
	// though it would be faster and more efficient to directly reference them...
	// TODO: Separate class that does that?
	for (unsigned int i = 0; i < pStringPoolHdr->stringCount; i++) {
		unsigned int pos = pStringPoolHdr->stringsStart + pStrOffsetTbl[i];
		const uint8_t *p_u8str = data + pos;
		if (p_u8str >= pEnd) {
			break;
		}

		if (isUTF8) {
			// TODO: Why the u16len and u8len?
			unsigned int u16len = *p_u8str++;
			if (p_u8str >= pEnd) {
				break;
			}
			if (u16len & 0x80) {
				// Larger than 128
				u16len = ((u16len & 0x7F) << 8) + *p_u8str++;
			}

			unsigned int u8len = *p_u8str++;
			if (p_u8str >= pEnd) {
				break;
			}
			if (u8len & 0x80) {
				// Larger than 128
				u8len = ((u8len & 0x7F) << 8) + *p_u8str++;
			}

			if (u8len > 0) {
				stringPool.emplace_back(reinterpret_cast<const char*>(p_u8str), u8len);
			} else {
				stringPool.push_back(string());
			}
		} else {
			if (p_u8str + 2 > pEnd) {
				break;
			}
			unsigned int u16len = read_u16(p_u8str);
			if (u16len & 0x8000) {
				// Larger than 32,768
				if (p_u8str + 2 > pEnd) {
					break;
				}
				u16len = ((u16len & 0x7FFF) << 16) + read_u16(p_u8str);
			}

			if (u16len > 0) {
				stringPool.emplace_back(utf16le_to_utf8(reinterpret_cast<const char16_t*>(p_u8str), u16len));
			} else {
				stringPool.push_back(string());
			}
		}
	}

	return stringPool;
}

/**
 * Convert an Android locale or country code to rom-properties language and country codes.
 * @param alocale Android locale
 * @param rLC rom-properties language code
 * @param rCC rom-properties country code
 */
void AndroidAPKPrivate::androidLocaleToRP(uint32_t alocale, uint32_t &rLC, uint32_t &rCC)
{
	// The individual elements are stored in big-endian regardless of runtime architecture.
	// TODO: Verify ordering.
	uint32_t lc = be16_to_cpu(alocale & 0xFFFF);
	uint32_t cc = be16_to_cpu(alocale >> 16);

	// Check for the 'packed' encoding.
	if (lc & 0x8000) {
		// lc is packed.
		lc = ((((lc      ) & 0x1F) + 'a') << 16) |
		     ((((lc >>  5) & 0x1F) + 'a') <<  8) |
		      (((lc >> 10) & 0x1F) + 'a');
	}
	if (cc & 0x8000) {
		// cc is packed.
		cc = ((((cc      ) & 0x1F) + 'a') << 16) |
		     ((((cc >>  5) & 0x1F) + 'a') <<  8) |
		      (((cc >> 10) & 0x1F) + 'a');
	}

	if (lc == 'zh') {
		// 'hans' and 'hant' conversion.
		lc = (cc == 'CN') ? 'hans' : 'hant';
	}

	rLC = lc;
	rCC = cc;
}

/**
 * Process RES_TABLE_TYPE_TYPE.
 * @param data Start of type
 * @param size Size of type
 * @param package_id Package ID
 * @return 0 on success; negative POSIX error code on error.
 */
ATTR_ACCESS_SIZE(read_only, 2, 3)
int AndroidAPKPrivate::processType(const uint8_t *data, size_t size, unsigned int package_id)
{
	const uint8_t *const pEnd = data + size;
	if (data + sizeof(ResTable_type) > pEnd) {
		return -EIO;
	}
	const ResTable_type *const pResTableType = reinterpret_cast<const ResTable_type*>(data);

	const uint8_t id = pResTableType->id;
	const unsigned int entryCount = pResTableType->entryCount;

	const uint8_t *p = data + pResTableType->header.headerSize;
	if (p >= pEnd) {
		return -EIO;
	}

	uint32_t lc = 0, cc = 0;
	if (pResTableType->config.locale != 0) {
		// Localized string
		// TODO: Figure out if the icon can be localized or not.

		// Check the locale.
		androidLocaleToRP(pResTableType->config.locale, lc, cc);

		// Do we support this locale?
		if (!SystemRegion::getLocalizedLanguageName(lc)) {
			// Not supported.
			return 0;
		}
	}

	// For the application icon, the locale will be 0 and density non-zero.
	if (lc == 0 && pResTableType->config.density != 0) {
		// We'll indicate this is an icon by setting the high bit.
		lc = DENSITY_FLAG | pResTableType->config.density;
	}

	// Key: resource_id
	// Value: pResValue->data
	unordered_map<uint32_t, uint32_t> refKeys;
	if (pResTableType->header.headerSize + (entryCount * sizeof(uint32_t)) != pResTableType->entriesStart) {
		// Inconsistent headers!
		assert(!"RES_TABLE_TYPE_TYPE header is inconsistent.");
		return -EIO;
	}

	// Entry indexes
	const int32_t *const pIndexTbl = reinterpret_cast<const int32_t*>(p);
	p += (pResTableType->entryCount * sizeof(uint32_t));
	if (p >= pEnd) {
		return -EIO;
	}

	// Get entries
	for (unsigned int i = 0; i < entryCount; i++) {
		if (pIndexTbl[i] == -1) {
			continue;
		}

		uint32_t resource_id = (package_id << 24) | (id << 16) | i;

		if (p + sizeof(ResTable_entry) > pEnd) {
			break;
		}
		const ResTable_entry *const pEntry = reinterpret_cast<const ResTable_entry*>(p);
		p += sizeof(ResTable_entry);
		if (!(pEntry->flags & ResTable_entry::FLAG_COMPLEX)) {
			// Simple case
			if (p + sizeof(Res_value) > pEnd) {
				break;
			}
			const Res_value *const pResValue = reinterpret_cast<const Res_value*>(p);
			p += sizeof(Res_value);
			// TODO: Verify pEntry->key.index
			const string &keyStr = keyStringPool[pEntry->key.index];

			// TODO: Truncate resource_id to the low 16 bits?
			auto iter = entryMap.find(resource_id);
			if (iter != entryMap.end()) {
				// We have a vector<string> for this resource ID already.
				iter->second.push_back(keyStr);
			} else {
				// No vector<string>. Create one.
				vector<string> v_str;
				v_str.push_back(keyStr);
				entryMap.emplace(resource_id, std::move(v_str));
			}

			// Value processing
			string data;
			switch (pResValue->dataType) {
				case Res_value::TYPE_STRING:
					data = valueStringPool[pResValue->data];
					break;
				case Res_value::TYPE_REFERENCE:
					refKeys.emplace(resource_id, pResValue->data);
					break;
				default:
					data = fmt::to_string(pResValue->data);
					break;
			}

			// Check if we already have a map for this resource ID.
			response_map_t *pResponseMap;
			auto iter2 = responseMap_i18n.find(resource_id);
			if (iter2 != responseMap_i18n.end()) {
				// We have a map for this resource ID already.
				pResponseMap = &(iter2->second);
			} else {
				// No resource map. Create one.
				auto emp = responseMap_i18n.emplace(resource_id, response_map_t());
				pResponseMap = &(emp.first->second);
			}

			// Check if we have a vector for this language code.
			vector<string> *pVec;
			auto iter3 = pResponseMap->find(lc);
			if (iter3 != pResponseMap->end()) {
				// We have a vector for this language code already.
				pVec = &(iter3->second);
			} else {
				// No vector for this lc. Create one.
				auto emp = pResponseMap->emplace(lc, vector<string>());
				pVec = &(emp.first->second);
			}

			// Add the string to the vector.
			pVec->push_back(std::move(data));
		} else {
			// Complex case
			// NOTE: Original C# code didn't actually make use of this...
			// FIXME: Replacing the two read_u32() with `p += (4 * 2);` fails???
			const unsigned int entryParent = read_u32(p);
			const unsigned int entryCount = read_u32(p);
			p += (12 * entryCount);
		}
	}

	// TODO: refKeys handling.

	return 0;
}

/**
 * Process an Android resource package.
 * @param data Start of package
 * @param size Size of package
 * @return 0 on success; negative POSIX error code on error.
 */
int AndroidAPKPrivate::processPackage(const uint8_t *data, size_t size)
{
	const uint8_t *const pEnd = data + size;

	if (data + sizeof(ResTable_package) > pEnd) {
		return -EIO;
	}
	const ResTable_package *pPackage = reinterpret_cast<const ResTable_package*>(data);

	// Package string pools
	const ResStringPool_header *const pTypeStrings = reinterpret_cast<const ResStringPool_header*>(data + pPackage->typeStrings);
	typeStringPool = processStringPool(reinterpret_cast<const uint8_t*>(pTypeStrings), size - pPackage->typeStrings);
	const ResStringPool_header *const pKeyStrings = reinterpret_cast<const ResStringPool_header*>(data + pPackage->keyStrings);
	keyStringPool = processStringPool(reinterpret_cast<const uint8_t*>(pKeyStrings), size - pPackage->keyStrings);

	// Iterate through chunks
	unsigned int typeSpecCount = 0;
	unsigned int typeCount = 0;

	const uint8_t *p = reinterpret_cast<const uint8_t*>(pKeyStrings) + pKeyStrings->header.size;

	while (true) {
		if (p + sizeof(ResChunk_header) > pEnd) {
			break;
		}
		const ResChunk_header *const pHdr = reinterpret_cast<const ResChunk_header*>(p);

		switch (pHdr->type) {
			case RES_TABLE_TYPE_SPEC_TYPE:
				// NOTE: processTypeSpec() in the original C# code didn't actually do anything...
				//processTypeSpec(p, pEnd - p);
				typeSpecCount++;
				break;
			case RES_TABLE_TYPE_TYPE:
				// Process the package.
				processType(p, pEnd - p, pPackage->id);
				typeCount++;
				break;
			default:
				// FIXME
				break;
		}

		p += pHdr->size;
	}

	return 0;
}

/**
 * Load Android resource data.
 * @param pArsc		[in] Android resource data
 * @param arscLen	[in] Size of resource data
 * @return 0 on success; negative POSIX error code on error.
 */
int AndroidAPKPrivate::loadResourceAsrc(const uint8_t *pArsc, size_t arscLen)
{
	assert(pArsc != nullptr);
	assert(arscLen > 0);
	if (!pArsc || arscLen == 0) {
		return -EINVAL;
	}

	// Based on: https://github.com/hylander0/Iteedee.ApkReader/blob/master/Iteedee.ApkReader/ApkResourceFinder.cs
	const uint8_t *p = pArsc;
	const uint8_t *const pEnd = pArsc + arscLen;

	if (p + sizeof(ResTable_header) > pEnd) {
		return -EIO;
	}
	const ResTable_header *const pResTableHdr = reinterpret_cast<const ResTable_header*>(p);
	if (pResTableHdr->header.type != RES_TABLE_TYPE || pResTableHdr->header.size != arscLen) {
		// Something is wrong here...
		return -EIO;
	}
	p += pResTableHdr->header.headerSize;

	unsigned int realStringPoolCount = 0;
	unsigned int realPackageCount = 0;

	while (true) {
		if (p + sizeof(ResChunk_header) > pEnd) {
			break;
		}
		const ResChunk_header *const pHdr = reinterpret_cast<const ResChunk_header*>(p);

		switch (pHdr->type) {
			case RES_STRING_POOL_TYPE:
				// Only processing the first string pool.
				if (realStringPoolCount == 0) {
					valueStringPool = processStringPool(p, pHdr->size);
				}
				realStringPoolCount++;
				break;

			case RES_TABLE_PACKAGE_TYPE:
				// Process the package.
				processPackage(p, pHdr->size);
				realPackageCount++;
				break;

			default:
				break;
		}

		p += pHdr->size;
		if (p >= pEnd) {
			break;
		}
	}

	// Verify counts.
	assert(realStringPoolCount == 1);
	assert(realPackageCount == pResTableHdr->packageCount);
	if (realStringPoolCount != 1 || realPackageCount != pResTableHdr->packageCount) {
		return -EIO;
	}
	return 0;
}

/**
 * Get a string from Android resource data.
 * @param id		[in] Resource ID
 * @return String, or nullptr if not found.
 */
const char *AndroidAPKPrivate::getStringFromResource(uint32_t id)
{
	assert(!responseMap_i18n.empty());
	if (responseMap_i18n.empty()) {
		return nullptr;
	}

	// Get the map for the specified resource ID.
	auto iter = responseMap_i18n.find(id);
	if (iter == responseMap_i18n.end()) {
		return nullptr;
	}
	const auto &response_map = iter->second;

	// TODO: Multi-language string handling.
	// For now, using this system:
	// - Use 'en' if available.
	// - Otherwise, use 0.
	// - Otherwise, use the first available.
	response_map_t::const_iterator iter2 = response_map.find('en');
	if (iter2 == response_map.end()) {
		iter2 = response_map.find(0);
		if (iter2 == response_map.end()) {
			iter2 = response_map.cbegin();
		}
	}

	const auto &vec = iter2->second;
	if (vec.empty()) {
		return nullptr;
	}

	// Find the first non-empty string.
	for (const auto &str : vec) {
		if (!str.empty()) {
			return str.c_str();
		}
	}

	// No non-empty strings with this ID were found...
	return nullptr;
}

/**
 * Decompress Android binary XML.
 * Strings that are referencing resources will be printed as "@0x12345678".
 * @param pXml		[in] Android binary XML data
 * @param xmlLen	[in] Size of XML data
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
					// Value is in the resource file.
					// Print the resource ID here so we can handle multi-language lookup later.
					xmlAttr.set_value(fmt::format(FSTR("@0x{:0>8X}"), attrResId));
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

	xml_document xml = decompressAndroidBinaryXml(
		AndroidManifest_xml_buf.data(), AndroidManifest_xml_buf.size());
	if (xml.empty()) {
		// Empty???
		manifest_xml.reset();
		return -EIO;
	}

	// Load resources.arsc.
	// NOTE: We have to load the full file due to .zip limitations.
	// TODO: Figure out the best "max size".
	rp::uvector<uint8_t> resources_arsc_buf = loadFileFromZip(
		"resources.arsc", resources_arsc_FILE_SIZE_MAX);
	if (!resources_arsc_buf.empty()) {
		loadResourceAsrc(resources_arsc_buf.data(), resources_arsc_buf.size());
	}

	manifest_xml.reset(new xml_document);
	*manifest_xml = std::move(xml);
	return 0;
}

/**
 * Parse a resource ID from the XML.
 * @param str Resource ID (in format: "@0x12345678")
 * @return Resource ID, or 0 if not valid
 */
uint32_t AndroidAPKPrivate::parseResourceID(const char *str)
{
	if (str[0] == '@' && str[1] == '0' && str[2] == 'x' && str[3] != '\0') {
		// Convert from hexadecimal.
		char *pEnd = nullptr;
		uint32_t resource_id = strtoul(&str[3], &pEnd, 16);
		if (resource_id != 0 && pEnd && *pEnd == '\0') {
			return resource_id;
		}
	}

	// Not valid.
	return 0;
}

/**
 * Add string field data.
 *
 * If the string is in the format "@0x12345678", it will be loaded from
 * resource.arsc if available, with RFT_STRING_MULTI.
 *
 * @param name Field name
 * @param str String
 * @param flags Formatting flags
 * @return Field index, or -1 on error.
 */
int AndroidAPKPrivate::addField_string_i18n(const char *name, const char *str, unsigned int flags)
{
	// Check if the name is in the format "@0x12345678".
	const uint32_t resource_id = parseResourceID(str);
	if (resource_id != 0) {
		// Resource ID parsed.
		auto iter = responseMap_i18n.find(resource_id);
		if (iter != responseMap_i18n.end() && !iter->second.empty()) {
			// Add the localized strings.
			RomFields::StringMultiMap_t *const pStringMultiMap = new RomFields::StringMultiMap_t;
			const auto &lcmap = iter->second;

			// TODO: Find the first non-empty string in each vector.

			// Get the English string first and use it to de-duplicate other strings.
			const string *s_en = nullptr;
			uint32_t lc_dedupe = 0;
			auto iter_en = lcmap.find('en');
			if (iter_en != lcmap.end()) {
				const auto &vec = iter_en->second;
				if (!vec.empty()) {
					s_en = &vec[0];
					lc_dedupe = 'en';
					pStringMultiMap->emplace(lc_dedupe, vec[0]);
				}
			}
			if (!s_en) {
				iter_en = lcmap.find(0);
				if (iter_en != lcmap.end()) {
					const auto &vec = iter_en->second;
					if (!vec.empty()) {
						s_en = &vec[0];
						lc_dedupe = 0;
						pStringMultiMap->emplace('en', vec[0]);
					}
				}
			}
			if (!s_en) {
				iter_en = lcmap.begin();
				const auto &vec = iter_en->second;
				if (!vec.empty()) {
					s_en = &vec[0];
					lc_dedupe = iter_en->first;
					pStringMultiMap->emplace(lc_dedupe, vec[0]);
				}
			}

			for (auto iter2 : lcmap) {
				// Get the first string from the vector.
				// TODO: What to do with the rest of the strings?
				if (iter2.first == lc_dedupe) {
					continue;
				}
				const auto &vec = iter2.second;
				if (vec.empty()) {
					continue;
				}

				const string &str = vec[0];
				if (s_en && *s_en == str) {
					// Matches 'en'.
					continue;
				}

				// NOTE: Replacing `lc == 0` with 'en'.
				uint32_t lc = iter2.first;
				if (lc == 0) {
					lc = 'en';
				}
				pStringMultiMap->emplace(lc, vec[0]);
			}
			if (pStringMultiMap) {
				// TODO: def_lc?
				fields.addField_string_multi(name, pStringMultiMap, 'en', flags);
				return fields.count();
			} else {
				// No strings...?
				delete pStringMultiMap;
			}
		}
	}

	// Localization failed. Add the string as-is.
	return fields.addField_string(name, str, flags);
}

/**
 * Load the icon.
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr AndroidAPKPrivate::loadIcon(void)
{
	if (img_icon) {
		// Icon has already been loaded.
		return img_icon;
	} else if (!this->isValid) {
		// Can't load the icon.
		return {};
	}

	// Make sure the .apk file is open.
	if (!apkFile) {
		// Not open...
		return {};
	}

	// Get the icon filename from the AndroidManfiest.xml file.
	xml_node manifest_node = manifest_xml->child("manifest");
	if (!manifest_node) {
		// No "<manifest>" node???
		return {};
	}
	xml_node application_node = manifest_node.child("application");
	if (!application_node) {
		return {};
	}
	const char *icon_filename = application_node.attribute("icon").as_string(nullptr);
	if (!icon_filename || icon_filename[0] == '\0') {
		return {};
	}

	// TODO: Lower density on request?
	unsigned int highest_density = 0;
	const uint32_t resource_id = parseResourceID(icon_filename);
	string resIcon;	// FIXME: Copying the pointer doesn't seem to work???
	if (resource_id != 0) {
		// Icon filename has a resource ID.
		// Find the icon with the highest density.
		auto iter = responseMap_i18n.find(resource_id);
		if (iter == responseMap_i18n.end()) {
			return {};
		}
		const auto &lcmap = iter->second;

		icon_filename = nullptr;
		for (auto iter2 : lcmap) {
			if (!(iter2.first & DENSITY_FLAG)) {
				continue;
			}

			const unsigned int density = (iter2.first & ~DENSITY_FLAG);
			if (density > highest_density) {
				const auto &vec = iter2.second;
				// Find the first non-empty entry.
				for (const auto &str : vec) {
					if (!str.empty()) {
						resIcon = str;
						highest_density = density;
						break;
					}
				}
			}
		}
	}
	if (!resIcon.empty()) {
		icon_filename = resIcon.c_str();
	}
	if (!icon_filename) {
		// Unable to load the icon filename...
		return {};
	}

	// Attempt to load the file.
	rp::uvector<uint8_t> icon_buf = loadFileFromZip(icon_filename, ICON_PNG_FILE_SIZE_MAX);
	if (icon_buf.size() < 8) {
		// Unable to load the icon file.
		return {};
	}

	// Check for an Adaptive Icon.
	// The icon file will be a binary XML instead of a PNG image.
	static const array<uint8_t, 4> AndroidBinaryXML_magic = {{0x03, 0x00, 0x08, 0x00}};
	if (!memcmp(icon_buf.data(), AndroidBinaryXML_magic.data(), AndroidBinaryXML_magic.size())) {
		// TODO: Handle adaptive icons.
		return {};
	}

	// Create a MemFile and decode the image.
	// TODO: For rpcli, shortcut to extract the PNG directly.
	MemFile f_mem(icon_buf.data(), icon_buf.size());
	this->img_icon = RpPng::load(&f_mem);
	return this->img_icon;
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
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t AndroidAPK::supportedImageTypes(void) const
{
	return IMGBF_INT_ICON;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> AndroidAPK::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_INT_ICON:
			// TODO: Get the actual image size.
			return {{nullptr, 64, 64, 0}};
		default:
			break;
	}

	// Unsupported image type.
	return {};
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> AndroidAPK::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);
	//RP_D(const AndroidAPK);

	switch (imageType) {
		case IMG_INT_ICON:
			// TODO: Get the actual image size.
			return {{nullptr, 64, 64, 0}};
		default:
			break;
	}

	// Unsupported image type.
	return {};
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
			d->addField_string_i18n(C_("AndroidAPK", "Title"), label);
		}

		const char *const name = application_node.attribute("name").as_string(nullptr);
		if (name && name[0] != '\0') {
			d->addField_string_i18n(C_("AndroidAPK", "Package Name"), name);
		}

		const char *const description = application_node.attribute("description").as_string(nullptr);
		if (description && description[0] != '\0') {
			d->addField_string_i18n(C_("AndroidAPK", "Description"), description);
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

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int AndroidAPK::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(AndroidAPK);

	ROMDATA_loadInternalImage_single(
		IMG_INT_ICON,	// ourImageType
		d->file,	// file
		d->isValid,	// isValid
		0,		// romType
		d->img_icon,	// imgCache
		d->loadIcon);	// func
}

} // namespace LibRomData
