/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AndroidResourceReader.cpp: Android resource reader.                     *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"
#include "AndroidResourceReader.hpp"
#include "../Handheld/android_apk_structs.h"

// Other rom-properties libraries
#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;
using std::unordered_map;
using std::vector;

namespace LibRomData {

class AndroidResourceReaderPrivate
{
public:
	AndroidResourceReaderPrivate(const uint8_t *pArsc, size_t arscLen);

private:
	RP_DISABLE_COPY(AndroidResourceReaderPrivate)

public:
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
	int processType(const uint8_t *data, size_t size, uint32_t package_id);

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

public:
	const uint8_t *pArsc;
	size_t arscLen;

	// String pools from resource.arsc
	AndroidResourceReader::StringPoolAccessor valueStringPool;
	AndroidResourceReader::StringPoolAccessor typeStringPool;
	AndroidResourceReader::StringPoolAccessor keyStringPool;

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
};

static inline uint16_t read_u16(const uint8_t *&p)
{
	const uint16_t *const p16 = reinterpret_cast<const uint16_t*>(p);
	const uint16_t v = *p16;
	p += sizeof(v);
	return le16_to_cpu(v);
}

static inline uint32_t read_u32(const uint8_t *&p)
{
	const uint32_t *const p32 = reinterpret_cast<const uint32_t*>(p);
	const uint32_t v = *p32;
	p += sizeof(v);
	return le32_to_cpu(v);
}

/** StringPoolAccessor **/

/**
 * String Pool accessor class
 *
 * NOTE: The specified memory buffer *must* remain valid while this
 * StringPoolAccessor is open.
 *
 * @param data Pointer to the string pool
 * @param size Size of the string pool
 */
AndroidResourceReader::StringPoolAccessor::StringPoolAccessor(const uint8_t *data, size_t size)
	: pEnd(data + size)
	, pStrOffsetTbl(nullptr)
	, pStringsStart(nullptr)
	, stringCount(0)
	, isUTF8(false)
{
	assert(size > sizeof(ResStringPool_header));
	if (size <= sizeof(ResStringPool_header)) {
		// Too small...
		return;
	}

	const ResStringPool_header *const pStringPoolHdr = reinterpret_cast<const ResStringPool_header*>(data);
	stringCount = le32_to_cpu(pStringPoolHdr->stringCount);
	isUTF8 = (le32_to_cpu(pStringPoolHdr->flags) & ResStringPool_header::UTF8_FLAG);
	pStringsStart = data + le32_to_cpu(pStringPoolHdr->stringsStart);

	// Offset table
	pStrOffsetTbl = reinterpret_cast<const uint32_t*>(data + sizeof(ResStringPool_header));
	assert(pStrOffsetTbl + stringCount < reinterpret_cast<const uint32_t*>(pEnd));
	if (pStrOffsetTbl + stringCount >= reinterpret_cast<const uint32_t*>(pEnd)) {
		stringCount = 0;
		return;
	}
}

/**
 * Get a string from the string pool.
 *
 * NOTE: The string pool might be either UTF-8 or UTF-16LE.
 * This function will automatically convert it to UTF-8 if necessary.
 *
 * @param index String index
 * @return String, or empty string on error.
 */
string AndroidResourceReader::StringPoolAccessor::getString(unsigned int index)
{
	// TODO: Cache converted strings?
	assert(index < stringCount);
	if (index >= stringCount) {
		return {};
	}

	const uint8_t *p_u8str = pStringsStart + le32_to_cpu(pStrOffsetTbl[index]);
	assert(p_u8str < pEnd);
	if (p_u8str >= pEnd) {
		return {};
	}

	if (isUTF8) {
		// TODO: Why the u16len and u8len?
		unsigned int u16len = *p_u8str++;
		assert(p_u8str < pEnd);
		if (p_u8str >= pEnd) {
			return {};
		}
		if (u16len & 0x80) {
			// >= 128
			u16len = ((u16len & 0x7F) << 8) + *p_u8str++;
			assert(p_u8str < pEnd);
			if (p_u8str >= pEnd) {
				return {};
			}
		}

		unsigned int u8len = *p_u8str++;
		assert(p_u8str < pEnd);
		if (p_u8str >= pEnd) {
			return {};
		}
		if (u8len & 0x80) {
			// >= 128
			u8len = ((u8len & 0x7F) << 8) + *p_u8str++;
			assert(p_u8str < pEnd);
			if (p_u8str >= pEnd) {
				return {};
			}
		}

		if (u8len > 0) {
			return string(reinterpret_cast<const char*>(p_u8str), u8len);
		}
	} else {
		assert(p_u8str + 2 <= pEnd);
		if (p_u8str + 2 > pEnd) {
			return {};
		}
		unsigned int u16len = read_u16(p_u8str);
		if (u16len & 0x8000) {
			// >= 32,768
			assert(p_u8str + 2 <= pEnd);
			if (p_u8str + 2 > pEnd) {
				return {};
			}
			u16len = ((u16len & 0x7FFF) << 16) + read_u16(p_u8str);
		}

		if (u16len > 0) {
			return utf16le_to_utf8(reinterpret_cast<const char16_t*>(p_u8str), u16len);
		}
	}

	// Empty string?
	return {};
}

/** AndroidResourceReaderPrivate **/

AndroidResourceReaderPrivate::AndroidResourceReaderPrivate(const uint8_t *pArsc, size_t arscLen)
	: pArsc(pArsc)
	, arscLen(arscLen)
{
	assert(pArsc != nullptr);
	assert(arscLen > 0);
	if (!pArsc || arscLen == 0) {
		// No resources...
		return;
	}

	// Load the resources.
	// TODO: On demand?
	int ret = loadResourceAsrc(pArsc, arscLen);
	if (ret != 0) {
		// An error occurred...
		this->pArsc = nullptr;
		this->arscLen = 0;
		responseMap_i18n.clear();
	}
}

/**
 * Convert an Android locale or country code to rom-properties language and country codes.
 * @param alocale Android locale
 * @param rLC rom-properties language code
 * @param rCC rom-properties country code
 */
void AndroidResourceReaderPrivate::androidLocaleToRP(uint32_t alocale, uint32_t &rLC, uint32_t &rCC)
{
	// The individual elements are stored in big-endian regardless of runtime architecture.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	uint32_t lc = be16_to_cpu(alocale & 0xFFFF);
	uint32_t cc = be16_to_cpu(alocale >> 16);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	uint32_t lc = be16_to_cpu(alocale >> 16);
	uint32_t cc = be16_to_cpu(alocale & 0xFFFF);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

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
int AndroidResourceReaderPrivate::processType(const uint8_t *data, size_t size, uint32_t package_id)
{
	const uint8_t *const pEnd = data + size;
	assert(data + sizeof(ResTable_type) <= pEnd);
	if (data + sizeof(ResTable_type) > pEnd) {
		return -EIO;
	}
	const ResTable_type *const pResTableType = reinterpret_cast<const ResTable_type*>(data);

	const uint8_t id = pResTableType->id;
	const uint32_t entryCount = le32_to_cpu(pResTableType->entryCount);

	const uint16_t resTable_headerSize = le16_to_cpu(pResTableType->header.headerSize);
	const uint8_t *p = data + resTable_headerSize;
	assert(p < pEnd);
	if (p >= pEnd) {
		return -EIO;
	}

	uint32_t lc = 0, cc = 0;
	if (pResTableType->config.locale != 0) {
		// Localized string
		// TODO: Figure out if the icon can be localized or not.

		// Check the locale. (NOTE: Stored in BE, though LC/CC may change position...)
		androidLocaleToRP(pResTableType->config.locale, lc, cc);

		// Do we support this locale?
		if (!SystemRegion::getLocalizedLanguageName(lc)) {
			// Not supported.
			return 0;
		}
	}

	// For the application icon, the locale will be 0 and density non-zero.
	const uint16_t config_density = le16_to_cpu(pResTableType->config.density);
	if (lc == 0 && config_density != 0) {
		// We'll indicate this is an icon by setting the high bit.
		lc = DENSITY_FLAG | config_density;
	}

	// Key: resource_id
	// Value: pResValue->data
	unordered_map<uint32_t, uint32_t> refKeys;
	if (resTable_headerSize + (entryCount * sizeof(uint32_t)) != le32_to_cpu(pResTableType->entriesStart)) {
		// Inconsistent headers!
		assert(!"RES_TABLE_TYPE_TYPE header is inconsistent.");
		return -EIO;
	}

	// Entry indexes
	const int32_t *const pIndexTbl = reinterpret_cast<const int32_t*>(p);
	p += (entryCount * sizeof(uint32_t));
	assert(p < pEnd);
	if (p >= pEnd) {
		return -EIO;
	}

	// Get entries
	for (unsigned int i = 0; i < entryCount; i++) {
		if (pIndexTbl[i] == cpu_to_le32(-1)) {
			continue;
		}

		uint32_t resource_id = (package_id << 24) | (id << 16) | i;

		assert(p + sizeof(ResTable_entry) <= pEnd);
		if (p + sizeof(ResTable_entry) > pEnd) {
			break;
		}
		const ResTable_entry *const pEntry = reinterpret_cast<const ResTable_entry*>(p);
		p += sizeof(ResTable_entry);
		if (!(le16_to_cpu(pEntry->flags) & ResTable_entry::FLAG_COMPLEX)) {
			// Simple case
			assert(p + sizeof(Res_value) <= pEnd);
			if (p + sizeof(Res_value) > pEnd) {
				break;
			}
			const Res_value *const pResValue = reinterpret_cast<const Res_value*>(p);
			p += sizeof(Res_value);
			const uint32_t key_index = le32_to_cpu(pEntry->key.index);
			const string keyStr = keyStringPool.getString(key_index);

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
			const uint32_t u32data = le32_to_cpu(pResValue->data);
			string data;
			switch (pResValue->dataType) {
				case Res_value::TYPE_STRING:
					data = valueStringPool.getString(u32data);
					break;
				case Res_value::TYPE_REFERENCE:
					refKeys.emplace(resource_id, u32data);
					break;
				default:
					data = fmt::to_string(u32data);
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
int AndroidResourceReaderPrivate::processPackage(const uint8_t *data, size_t size)
{
	const uint8_t *const pEnd = data + size;

	assert(data + sizeof(ResTable_package) <= pEnd);
	if (data + sizeof(ResTable_package) > pEnd) {
		return -EIO;
	}
	const ResTable_package *pPackage = reinterpret_cast<const ResTable_package*>(data);
	const uint32_t pPackage_typeStrings = le32_to_cpu(pPackage->typeStrings);
	const uint32_t pPackage_keyStrings = le32_to_cpu(pPackage->keyStrings);

	// Package string pools
	const ResStringPool_header *const pTypeStrings = reinterpret_cast<const ResStringPool_header*>(data + pPackage_typeStrings);
	typeStringPool = AndroidResourceReader::StringPoolAccessor(reinterpret_cast<const uint8_t*>(pTypeStrings), size - pPackage_typeStrings);
	const ResStringPool_header *const pKeyStrings = reinterpret_cast<const ResStringPool_header*>(data + pPackage_keyStrings);
	keyStringPool = AndroidResourceReader::StringPoolAccessor(reinterpret_cast<const uint8_t*>(pKeyStrings), size - pPackage_keyStrings);

	// Iterate through chunks
	unsigned int typeSpecCount = 0;
	unsigned int typeCount = 0;

	const uint8_t *p = reinterpret_cast<const uint8_t*>(pKeyStrings) + le32_to_cpu(pKeyStrings->header.size);

	while (p != pEnd) {
		assert(p + sizeof(ResChunk_header) <= pEnd);
		if (p + sizeof(ResChunk_header) > pEnd) {
			break;
		}
		const ResChunk_header *const pHdr = reinterpret_cast<const ResChunk_header*>(p);

		switch (le16_to_cpu(pHdr->type)) {
			case RES_TABLE_TYPE_SPEC_TYPE:
				// NOTE: processTypeSpec() in the original C# code didn't actually do anything...
				//processTypeSpec(p, pEnd - p);
				typeSpecCount++;
				break;
			case RES_TABLE_TYPE_TYPE:
				// Process the package.
				processType(p, pEnd - p, le32_to_cpu(pPackage->id));
				typeCount++;
				break;
			default:
				// FIXME
				break;
		}

		p += le32_to_cpu(pHdr->size);
		assert(p <= pEnd);
	}

	return 0;
}

/**
 * Load Android resource data.
 * @param pArsc		[in] Android resource data
 * @param arscLen	[in] Size of resource data
 * @return 0 on success; negative POSIX error code on error.
 */
int AndroidResourceReaderPrivate::loadResourceAsrc(const uint8_t *pArsc, size_t arscLen)
{
	assert(pArsc != nullptr);
	assert(arscLen > 0);
	if (!pArsc || arscLen == 0) {
		return -EINVAL;
	}

	// Based on: https://github.com/hylander0/Iteedee.ApkReader/blob/master/Iteedee.ApkReader/ApkResourceFinder.cs
	const uint8_t *p = pArsc;
	const uint8_t *const pEnd = pArsc + arscLen;

	assert(p + sizeof(ResTable_header) <= pEnd);
	if (p + sizeof(ResTable_header) > pEnd) {
		return -EIO;
	}
	const ResTable_header *const pResTableHdr = reinterpret_cast<const ResTable_header*>(p);
	assert(pResTableHdr->header.type == cpu_to_le16(RES_TABLE_TYPE));
	assert(le32_to_cpu(pResTableHdr->header.size) == arscLen);
	if (pResTableHdr->header.type != cpu_to_le16(RES_TABLE_TYPE) || le32_to_cpu(pResTableHdr->header.size) != arscLen) {
		// Something is wrong here...
		return -EIO;
	}
	p += le16_to_cpu(pResTableHdr->header.headerSize);

	unsigned int realStringPoolCount = 0;
	unsigned int realPackageCount = 0;

	while (true) {
		assert(p + sizeof(ResChunk_header) <= pEnd);
		if (p + sizeof(ResChunk_header) > pEnd) {
			break;
		}
		const ResChunk_header *const pHdr = reinterpret_cast<const ResChunk_header*>(p);

		const uint32_t pHdr_size = le32_to_cpu(pHdr->size);
		switch (le16_to_cpu(pHdr->type)) {
			case RES_STRING_POOL_TYPE:
				// Only processing the first string pool.
				if (realStringPoolCount == 0) {
					valueStringPool = AndroidResourceReader::StringPoolAccessor(p, pHdr_size);
				}
				realStringPoolCount++;
				break;

			case RES_TABLE_PACKAGE_TYPE:
				// Process the package.
				processPackage(p, pHdr_size);
				realPackageCount++;
				break;

			default:
				break;
		}

		p += pHdr_size;
		if (p >= pEnd) {
			break;
		}
	}

	// Verify counts.
	assert(realStringPoolCount == 1);
	assert(realPackageCount == le32_to_cpu(pResTableHdr->packageCount));
	if (realStringPoolCount != 1 || realPackageCount != le32_to_cpu(pResTableHdr->packageCount)) {
		return -EIO;
	}
	return 0;
}

/** AndroidResourceReader **/

/**
 * Construct an AndroidResourceReader with the specified IRpFile.
 *
 * NOTE: The specified memory buffer *must* remain valid while this
 * AndroidResourceReader is open.
 *
 * @param pArsc		[in] resources.arsc
 * @param arscLen	[in] Size of pArsc
 */
AndroidResourceReader::AndroidResourceReader(const uint8_t *pArsc, size_t arscLen)
	: d_ptr(new AndroidResourceReaderPrivate(pArsc, arscLen))
{ }

AndroidResourceReader::~AndroidResourceReader()
{
	delete d_ptr;
}

/**
 * Is the resource data valid?
 * @return True if valid; false if not.
 */
bool AndroidResourceReader::isValid(void) const
{
	RP_D(const AndroidResourceReader);
	return (d->pArsc != nullptr);
}

/**
 * Parse a resource ID from AndroidManifest.xml, as loaded by the AndroidManifestXML class.
 * @param str Resource ID (in format: "@0x12345678")
 * @return Resource ID, or 0 if not valid
 */
uint32_t AndroidResourceReader::parseResourceID(const char *str)
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
 * Get a string from Android resource data.
 * @param id	[in] Resource ID
 * @return String, or nullptr if not found.
 */
const char *AndroidResourceReader::getStringFromResource(uint32_t id) const
{
	RP_D(const AndroidResourceReader);
	assert(!d->responseMap_i18n.empty());
	if (d->responseMap_i18n.empty()) {
		return nullptr;
	}

	// Get the map for the specified resource ID.
	auto iter = d->responseMap_i18n.find(id);
	if (iter == d->responseMap_i18n.end()) {
		return nullptr;
	}
	const auto &response_map = iter->second;

	// TODO: Multi-language string handling.
	// For now, using this system:
	// - Use 'en' if available.
	// - Otherwise, use 0.
	// - Otherwise, use the first available.
	AndroidResourceReaderPrivate::response_map_t::const_iterator iter2 = response_map.find('en');
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
 * Get a string from Android resource data.
 * @param s_id	[in] Resource ID (as a string in the format "@0x12345678")
 * @return String, or "id" if not found.
 */
const char *AndroidResourceReader::getStringFromResource(const char *s_id) const
{
	const uint32_t id = parseResourceID(s_id);
	if (id != 0) {
		const char *s_ret = getStringFromResource(id);
		if (s_ret) {
			return s_ret;
		}
	}

	// Either s_id is not a resource ID, or the resource was not found.
	return s_id;
}

/**
 * Add string field data to the specified RomFields object.
 *
 * If the string is in the format "@0x12345678", it will be loaded from
 * resource.arsc, with RFT_STRING_MULTI.
 *
 * @param fields RomFields
 * @param name Field name
 * @param str String
 * @param flags Formatting flags
 * @return Field index, or -1 on error.
 */
int AndroidResourceReader::addField_string_i18n(RomFields *fields, const char *name, const char *str, unsigned int flags) const
{
	RP_D(const AndroidResourceReader);
	assert(!d->responseMap_i18n.empty());
	if (d->responseMap_i18n.empty()) {
		// No resources. Add the string directly.
		fields->addField_string(name, str, flags);
		return 0;
	}

	// Check if the name is in the format "@0x12345678".
	const uint32_t resource_id = AndroidResourceReader::parseResourceID(str);
	if (resource_id == 0) {
		// Not a resource. Add the string directly.
		fields->addField_string(name, str, flags);
		return 0;
	}

	// Resource ID parsed.
	auto iter = d->responseMap_i18n.find(resource_id);
	if (iter == d->responseMap_i18n.end() || iter->second.empty()) {
		// Resource ID not found, or has an empty map.
		// Add the string directly.
		fields->addField_string(name, str, flags);
		return 0;
	}

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
		return fields->addField_string_multi(name, pStringMultiMap, 'en', flags);
	}

	// No strings...?
	// Add the string directly...
	delete pStringMultiMap;
	return fields->addField_string(name, str, flags);
}

/**
 * Find an icon filename with the highest density.
 * @param resource_id Resource ID
 * @return Icon filename, or empty string if not found.
 */
string AndroidResourceReader::findIconHighestDensity(uint32_t resource_id) const
{
	RP_D(const AndroidResourceReader);
	assert(!d->responseMap_i18n.empty());
	if (d->responseMap_i18n.empty()) {
		// No resources.
		return {};
	}

	auto iter = d->responseMap_i18n.find(resource_id);
	if (iter == d->responseMap_i18n.end()) {
		return {};
	}
	const auto &lcmap = iter->second;

	int highest_density = -1;
	string icon_filename;	// FIXME: Using `const char*` results in an invalid string here...
	for (auto iter2 : lcmap) {
		// NOTE: Some packages, e.g. Magisk, don't have the density flag set.
		// Allow this if the lc is 0.
		if (!(iter2.first & d->DENSITY_FLAG) && iter2.first != 0) {
			continue;
		}

		const int density = static_cast<int>(iter2.first & ~d->DENSITY_FLAG);
		if (density > highest_density) {
			const auto &vec = iter2.second;
			// Find the first non-empty entry that ends in ".png" or ".webp".
			// NOTE: Skipping ".xml" for now due to not supporting SVG.
			for (const auto &str : vec) {
				if (str.size() > 4) {
					if ((str.compare(str.size()-4, 4, ".png") == 0) /*||
					    (str.compare(str.size()-4, 4, ".xml") == 0)*/) {
						icon_filename = str;
						highest_density = density;
						break;
					}
				}
#ifdef ENABLE_WEBP
				if (str.size() > 5 && str.compare(str.size()-5, 5, ".webp") == 0) {
					icon_filename = str;
					highest_density = density;
					break;
				}
#endif /* ENABLE_WEBP */
			}
		}
	}

	return icon_filename;
}

}
