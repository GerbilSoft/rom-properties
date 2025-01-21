/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox360_XDBF.cpp: Microsoft Xbox 360 game resource reader.              *
 * Handles XDBF files and sections.                                        *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Xbox360_XDBF.hpp"
#include "xbox360_xdbf_structs.h"
#include "data/XboxLanguage.hpp"

// Other rom-properties libraries
#include "librpbase/img/RpPng.hpp"
#include "librpfile/MemFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// C++ STL classes
using std::array;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

namespace LibRomData {

// Workaround for RP_D() expecting the no-underscore naming convention.
#define Xbox360_XDBFPrivate Xbox360_XDBF_Private

class Xbox360_XDBF_Private final : public RomDataPrivate
{
public:
	Xbox360_XDBF_Private(const IRpFilePtr &file, bool xex);
	~Xbox360_XDBF_Private() final;

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(Xbox360_XDBF_Private)

public:
	/** RomDataInfo **/
	static const array<const char*, 3+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// XDBF type
	enum class XDBFType {
		Unknown	= -1,	// Unknown XDBF type.

		SPA	= 0,	// XEX resource
		GPD	= 1,	// Game Profile Data

		Max
	};
	XDBFType xdbfType;

	// Internal icon
	// Points to an rp_image within map_images.
	rp_image_const_ptr img_icon;

	// Loaded images.
	// - Key: resource_id
	// - Value: rp_image*
	unordered_map<uint64_t, rp_image_ptr > map_images;

public:
	// XDBF header.
	XDBF_Header xdbfHeader;

	// Entry table.
	// NOTE: Data is *not* byteswapped on load.
	rp::uvector<XDBF_Entry> entryTable;

	// Data start offset within the file.
	uint32_t data_offset;

	// Cached language ID.
	XDBF_Language_e m_langID;

	// String table indexes.
	// These are indexes into entryTable that indicate
	// where a language table entry is located.
	// If -1, the string table is not present.
	// Note that we're using 16-bit indexes instead of pointers
	// in order to save memory. Also, there shouldn't be more
	// than 32,767 entries in the table.
	array<int16_t, XDBF_LANGUAGE_MAX> strTblIndexes;

	// String tables.
	// NOTE: These are *pointers* to rp::uvector<>.
	array<rp::uvector<char>*, XDBF_LANGUAGE_MAX> strTbls;

	// If true, this XDBF section is in an XEX executable.
	// Some fields shouldn't be displayed.
	bool xex;

	/**
	 * Find a resource in the entry table.
	 * @param namespace_id Namespace ID.
	 * @param resource_id Resource ID.
	 * @return XDBF_Entry*, or nullptr if not found.
	 */
	const XDBF_Entry *findResource(uint16_t namespace_id, uint64_t resource_id) const;

	/**
	 * Determine what languages are available.
	 * This initializes the strTblIndexes array.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int initStrTblIndexes(void);

private:
	/**
	 * Load a string table. (SPA only)
	 * @param langID Language ID.
	 * @return Pointer to string table on success; nullptr on error.
	 */
	const rp::uvector<char> *loadStringTable_SPA(XDBF_Language_e langID);

public:
	/**
	 * Get a string from a string table. (SPA)
	 * @param langID Language ID.
	 * @param string_id String ID.
	 * @return String, or empty string on error.
	 */
	string loadString_SPA(XDBF_Language_e langID, uint16_t string_id);

	/**
	 * Get a string from the resource table. (GPD)
	 * @param string_id String ID.
	 * @return String, or empty string on error.
	 */
	string loadString_GPD(uint16_t string_id);

	/**
	 * Get the language ID to use for the title fields.
	 * @return XDBF language ID.
	 */
	XDBF_Language_e getLanguageID(void) const;

	/**
	 * Get the default language code for the multi-string fields.
	 * @return Language code, e.g. 'en' or 'es'.
	 */
	inline uint32_t getDefaultLC(void) const;

	/**
	 * Load an image resource.
	 * @param image_id Image ID.
	 * @return Decoded image, or nullptr on error.
	 */
	rp_image_const_ptr loadImage(uint64_t image_id);

	/**
	 * Load the main title icon.
	 * @return Icon, or nullptr on error.
	 */
	rp_image_const_ptr loadIcon(void);

public:
	/**
	 * Get the title type as a string.
	 * @return Title type, or nullptr if not found.
	 */
	const char *getTitleType(void) const;

private:
	/**
	 * Add string fields. (SPA)
	 * @param fields RomFields*
	 * @return 0 on success; non-zero on error.
	 */
	int addFields_strings_SPA(RomFields *fields) const;

	/**
	 * Add the Achievements RFT_LISTDATA field. (SPA)
	 * @return 0 on success; non-zero on error.
	 */
	int addFields_achievements_SPA(void);

	/**
	 * Add the Avatar Awards RFT_LISTDATA field. (SPA)
	 * @return 0 on success; non-zero on error.
	 */
	int addFields_avatarAwards_SPA(void);

private:
	/**
	 * Add string fields. (GPD)
	 * @param fields RomFields*
	 * @return 0 on success; non-zero on error.
	 */
	int addFields_strings_GPD(RomFields *fields) const;

	/**
	 * Add the Achievements RFT_LISTDATA field. (GPD)
	 * @return 0 on success; non-zero on error.
	 */
	int addFields_achievements_GPD(void);

public:
	/**
	 * Add string fields. (SPA)
	 * @param fields RomFields*
	 * @return 0 on success; non-zero on error.
	 */
	inline int addFields_strings(RomFields *fields) const
	{
		int ret = 0;
		switch (xdbfType) {
			case XDBFType::SPA:
				ret = addFields_strings_SPA(fields);
				break;
			case XDBFType::GPD:
				ret = addFields_strings_GPD(fields);
				break;
			default:
				break;
		}
		return ret;
	}

	/**
	 * Add the Achievements RFT_LISTDATA field. (SPA)
	 * @return 0 on success; non-zero on error.
	 */
	inline int addFields_achievements(void)
	{
		int ret = 0;
		switch (xdbfType) {
			case XDBFType::SPA:
				ret = addFields_achievements_SPA();
				break;
			case XDBFType::GPD:
				ret = addFields_achievements_GPD();
				break;
			default:
				break;
		}
		return ret;
	}

	/**
	 * Add the Avatar Awards RFT_LISTDATA field. (SPA)
	 * @return 0 on success; non-zero on error.
	 */
	inline int addFields_avatarAwards(void)
	{
		if (xdbfType == XDBFType::SPA) {
			return addFields_avatarAwards_SPA();
		}
		// TODO: Find a GPD file with avatar awards.
		return 0;
	}
};

ROMDATA_IMPL(Xbox360_XDBF)
ROMDATA_IMPL_IMG_TYPES(Xbox360_XDBF)
ROMDATA_IMPL_IMG_SIZES(Xbox360_XDBF)

/** Xbox360_XDBF_Private **/

/* RomDataInfo */
// NOTE: Using the same image settings as Xbox360_XEX.
const array<const char*, 3+1> Xbox360_XDBF_Private::exts = {{
	".xdbf",
	".spa",		// XEX XDBF files
	".gpd",		// Gamer Profile Data

	nullptr
}};
const array<const char*, 1+1> Xbox360_XDBF_Private::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-xbox360-xdbf",

	nullptr
}};
const RomDataInfo Xbox360_XDBF_Private::romDataInfo = {
	"Xbox360_XEX", exts.data(), mimeTypes.data()
};

Xbox360_XDBF_Private::Xbox360_XDBF_Private(const IRpFilePtr &file, bool xex)
	: super(file, &romDataInfo)
	, xdbfType(XDBFType::Unknown)
	, data_offset(0)
	, m_langID(XDBF_LANGUAGE_UNKNOWN)
	, xex(xex)
{
	// Clear the header.
	memset(&xdbfHeader, 0, sizeof(xdbfHeader));

	// Clear the string table pointers and indexes.
	strTbls.fill(nullptr);
	strTblIndexes.fill(0);
}

Xbox360_XDBF_Private::~Xbox360_XDBF_Private()
{
	// Delete any allocated string tables.
	for (rp::uvector<char> *pStrTbl : strTbls) {
		delete pStrTbl;
	}
}

/**
 * Find a resource in the entry table.
 * @param namespace_id Namespace ID.
 * @param resource_id Resource ID.
 * @return XDBF_Entry*, or nullptr if not found.
 */
const XDBF_Entry *Xbox360_XDBF_Private::findResource(uint16_t namespace_id, uint64_t resource_id) const
{
	if (entryTable.empty()) {
		// Entry table isn't loaded...
		return nullptr;
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the IDs to make it easier to find things.
	namespace_id = cpu_to_be16(namespace_id);
	resource_id  = cpu_to_be64(resource_id);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	auto iter = std::find_if(entryTable.cbegin(), entryTable.cend(),
		[namespace_id, resource_id](const XDBF_Entry &p) noexcept -> bool {
			return (p.namespace_id == namespace_id &&
				p.resource_id == resource_id);
		}
	);

	return (iter != entryTable.cend() ? &(*iter) : nullptr);
}

/**
 * Determine what languages are available. (SPA only)
 * This initializes the strTblIndexes array.
 * @return 0 on success; negative POSIX error code on error.
 */
int Xbox360_XDBF_Private::initStrTblIndexes(void)
{
	// Clear the array first.
	strTblIndexes.fill(-1);

	if (xdbfType != XDBFType::SPA) {
		// Not SPA. No string tables.
		return 0;
	}

	if (entryTable.empty()) {
		// Entry table isn't loaded...
		return -EIO;
	}

	// Go through the entry table.
	unsigned int total = 0;
	int16_t idx = 0;
	const auto entryTable_cend = entryTable.cend();
	for (auto iter = entryTable.cbegin();
	     iter != entryTable_cend && total < XDBF_LANGUAGE_MAX; ++iter, idx++)
	{
		if (iter->namespace_id != cpu_to_be16(XDBF_SPA_NAMESPACE_STRING_TABLE))
			continue;

		// Found a string table.
		const uint64_t langID_64 = be64_to_cpu(iter->resource_id);
		assert(langID_64 < XDBF_LANGUAGE_MAX);
		if (langID_64 >= XDBF_LANGUAGE_MAX)
			continue;

		const int16_t langID = static_cast<int16_t>(langID_64);
		assert(strTblIndexes[langID] < 0);
		if (strTblIndexes[langID] < 0) {
			// Found the language. (Assuming only one string table per language.)
			strTblIndexes[langID] = idx;
			total++;
		}
	}

	return (total > 0 ? 0 : -ENOENT);
}

/**
 * Load a string table. (SPA only)
 * @param langID Language ID.
 * @return Pointer to string table on success; nullptr on error.
 */
const rp::uvector<char> *Xbox360_XDBF_Private::loadStringTable_SPA(XDBF_Language_e langID)
{
	assert(langID >= 0);
	assert(langID < XDBF_LANGUAGE_MAX);
	// TODO: Do any games have string tables with language ID XDBF_LANGUAGE_UNKNOWN?
	if (langID <= XDBF_LANGUAGE_UNKNOWN || langID >= XDBF_LANGUAGE_MAX)
		return nullptr;

	// Is the string table already loaded?
	if (this->strTbls[langID]) {
		return this->strTbls[langID];
	}

	// Can we load the string table?
	if (!file || !isValid) {
		// Can't load the string table.
		return nullptr;
	}

	// String table index should already be loaded.
	const int16_t idx = strTblIndexes[langID];
	assert(idx >= 0);
	assert(idx < (uint16_t)entryTable.size());
	if (idx < 0 || idx >= (uint16_t)entryTable.size()) {
		// Out of range...
		return nullptr;
	}

	const XDBF_Entry *const entry = &entryTable[idx];

	// Allocate memory and load the string table.
	const unsigned int str_tbl_sz = be32_to_cpu(entry->length);
	// Sanity check:
	// - Size must be larger than sizeof(XDBF_XSTR_Header)
	// - Size must be a maximum of 1 MB.
	assert(str_tbl_sz > sizeof(XDBF_XSTR_Header));
	assert(str_tbl_sz <= 1024*1024);
	if (str_tbl_sz <= sizeof(XDBF_XSTR_Header) || str_tbl_sz > 1024*1024) {
		// Size is out of range.
		return nullptr;
	}
	rp::uvector<char> *vec = new rp::uvector<char>(str_tbl_sz);

	const unsigned int str_tbl_addr = be32_to_cpu(entry->offset) + this->data_offset;
	size_t size = file->seekAndRead(str_tbl_addr, vec->data(), str_tbl_sz);
	if (size != str_tbl_sz) {
		// Seek and/or read error.
		delete vec;
		return nullptr;
	}

	// Validate the string table magic.
	const XDBF_XSTR_Header *const tblHdr =
		reinterpret_cast<const XDBF_XSTR_Header*>(vec->data());
	if (tblHdr->magic != cpu_to_be32(XDBF_XSTR_MAGIC) ||
	    tblHdr->version != cpu_to_be32(XDBF_XSTR_VERSION))
	{
		// Magic is invalid.
		// TODO: Report an error?
		delete vec;
		return nullptr;
	}

	// String table loaded successfully.
	this->strTbls[langID] = vec;
	return vec;
}

/**
 * Get a string from a string table. (SPA)
 * @param langID Language ID.
 * @param string_id String ID.
 * @return String, or empty string on error.
 */
string Xbox360_XDBF_Private::loadString_SPA(XDBF_Language_e langID, uint16_t string_id)
{
	string ret;

	assert(langID >= 0);
	assert(langID < XDBF_LANGUAGE_MAX);
	if (langID < 0 || langID >= XDBF_LANGUAGE_MAX)
		return ret;

	// Get the string table.
	const rp::uvector<char> *vec = strTbls[langID];
	if (!vec) {
		vec = loadStringTable_SPA(langID);
		if (!vec) {
			// Unable to load the string table.
			return ret;
		}
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the ID to make it easier to find things.
	string_id = cpu_to_be16(string_id);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// TODO: Optimize by creating an unordered_map of IDs to strings?
	// Might not be a good optimization if we don't have that many strings...

	// Search for the specified string.
	const char *p = vec->data() + sizeof(XDBF_XSTR_Header);
	const char *const p_end = p + vec->size() - sizeof(XDBF_XSTR_Header);
	while (p < p_end) {
		// TODO: Verify alignment.
		const XDBF_XSTR_Entry_Header *const hdr =
			reinterpret_cast<const XDBF_XSTR_Entry_Header*>(p);
		const uint16_t length = be16_to_cpu(hdr->length);
		if (hdr->string_id == string_id) {
			// Found the string.
			// Verify that it doesn't go out of bounds.
			const char *const p_str = p + sizeof(XDBF_XSTR_Entry_Header);
			const char *const p_str_end = p_str + length;
			if (p_str_end <= p_end) {
				// Bounds are OK.
				// Character set conversion isn't needed, since
				// the string table is UTF-8, but we do need to
				// convert from DOS to UNIX line endings.
				ret = dos2unix(p_str, length);
			}
			break;
		} else {
			// Not the requested string.
			// Go to the next string.
			p += sizeof(XDBF_XSTR_Entry_Header) + length;
		}
	}

	return ret;
}

/**
 * Get a string from the resource table. (GPD)
 * @param string_id String ID.
 * @return String, or empty string on error.
 */
string Xbox360_XDBF_Private::loadString_GPD(uint16_t string_id)
{
	string ret;

	if (entryTable.empty()) {
		// Entry table isn't loaded...
		return ret;
	}

	// Can we load the string?
	if (!file || !isValid) {
		// Can't load the string.
		return ret;
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the ID to make it easier to find things.
	string_id = cpu_to_be16(string_id);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// TODO: Optimize by creating an unordered_map of IDs to strings?
	// Might not be a good optimization if we don't have that many strings...

	// NOTE: GPDs only have one language, so not using RFT_STRING_MULTI here.

	// GPD doesn't have string tables.
	// Instead, each string is its own entry in the main resource table.
	for (const XDBF_Entry &p : entryTable) {
		if (p.namespace_id != cpu_to_be16(XDBF_GPD_NAMESPACE_STRING)) {
			// Not a string.
			continue;
		}

		// Check for a matching resource ID.
		if (p.resource_id != string_id) {
			// Not a match. Skip this entry.
			continue;
		}

		const uint32_t addr = be32_to_cpu(p.offset) + this->data_offset;
		uint32_t length = be32_to_cpu(p.length);
		// Sanity check: Length must be > 2 but <= 4096 bytes,
		// and must be divisible by 2.
		assert(length > 2);
		assert(length <= 4096);
		assert(length % 2 == 0);
		if (length < 2 || length > 4096 || length % 2 != 0)
			continue;

		// Length includes the NULL terminator, so remove it.
		// NOTE: Some GPD files only have one NULL byte if the string
		// is at the end of the file, though length specifies two.
		length -= 2;

		// Read the string.
		unique_ptr<char16_t[]> sbuf(new char16_t[length / 2]);
		size_t size = file->seekAndRead(addr, sbuf.get(), length);
		if (size != length) {
			// Seek and/or read error.
			continue;
		}

		// Convert from UTF-16BE and DOS line endings.
		ret = dos2unix(utf16be_to_utf8(sbuf.get(), length / 2));
		break;
	}

	return ret;
}

/**
 * Get the language ID to use for the title fields.
 * @return XDBF language ID.
 */
XDBF_Language_e Xbox360_XDBF_Private::getLanguageID(void) const
{
	// TODO: Show the default language (XSTC) in a field?
	// (for both Xbox360_XDBF and Xbox360_XEX)
	if (m_langID != XDBF_LANGUAGE_UNKNOWN) {
		// We already got the language ID.
		return m_langID;
	}

	if (xdbfType != XDBFType::SPA) {
		// No language ID for GPD.
		return XDBF_LANGUAGE_UNKNOWN;
	}

	// Non-const pointer.
	Xbox360_XDBF_Private *const ncthis = const_cast<Xbox360_XDBF_Private*>(this);

	// Get the system language.
	const XDBF_Language_e langID = static_cast<XDBF_Language_e>(XboxLanguage::getXbox360Language());
	if (langID > XDBF_LANGUAGE_UNKNOWN && langID < XDBF_LANGUAGE_MAX) {
		// System language obtained.
		// Make sure the string table exists.
		if (ncthis->loadStringTable_SPA(langID) != nullptr) {
			// String table loaded.
			ncthis->m_langID = langID;
			return langID;
		}
	}

	// Not supported.
	// Get the XSTC struct to determine the default language.
	const XDBF_Entry *const entry = findResource(XDBF_SPA_NAMESPACE_METADATA, XDBF_XSTC_MAGIC);
	if (!entry) {
		// Not found...
		return XDBF_LANGUAGE_UNKNOWN;
	}

	// Load the XSTC entry.
	const uint32_t addr = be32_to_cpu(entry->offset) + this->data_offset;
	if (be32_to_cpu(entry->length) != sizeof(XDBF_XSTC)) {
		// Invalid size.
		return XDBF_LANGUAGE_UNKNOWN;
	}
	
	XDBF_XSTC xstc;
	size_t size = file->seekAndRead(addr, &xstc, sizeof(xstc));
	if (size != sizeof(xstc)) {
		// Seek and/or read error.
		return XDBF_LANGUAGE_UNKNOWN;
	}

	// Validate magic, version, and size.
	if (xstc.magic != cpu_to_be32(XDBF_XSTC_MAGIC) ||
	    xstc.version != cpu_to_be32(XDBF_XSTC_VERSION) ||
	    xstc.size != cpu_to_be32(sizeof(XDBF_XSTC) - sizeof(uint32_t)))
	{
		// Invalid fields.
		return XDBF_LANGUAGE_UNKNOWN;
	}

	// TODO: Check if the XSTC language matches langID,
	// and if so, skip the XSTC check.
	const XDBF_Language_e langID_xstc = static_cast<XDBF_Language_e>(be32_to_cpu(xstc.default_language));
	if (langID_xstc != langID) {
		if (langID_xstc <= XDBF_LANGUAGE_UNKNOWN || langID_xstc >= XDBF_LANGUAGE_MAX) {
			// Out of range.
			return XDBF_LANGUAGE_UNKNOWN;
		}

		// Default language obtained.
		// Make sure the string table exists.
		if (ncthis->loadStringTable_SPA(langID_xstc) != nullptr) {
			// String table loaded.
			ncthis->m_langID = langID_xstc;
			return langID_xstc;
		}
	}

	// One last time: Try using English as a fallback language.
	if (langID != XDBF_LANGUAGE_ENGLISH && langID_xstc != XDBF_LANGUAGE_ENGLISH) {
		if (ncthis->loadStringTable_SPA(XDBF_LANGUAGE_ENGLISH) != nullptr) {
			// String table loaded.
			ncthis->m_langID = XDBF_LANGUAGE_ENGLISH;
			return XDBF_LANGUAGE_ENGLISH;
		}
	}

	// No languages are available...
	return XDBF_LANGUAGE_UNKNOWN;
}

/**
 * Get the default language code for the multi-string fields.
 * @return Language code, e.g. 'en' or 'es'.
 */
inline uint32_t Xbox360_XDBF_Private::getDefaultLC(void) const
{
	// Get the system language.
	// TODO: Verify against the game's region code?
	const XDBF_Language_e langID = getLanguageID();
	uint32_t lc = XboxLanguage::getXbox360LanguageCode(langID);
	if (lc == 0) {
		// Invalid language code...
		// Default to English.
		lc = 'en';
	}
	return lc;
}

/**
 * Load an image resource.
 * @param image_id Image ID.
 * @return Decoded image, or nullptr on error.
 */
rp_image_const_ptr Xbox360_XDBF_Private::loadImage(uint64_t image_id)
{
	// Is the image already loaded?
	auto iter = map_images.find(image_id);
	if (iter != map_images.end()) {
		// We already loaded the image.
		return iter->second;
	}

	if (entryTable.empty()) {
		// Entry table isn't loaded...
		return nullptr;
	}

	// Can we load the image?
	if (!file || !isValid) {
		// Can't load the image.
		return nullptr;
	}

	// Icons are stored in PNG format.

	// Get the icon resource.
	const XDBF_Entry *const entry = findResource(XDBF_SPA_NAMESPACE_IMAGE, image_id);
	if (!entry) {
		// Not found...
		return nullptr;
	}

	// Load the image.
	const uint32_t addr = be32_to_cpu(entry->offset) + this->data_offset;
	const uint32_t length = be32_to_cpu(entry->length);
	// Sanity check:
	// - Size must be at least 16 bytes. [TODO: Smallest PNG?]
	// - Size must be a maximum of 1 MB.
	assert(length >= 16);
	assert(length <= 1024*1024);
	if (length < 16 || length > 1024*1024) {
		// Size is out of range.
		return nullptr;
	}

	unique_ptr<uint8_t[]> png_buf(new uint8_t[length]);
	size_t size = file->seekAndRead(addr, png_buf.get(), length);
	if (size != length) {
		// Seek and/or read error.
		return nullptr;
	}

	// Create a MemFile and decode the image.
	// TODO: For rpcli, shortcut to extract the PNG directly.
	MemFile *const f_mem = new MemFile(png_buf.get(), length);
	rp_image_ptr img = RpPng::load(f_mem);
	delete f_mem;

	if (img) {
		// Save the image for later use.
		map_images.emplace(image_id, img);
	}

	return img;
}

/**
 * Load the main title icon.
 * @return Icon, or nullptr on error.
 */
rp_image_const_ptr Xbox360_XDBF_Private::loadIcon(void)
{
	if (img_icon) {
		// Icon has already been loaded.
		return img_icon;
	} else if (!file || !isValid) {
		// Can't load the icon.
		return nullptr;
	}

	// Make sure the entry table is loaded.
	if (entryTable.empty()) {
		// Not loaded. Cannot load an icon.
		return nullptr;
	}

	// Get the icon.
	img_icon = loadImage(XDBF_ID_TITLE);
	return img_icon;
}

/**
 * Get the title type as a string.
 * @return Title type, or nullptr if not found.
 */
const char *Xbox360_XDBF_Private::getTitleType(void) const
{
	// Get the XTHD struct.
	// TODO: Cache it?
	const XDBF_Entry *const entry = findResource(XDBF_SPA_NAMESPACE_METADATA, XDBF_XTHD_MAGIC);
	if (!entry) {
		// Not found...
		return nullptr;
	}

	// Load the XTHD entry.
	const uint32_t addr = be32_to_cpu(entry->offset) + data_offset;
	if (be32_to_cpu(entry->length) != sizeof(XDBF_XTHD)) {
		// Invalid size.
		return nullptr;
	}

	XDBF_XTHD xthd;
	size_t size = file->seekAndRead(addr, &xthd, sizeof(xthd));
	if (size != sizeof(xthd)) {
		// Seek and/or read error.
		return nullptr;
	}

	static const array<const char*, 4> title_type_tbl = {{
		NOP_C_("Xbox360_XDBF|TitleType", "System Title"),
		NOP_C_("Xbox360_XDBF|TitleType", "Full Game"),
		NOP_C_("Xbox360_XDBF|TitleType", "Demo"),
		NOP_C_("Xbox360_XDBF|TitleType", "Download"),
	}};

	const uint32_t title_type = be32_to_cpu(xthd.title_type);
	if (title_type < title_type_tbl.size()) {
		return pgettext_expr("Xbox360_XDBF|TitleType",
			title_type_tbl[title_type]);
	}

	// Not found...
	return nullptr;
}

/** addFields: SPA **/

/**
 * Add the various XDBF string fields. (SPA)
 * @param fields RomFields*
 * @return 0 on success; non-zero on error.
 */
int Xbox360_XDBF_Private::addFields_strings_SPA(RomFields *fields) const
{
	// Title: Check if English is valid.
	// If it is, we'll de-duplicate the fields.
	// NOTE: English is language 1, so we can start the loop at 2 (Japanese).
	string title_en;
	if (strTblIndexes[XDBF_LANGUAGE_ENGLISH] >= 0) {
		title_en = const_cast<Xbox360_XDBF_Private*>(this)->loadString_SPA(
			XDBF_LANGUAGE_ENGLISH, XDBF_ID_TITLE);
	}
	const bool dedupe_titles = !title_en.empty();

	// Title fields.
	RomFields::StringMultiMap_t *const pMap_title = new RomFields::StringMultiMap_t();
	if (!title_en.empty()) {
		pMap_title->emplace('en', title_en);
	}
	for (int langID = XDBF_LANGUAGE_JAPANESE; langID < XDBF_LANGUAGE_MAX; langID++) {
		if (strTblIndexes[langID] < 0) {
			// This language is not available.
			continue;
		}

		string title_lang = const_cast<Xbox360_XDBF_Private*>(this)->loadString_SPA(
			(XDBF_Language_e)langID, XDBF_ID_TITLE);
		if (title_lang.empty()) {
			// Title is not available for this language.
			continue;
		}

		if (dedupe_titles) {
			// Is the title the same as the English title?
			// TODO: Avoid converting the string before this check?
			if (title_lang == title_en) {
				// Same title. Skip it.
				continue;
			}
		}

		const uint32_t lc = XboxLanguage::getXbox360LanguageCode(langID);
		assert(lc != 0);
		if (lc == 0)
			continue;

		pMap_title->emplace(lc, std::move(title_lang));
	}

	const char *const s_title_title = C_("RomData", "Title");
	if (!pMap_title->empty()) {
		const uint32_t def_lc = getDefaultLC();
		fields->addField_string_multi(s_title_title, pMap_title, def_lc);
	} else {
		delete pMap_title;
		fields->addField_string(s_title_title, C_("RomData", "Unknown"));
	}

	// Title type
	const char *const title_type = getTitleType();
	fields->addField_string(C_("RomData", "Type"),
		title_type ? title_type : C_("RomData", "Unknown"));

	// TODO: Get more fields from the .xlast resource. (XSRC)
	// - gzipped XML file, in UTF-16LE
	// - Has string IDs as well as the translated strings.

	// All fields added successfully.
	return 0;
}

/**
 * Add the Achievements RFT_LISTDATA field. (SPA)
 * @return 0 on success; non-zero on error.
 */
int Xbox360_XDBF_Private::addFields_achievements_SPA(void)
{
	if (entryTable.empty()) {
		// Entry table isn't loaded...
		return 1;
	}

	// Can we load the achievements?
	if (!file || !isValid) {
		// Can't load the achievements.
		return 2;
	}

	// Get the achievements table.
	const XDBF_Entry *const entry = findResource(XDBF_SPA_NAMESPACE_METADATA, XDBF_XACH_MAGIC);
	if (!entry) {
		// Not found...
		return 3;
	}

	// Load the achievements table.
	const uint32_t addr = be32_to_cpu(entry->offset) + this->data_offset;
	const uint32_t length = be32_to_cpu(entry->length);
	// Sanity check:
	// - Size must be at least sizeof(XDBF_XACH_Header).
	// - Size must be a maximum of sizeof(XDBF_XACH_Header) + (sizeof(XDBF_XACH_Entry_SPA) * 512).
	static constexpr unsigned int XACH_MAX_COUNT = 512;
	static constexpr uint32_t XACH_MIN_SIZE = (uint32_t)sizeof(XDBF_XACH_Header);
	static constexpr uint32_t XACH_MAX_SIZE = XACH_MIN_SIZE + (uint32_t)(sizeof(XDBF_XACH_Entry_SPA) * XACH_MAX_COUNT);
	assert(length > XACH_MIN_SIZE);
	assert(length <= XACH_MAX_SIZE);
	if (length < XACH_MIN_SIZE || length > XACH_MAX_SIZE) {
		// Size is out of range.
		return 4;
	}

	unique_ptr<uint8_t[]> xach_buf(new uint8_t[length]);
	size_t size = file->seekAndRead(addr, xach_buf.get(), length);
	if (size != length) {
		// Seek and/or read error.
		return 5;
	}

	// XACH header.
	const XDBF_XACH_Header *const hdr =
		reinterpret_cast<const XDBF_XACH_Header*>(xach_buf.get());
	// Validate the header.
	if (hdr->magic != cpu_to_be32(XDBF_XACH_MAGIC) ||
	    hdr->version != cpu_to_be32(XDBF_XACH_VERSION))
	{
		// Magic is invalid.
		// TODO: Report an error?
		return 6;
	}

	// Validate the entry count.
	unsigned int xach_count = be16_to_cpu(hdr->xach_count);
	if (xach_count > XACH_MAX_COUNT) {
		// Too many entries.
		// Reduce it to XACH_MAX_COUNT.
		xach_count = XACH_MAX_COUNT;
	} else if (xach_count > ((length - sizeof(XDBF_XACH_Header)) / sizeof(XDBF_XACH_Entry_SPA))) {
		// Entry count is too high.
		xach_count = ((length - sizeof(XDBF_XACH_Header)) / sizeof(XDBF_XACH_Entry_SPA));
	}

	const XDBF_XACH_Entry_SPA *p =
		reinterpret_cast<const XDBF_XACH_Entry_SPA*>(xach_buf.get() + sizeof(*hdr));
	const XDBF_XACH_Entry_SPA *const p_end = p + xach_count;

	// Icons don't have their own column name; they're considered
	// a virtual column, much like checkboxes.

	// Columns
	static const array<const char*, 3> xach_col_names = {{
		NOP_C_("Xbox360_XDBF|Achievements", "ID"),
		NOP_C_("Xbox360_XDBF|Achievements", "Description"),
		NOP_C_("Xbox360_XDBF|Achievements", "Gamerscore"),
	}};
	vector<string> *const v_xach_col_names = RomFields::strArrayToVector_i18n(
		"Xbox360_XDBF|Achievements", xach_col_names);

	// Vectors.
	array<RomFields::ListData_t*, XDBF_LANGUAGE_MAX> pvv_xach;
	pvv_xach[XDBF_LANGUAGE_UNKNOWN] = nullptr;
	for (int langID = XDBF_LANGUAGE_ENGLISH; langID < XDBF_LANGUAGE_MAX; langID++) {
		pvv_xach[langID] = (strTblIndexes[langID] >= 0)
			? new RomFields::ListData_t(xach_count)
			: nullptr;
	}
	auto *const vv_icons = new RomFields::ListDataIcons_t(xach_count);
	auto icon_iter = vv_icons->begin();
	for (unsigned int i = 0; p < p_end && i < xach_count; p++, i++, ++icon_iter) {
		// NOTE: Not deduplicating strings here.

		// Icon
		*icon_iter = loadImage(be32_to_cpu(p->image_id));

		// Achievement IDs.
		const uint16_t name_id = be16_to_cpu(p->name_id);
		const uint16_t locked_desc_id = be16_to_cpu(p->locked_desc_id);
		const uint16_t unlocked_desc_id = be16_to_cpu(p->unlocked_desc_id);

		// TODO: Localized numeric formatting?
		const string s_achievement_id = fmt::format(FSTR("{:d}"), be16_to_cpu(p->achievement_id));
		const string s_gamerscore = fmt::format(FSTR("{:d}"), be16_to_cpu(p->gamerscore));

		for (int langID = XDBF_LANGUAGE_ENGLISH; langID < XDBF_LANGUAGE_MAX; langID++) {
			if (!pvv_xach[langID]) {
				// No strings for this language.
				continue;
			}
			auto &data_row = pvv_xach[langID]->at(i);
			data_row.reserve(3);

			// Achievement ID
			data_row.push_back(s_achievement_id);

			// Title.
			string desc = loadString_SPA((XDBF_Language_e)langID, name_id);
			if (desc.empty() && langID != XDBF_LANGUAGE_ENGLISH) {
				// String not found in this language. Try English.
				desc = loadString_SPA(XDBF_LANGUAGE_ENGLISH, name_id);
			}

			// Description.
			// If we don't have a locked ID, use the unlocked ID.
			// (TODO: This may be a hidden achievement.)
			const uint16_t desc_id = (locked_desc_id != 0xFFFF)
							? locked_desc_id
							: unlocked_desc_id;

			string lck_desc = loadString_SPA((XDBF_Language_e)langID, desc_id);
			if (lck_desc.empty() && langID != XDBF_LANGUAGE_ENGLISH) {
				// String not found in this language. Try English.
				lck_desc = loadString_SPA(XDBF_LANGUAGE_ENGLISH, desc_id);
			}

			if (!lck_desc.empty()) {
				if (!desc.empty()) {
					desc += '\n';
					desc += lck_desc;
				} else {
					desc = std::move(lck_desc);
				}
			}

			// TODO: Formatting value indicating that the first line should be bold.
			data_row.emplace_back(std::move(desc));

			// Gamerscore
			data_row.push_back(s_gamerscore);
		}
	}

	// Add the vectors to a map.
	RomFields::ListDataMultiMap_t *const mvv_xach = new RomFields::ListDataMultiMap_t();
	for (int langID = XDBF_LANGUAGE_ENGLISH; langID < XDBF_LANGUAGE_MAX; langID++) {
		if (!pvv_xach[langID]) {
			// No vector for this language.
			continue;
		} else if (pvv_xach[langID]->empty()) {
			// No string data.
			delete pvv_xach[langID];
			continue;
		}

		const uint32_t lc = XboxLanguage::getXbox360LanguageCode(langID);
		assert(lc != 0);
		if (lc == 0) {
			// Invalid language code.
			delete pvv_xach[langID];
			continue;
		}

		mvv_xach->emplace(lc, std::move(*pvv_xach[langID]));
		delete pvv_xach[langID];
	}

	// Add the list data.
	RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_SEPARATE_ROW |
	                              RomFields::RFT_LISTDATA_ICONS |
				      RomFields::RFT_LISTDATA_MULTI, 0);
	params.headers = v_xach_col_names;
	params.data.multi = mvv_xach;
	params.def_lc = getDefaultLC();
	// TODO: Header alignment?
	params.col_attrs.align_headers	= AFLD_ALIGN3(TXA_D, TXA_D, TXA_C);
	params.col_attrs.align_data	= AFLD_ALIGN3(TXA_L, TXA_L, TXA_C);
	params.col_attrs.sizing		= AFLD_ALIGN3(COLSZ_R, COLSZ_S, COLSZ_R);
	params.col_attrs.sorting	= AFLD_ALIGN3(COLSORT_NUM, COLSORT_STD, COLSORT_NUM);
	params.col_attrs.sort_col	= 0;	// ID
	params.col_attrs.sort_dir	= RomFields::COLSORTORDER_ASCENDING;
	params.mxd.icons = vv_icons;
	fields.addField_listData(C_("Xbox360_XDBF", "Achievements"), &params);
	return 0;
}

/**
 * Add the Avatar Awards RFT_LISTDATA field. (SPA)
 * @return 0 on success; non-zero on error.
 */
int Xbox360_XDBF_Private::addFields_avatarAwards_SPA(void)
{
	if (entryTable.empty()) {
		// Entry table isn't loaded...
		return 1;
	}

	// Can we load the achievements?
	if (!file || !isValid) {
		// Can't load the achievements.
		return 2;
	}

	// Get the achievements table.
	const XDBF_Entry *const entry = findResource(XDBF_SPA_NAMESPACE_METADATA, XDBF_XGAA_MAGIC);
	if (!entry) {
		// Not found...
		return 3;
	}

	// Load the avatar awards table.
	const uint32_t addr = be32_to_cpu(entry->offset) + this->data_offset;
	const uint32_t length = be32_to_cpu(entry->length);
	// Sanity check:
	// - Size must be at least sizeof(XDBF_XGAA_Header).
	// - Size must be a maximum of sizeof(XDBF_XGAA_Header) + (sizeof(XDBF_XGAA_Entry) * 16).
	static constexpr unsigned int XGAA_MAX_COUNT = 16;
	static constexpr uint32_t XGAA_MIN_SIZE = (uint32_t)sizeof(XDBF_XGAA_Header);
	static constexpr uint32_t XGAA_MAX_SIZE = XGAA_MIN_SIZE + (uint32_t)(sizeof(XDBF_XGAA_Entry) * XGAA_MAX_COUNT);
	assert(length >= XGAA_MIN_SIZE);
	assert(length <= XGAA_MAX_SIZE);
	if (unlikely(length == XGAA_MIN_SIZE)) {
		// Minimum size, which means this section doesn't
		// actually have any avatar awards. This game was
		// built with a newer SDK that supports it, but no
		// avatar awards were created.
		return 4;
	} else if (length < XGAA_MIN_SIZE || length > XGAA_MAX_SIZE) {
		// Size is out of range.
		return 5;
	}

	unique_ptr<uint8_t[]> xgaa_buf(new uint8_t[length]);
	size_t size = file->seekAndRead(addr, xgaa_buf.get(), length);
	if (size != length) {
		// Seek and/or read error.
		return 6;
	}

	// XGAA header.
	const XDBF_XGAA_Header *const hdr =
		reinterpret_cast<const XDBF_XGAA_Header*>(xgaa_buf.get());
	// Validate the header.
	if (hdr->magic != cpu_to_be32(XDBF_XGAA_MAGIC) ||
	    hdr->version != cpu_to_be32(XDBF_XGAA_VERSION))
	{
		// Magic is invalid.
		// TODO: Report an error?
		return 7;
	}

	// Validate the entry count.
	unsigned int xgaa_count = be16_to_cpu(hdr->xgaa_count);
	if (unlikely(xgaa_count == 0)) {
		// No entries...
		return 8;
	} else if (xgaa_count > XGAA_MAX_COUNT) {
		// Too many entries.
		// Reduce it to XGAA_MAX_COUNT.
		xgaa_count = XGAA_MAX_COUNT;
	} else if (xgaa_count > ((length - sizeof(XDBF_XGAA_Header)) / sizeof(XDBF_XGAA_Entry))) {
		// Entry count is too high.
		xgaa_count = ((length - sizeof(XDBF_XGAA_Header)) / sizeof(XDBF_XGAA_Entry));
	}

	const XDBF_XGAA_Entry *p =
		reinterpret_cast<const XDBF_XGAA_Entry*>(xgaa_buf.get() + sizeof(*hdr));
	const XDBF_XGAA_Entry *const p_end = p + xgaa_count;

	// Icons don't have their own column name; they're considered
	// a virtual column, much like checkboxes.

	// Columns
	static const array<const char*, 2> xgaa_col_names = {
		NOP_C_("Xbox360_XDBF|AvatarAwards", "ID"),
		NOP_C_("Xbox360_XDBF|AvatarAwards", "Description"),
	};
	vector<string> *const v_xgaa_col_names = RomFields::strArrayToVector_i18n(
		"Xbox360_XDBF|AvatarAwards", xgaa_col_names);

	// Vectors.
	array<RomFields::ListData_t*, XDBF_LANGUAGE_MAX> pvv_xgaa;
	pvv_xgaa[XDBF_LANGUAGE_UNKNOWN] = nullptr;
	for (int langID = XDBF_LANGUAGE_ENGLISH; langID < XDBF_LANGUAGE_MAX; langID++) {
		pvv_xgaa[langID] = (strTblIndexes[langID] >= 0)
			? new RomFields::ListData_t(xgaa_count)
			: nullptr;
	}
	auto *const vv_icons = new RomFields::ListDataIcons_t(xgaa_count);
	auto icon_iter = vv_icons->begin();
	for (unsigned int i = 0; p < p_end && i < xgaa_count; p++, i++, ++icon_iter) {
		// NOTE: Not deduplicating strings here.

		// Icon
		*icon_iter = loadImage(be32_to_cpu(p->image_id));

		// Avatar award IDs.
		const uint16_t name_id = be16_to_cpu(p->name_id);
		const uint16_t locked_desc_id = be16_to_cpu(p->locked_desc_id);
		const uint16_t unlocked_desc_id = be16_to_cpu(p->unlocked_desc_id);

		// TODO: Localized numeric formatting?
		// FIXME: Should this be decimal instead of hex?
		const string s_avatar_award_id = fmt::format(FSTR("{:0>4X}"), be16_to_cpu(p->avatar_award_id));

		for (int langID = XDBF_LANGUAGE_ENGLISH; langID < XDBF_LANGUAGE_MAX; langID++) {
			if (!pvv_xgaa[langID]) {
				// No strings for this language.
				continue;
			}
			auto &data_row = pvv_xgaa[langID]->at(i);
			data_row.reserve(2);

			// Avatar award ID
			data_row.push_back(s_avatar_award_id);

			// Title.
			string desc = loadString_SPA((XDBF_Language_e)langID, name_id);
			if (desc.empty() && langID != XDBF_LANGUAGE_ENGLISH) {
				// String not found in this language. Try English.
				desc = loadString_SPA(XDBF_LANGUAGE_ENGLISH, name_id);
			}

			// Description.
			// If we don't have a locked ID, use the unlocked ID.
			// (TODO: This may be a hidden achievement.)
			const uint16_t desc_id = (locked_desc_id != 0xFFFF)
							? locked_desc_id
							: unlocked_desc_id;

			string lck_desc = loadString_SPA((XDBF_Language_e)langID, desc_id);
			if (lck_desc.empty() && langID != XDBF_LANGUAGE_ENGLISH) {
				// String not found in this language. Try English.
				lck_desc = loadString_SPA(XDBF_LANGUAGE_ENGLISH, desc_id);
			}

			if (!lck_desc.empty()) {
				if (!desc.empty()) {
					desc += '\n';
					desc += lck_desc;
				} else {
					desc = std::move(lck_desc);
				}
			}

			// TODO: Formatting value indicating that the first line should be bold.
			data_row.emplace_back(std::move(desc));
		}
	}

	// Add the vectors to a map.
	RomFields::ListDataMultiMap_t *const mvv_xgaa = new RomFields::ListDataMultiMap_t();
	for (int langID = XDBF_LANGUAGE_ENGLISH; langID < XDBF_LANGUAGE_MAX; langID++) {
		if (!pvv_xgaa[langID]) {
			// No vector for this language.
			continue;
		} else if (pvv_xgaa[langID]->empty()) {
			// No string data.
			delete pvv_xgaa[langID];
			continue;
		}

		const uint32_t lc = XboxLanguage::getXbox360LanguageCode(langID);
		assert(lc != 0);
		if (lc == 0) {
			// Invalid language code.
			delete pvv_xgaa[langID];
			continue;
		}

		mvv_xgaa->emplace(lc, std::move(*pvv_xgaa[langID]));
		delete pvv_xgaa[langID];
	}

	// Add the list data.
	// TODO: Improve the display? On KDE, it seems to be limited to
	// one row due to achievements taking up all the space.
	RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_SEPARATE_ROW |
	                              RomFields::RFT_LISTDATA_ICONS |
				      RomFields::RFT_LISTDATA_MULTI, 2);
	params.headers = v_xgaa_col_names;
	params.col_attrs.sizing		= AFLD_ALIGN2(COLSZ_R, COLSZ_S);
	params.col_attrs.sorting	= AFLD_ALIGN2(COLSORT_NUM, COLSORT_STD);
	params.col_attrs.sort_col	= 0;	// ID
	params.col_attrs.sort_dir	= RomFields::COLSORTORDER_ASCENDING;
	params.data.multi = mvv_xgaa;
	params.mxd.icons = vv_icons;
	fields.addField_listData(C_("Xbox360_XDBF", "Avatar Awards"), &params);
	return 0;
}

/** addFields(): GPD **/

/**
 * Add the various XDBF string fields. (GPD)
 * @param fields RomFields*
 * @return 0 on success; non-zero on error.
 */
int Xbox360_XDBF_Private::addFields_strings_GPD(RomFields *fields) const
{
	if (entryTable.empty()) {
		// Entry table isn't loaded...
		return 1;
	}

	// Can we load the achievements?
	if (!file || !isValid) {
		// Can't load the achievements.
		return 2;
	}

	// NOTE: GPDs only have one language, so not using RFT_STRING_MULTI here.

	// Title
	const char *const s_title_title = C_("RomData", "Title");
	const string s_title = const_cast<Xbox360_XDBF_Private*>(this)->loadString_GPD(XDBF_ID_TITLE);
	if (!s_title.empty()) {
		fields->addField_string(s_title_title, s_title);
	} else {
		fields->addField_string(s_title_title, C_("RomData", "Unknown"));
	}

	// TODO: More string resources in GPD files?

	// All fields added successfully.
	return 0;
}

/**
 * Add the Achievements RFT_LISTDATA field. (SPA)
 * @return 0 on success; non-zero on error.
 */
int Xbox360_XDBF_Private::addFields_achievements_GPD(void)
{
	if (entryTable.empty()) {
		// Entry table isn't loaded...
		return 1;
	}

	// Can we load the achievements?
	if (!file || !isValid) {
		// Can't load the achievements.
		return 2;
	}

	// NOTE: GPDs only have one language, so not using RFT_LISTDATA_MULTI here.
	// TODO: Optimal reservation values?

	// Icons don't have their own column name; they're considered
	// a virtual column, much like checkboxes.

	// Columns
	static const array<const char*, 3> xach_col_names = {{
		NOP_C_("Xbox360_XDBF|Achievements", "ID"),
		NOP_C_("Xbox360_XDBF|Achievements", "Description"),
		NOP_C_("Xbox360_XDBF|Achievements", "Gamerscore"),
	}};
	vector<string> *const v_xach_col_names = RomFields::strArrayToVector_i18n(
		"Xbox360_XDBF|Achievements", xach_col_names);

	RomFields::ListData_t *vv_xach = new RomFields::ListData_t();
	auto *const vv_icons = new RomFields::ListDataIcons_t();
	vv_xach->reserve(16);
	vv_icons->reserve(16);

	// GPD doesn't have an achievements table.
	// Instead, each achievement is its own entry in the main resource table.
#define XACH_GPD_BUF_LEN 4096
	unique_ptr<uint8_t[]> buf(new uint8_t[XACH_GPD_BUF_LEN]);
	const XDBF_XACH_Entry_Header_GPD *const pGPD =
		reinterpret_cast<const XDBF_XACH_Entry_Header_GPD*>(buf.get());
	for (const XDBF_Entry &p : entryTable) {
		if (p.namespace_id != cpu_to_be16(XDBF_GPD_NAMESPACE_ACHIEVEMENT)) {
			// Not an achievement.
			continue;
		}

		// Check for Sync List or Sync Data entry.
		if (p.resource_id == cpu_to_be64(XDBF_GPD_SYNC_LIST_ENTRY) ||
		    p.resource_id == cpu_to_be64(XDBF_GPD_SYNC_DATA_ENTRY))
		{
			// Skip this entry.
			continue;
		}

		const uint32_t addr = be32_to_cpu(p.offset) + this->data_offset;
		const uint32_t length = be32_to_cpu(p.length);
		// Sanity check: Achievement should be at least sizeof(*pGPD),
		// but shouldn't be more than sizeof(buf).
		assert(length >= sizeof(*pGPD));
		assert(length <= XACH_GPD_BUF_LEN);
		if (length < sizeof(*pGPD) || length > XACH_GPD_BUF_LEN)
			continue;

		// Read the achievement.
		size_t size = file->seekAndRead(addr, buf.get(), length);
		if (size != length) {
			// Seek and/or read error.
			continue;
		}

		// Verify achievement header size.
		assert(be32_to_cpu(pGPD->size) == sizeof(*pGPD));
		if (be32_to_cpu(pGPD->size) != sizeof(*pGPD)) {
			// Incorrect achievement header size.
			continue;
		}

		// Icon.
		// TODO: Grayscale version if locked?
		// NOTE: Most GPDs don't have achievement icons...
		vv_icons->push_back(loadImage(be32_to_cpu(pGPD->image_id)));

		// TODO: Localized numeric formatting?
		const string s_achievement_id = fmt::format(FSTR("{:d}"), be32_to_cpu(pGPD->achievement_id));
		const string s_gamerscore = fmt::format(FSTR("{:d}"), be32_to_cpu(pGPD->gamerscore));

		// Get the strings.
		const char16_t *pTitle = nullptr, *pUnlockedDesc = nullptr, *pLockedDesc = nullptr;
		const char16_t *pstr = reinterpret_cast<const char16_t*>(&buf[sizeof(*pGPD)]);
		const char16_t *const pstr_end = reinterpret_cast<const char16_t*>(&buf[length]);

		// Find the first NULL.
		const char16_t *pNull = u16_memchr(pstr, 0, (pstr_end - pstr));
		if (pNull) {
			pTitle = pstr;
			// Find the second NULL.
			pstr = pNull + 1;
			if (pstr < pstr_end) {
				pNull = u16_memchr(pstr, 0, (pstr_end - pstr));
				if (pNull) {
					pUnlockedDesc = pstr;
					// Find the third NULL.
					pstr = pNull + 1;
					if (pstr < pstr_end) {
						pNull = u16_memchr(pstr, 0, (pstr_end - pstr));
						if (pNull) {
							pLockedDesc = pstr;
						}
					}
				}
			}
		}

		// TODO: Verify flags for achievement unlocked status.
		// For now, assuming all achievements are unlocked.
		string desc;
		if (pTitle) {
			desc = dos2unix(utf16be_to_utf8(pTitle, -1));
		}
		if (pUnlockedDesc) {
			if (!desc.empty()) {
				desc += '\n';
			}
			desc += dos2unix(utf16be_to_utf8(pUnlockedDesc, -1));
		}

		// Add to RFT_LISTDATA.
		vector<string> data_row;
		data_row.reserve(3);
		data_row.push_back(s_achievement_id);
		data_row.emplace_back(std::move(desc));
		data_row.push_back(s_gamerscore);
		vv_xach->emplace_back(std::move(data_row));
	}

	// FIXME: Figure out why Dolphin segfaults if the list is empty.
	if (vv_xach->empty()) {
		// No achievements.
		delete v_xach_col_names;
		delete vv_xach;
		delete vv_icons;
		return -ENOENT;
	}

	// Add the list data.
	RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_SEPARATE_ROW |
	                              RomFields::RFT_LISTDATA_ICONS, 0);
	params.headers = v_xach_col_names;
	params.data.single = vv_xach;
	params.def_lc = getDefaultLC();
	// TODO: Header alignment?
	params.col_attrs.align_headers	= AFLD_ALIGN3(TXA_D, TXA_D, TXA_C);
	params.col_attrs.align_data	= AFLD_ALIGN3(TXA_L, TXA_L, TXA_C);
	params.col_attrs.sizing		= AFLD_ALIGN3(COLSZ_R, COLSZ_S, COLSZ_R);
	params.col_attrs.sorting	= AFLD_ALIGN3(COLSORT_NUM, COLSORT_STD, COLSORT_NUM);
	params.col_attrs.sort_col	= 0;	// ID
	params.col_attrs.sort_dir	= RomFields::COLSORTORDER_ASCENDING;
	params.mxd.icons = vv_icons;
	fields.addField_listData(C_("Xbox360_XDBF", "Achievements"), &params);
	return 0;
}

/** Xbox360_XDBF **/

/**
 * Read an Xbox 360 XDBF file and/or section.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the file.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open XDBF file and/or section
 * @param xex If true, hide fields that are displayed separately in XEX executables.
 */
Xbox360_XDBF::Xbox360_XDBF(const IRpFilePtr &file, bool xex)
	: super(new Xbox360_XDBF_Private(file, xex))
{
	// This class handles XDBF files and/or sections only.
	RP_D(Xbox360_XDBF);
	d->mimeType = "application/x-xbox360-xdbf";	// unofficial, not on fd.o
	d->fileType = FileType::ResourceFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the Xbox360_XDBF header.
	// NOTE: Reading 512 bytes so we can detect SPA vs. GPD.
	uint8_t header[512];
	d->file->rewind();
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header)) {
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(header), header},
		nullptr,	// ext (not needed for Xbox360_XDBF)
		0		// szFile (not needed for Xbox360_XDBF)
	};
	d->xdbfType = static_cast<Xbox360_XDBF_Private::XDBFType>(isRomSupported_static(&info));
	d->isValid = ((int)d->xdbfType >= 0);

	if (!d->isValid) {
		d->xdbfHeader.magic = 0;
		d->file.reset();
		return;
	}

	// Copy the XDBF header.
	memcpy(&d->xdbfHeader, header, sizeof(d->xdbfHeader));

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the header for little-endian systems.
	// NOTE: The magic number is *not* byteswapped here.
	d->xdbfHeader.version			= be32_to_cpu(d->xdbfHeader.version);
	d->xdbfHeader.entry_table_length	= be32_to_cpu(d->xdbfHeader.entry_table_length);
	d->xdbfHeader.entry_count		= be32_to_cpu(d->xdbfHeader.entry_count);
	d->xdbfHeader.free_space_table_length	= be32_to_cpu(d->xdbfHeader.free_space_table_length);
	d->xdbfHeader.free_space_table_count	= be32_to_cpu(d->xdbfHeader.free_space_table_count);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// Calculate the data start offset.
	d->data_offset = sizeof(d->xdbfHeader) +
		(d->xdbfHeader.entry_table_length * sizeof(XDBF_Entry)) +
		(d->xdbfHeader.free_space_table_length * sizeof(XDBF_Free_Space_Entry));

	// Sanity check: Maximum of 1,048,576 entries.
	//assert(d->xdbfHeader.entry_table_length < 1048576);
	if (d->xdbfHeader.entry_table_length >= 1048576) {
		// Too many entries.
		d->xdbfHeader.magic = 0;
		d->file.reset();
		d->isValid = false;
		return;
	}

	// Read the entry table.
	// TODO: For GPD, is it possible to have holes in the entry table?
	const size_t entry_table_sz = d->xdbfHeader.entry_table_length * sizeof(XDBF_Entry);
	d->entryTable.resize(d->xdbfHeader.entry_table_length);
	size = d->file->seekAndRead(sizeof(d->xdbfHeader), d->entryTable.data(), entry_table_sz);
	if (size != entry_table_sz) {
		// Read error.
		d->entryTable.clear();
		d->xdbfHeader.magic = 0;
		d->file.reset();
		d->isValid = false;
		return;
	}

	// Initialize the string table indexes.
	d->initStrTblIndexes();
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Xbox360_XDBF::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 512)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(Xbox360_XDBF_Private::XDBFType::Unknown);
	}

	// Check for XDBF.
	const XDBF_Header *const xdbfHeader =
		reinterpret_cast<const XDBF_Header*>(info->header.pData);
	if (xdbfHeader->magic != cpu_to_be32(XDBF_MAGIC) ||
	    xdbfHeader->version != cpu_to_be32(XDBF_VERSION))
	{
		// Not an XDBF file.
		return static_cast<int>(Xbox360_XDBF_Private::XDBFType::Unknown);
	}

	// We have an XDBF file.
	// Check if it's SPA or GPD.
	// SPA will have an XSRC or XSTC entry in the resource table.
	const unsigned int tblSize = info->header.size - sizeof(*xdbfHeader);
	const unsigned int entryCount = std::min(
		(unsigned int)be32_to_cpu(xdbfHeader->entry_count),
		(unsigned int)(tblSize / sizeof(XDBF_Entry)));

	const XDBF_Entry *entry =
		reinterpret_cast<const XDBF_Entry*>(&info->header.pData[sizeof(*xdbfHeader)]);
	const XDBF_Entry *const entry_end = entry + entryCount;
	for (; entry < entry_end; ++entry) {
		if (be16_to_cpu(entry->namespace_id) != XDBF_SPA_NAMESPACE_METADATA)
			continue;

		// Check if it's XSTC or XSRC.
		const uint64_t resource_id = be64_to_cpu(entry->resource_id);
		if (resource_id == XDBF_XSTC_MAGIC ||
			resource_id == XDBF_XSRC_MAGIC)
		{
			// Found XSTC or XSRC.
			// This is an SPA XDBF file.
			return static_cast<int>(Xbox360_XDBF_Private::XDBFType::SPA);
		}
	}

	// XSTC or XSRC were not found.
	// Assume this is a GPD XDBF file.
	return static_cast<int>(Xbox360_XDBF_Private::XDBFType::GPD);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *Xbox360_XDBF::systemName(unsigned int type) const
{
	RP_D(const Xbox360_XDBF);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Xbox 360 has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Xbox360_XDBF::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	// TODO: XDBF-specific, or just use Xbox 360?
	static const array<const char*, 4> sysNames = {{
		"Microsoft Xbox 360", "Xbox 360", "X360", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Xbox360_XDBF::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> Xbox360_XDBF::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_ICON) {
		// Only icons are supported.
		return {};
	}

	// FIXME: Get the actual icon size from the PNG image.
	// For now, assuming all games use 64x64.
	return {{nullptr, 64, 64, 0}};
}

/**
 * Get image processing flags.
 *
 * These specify post-processing operations for images,
 * e.g. applying transparency masks.
 *
 * @param imageType Image type.
 * @return Bitfield of ImageProcessingBF operations to perform.
 */
uint32_t Xbox360_XDBF::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_INT_ICON:
			// Use nearest-neighbor scaling when resizing.
			// Image is internally stored in PNG format.
			ret = IMGPF_RESCALE_NEAREST | IMGPF_INTERNAL_PNG_FORMAT;
			break;

		default:
			break;
	}
	return ret;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Xbox360_XDBF::loadFieldData(void)
{
	RP_D(Xbox360_XDBF);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->xdbfType < 0) {
		// Unknown XDBF type.
		return -EIO;
	}

	// Parse the XDBF file.
	// NOTE: The magic number is NOT byteswapped in the constructor.
	const XDBF_Header *const xdbfHeader = &d->xdbfHeader;
	if (xdbfHeader->magic != cpu_to_be32(XDBF_MAGIC)) {
		// Invalid magic number.
		return 0;
	}

	// Default tab name.
	d->fields.setTabName(0, "XDBF");

	// TODO: XSTR string table handling class.
	// For now, just reading it directly.

	// TODO: Convenience function to look up a resource
	// given a namespace ID and resource ID.

	if (!d->xex) {
		d->addFields_strings(&d->fields);
	}

	// TODO: Create a separate tab for avatar awards and achievements?

	// Avatar Awards
	// NOTE: Displayed before achievements because achievements uses up
	// the rest of the window.
	d->addFields_avatarAwards();

	// Achievements
	d->addFields_achievements();

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int Xbox360_XDBF::loadMetaData(void)
{
	RP_D(Xbox360_XDBF);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// XDBF file isn't valid.
		return -EIO;
	}

	d->metaData.reserve(1);	// Maximum of 1 metadata property.

	// NOTE: RomMetaData ignores empty strings, so we don't need to
	// check for them here.

	// Title
	d->metaData.addMetaData_string(Property::Title, getString(Property::Title));

	// Finished reading the metadata.
	return static_cast<int>(d->metaData.count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int Xbox360_XDBF::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(Xbox360_XDBF);
	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported by 3DS.
		pImage.reset();
		return -ENOENT;
	} else if (d->img_icon) {
		// Image has already been loaded.
		pImage = d->img_icon;
		return 0;
	} else if (!d->file) {
		// File isn't open.
		pImage.reset();
		return -EBADF;
	} else if (!d->isValid || (int)d->xdbfType < 0) {
		// XDBF file isn't valid.
		pImage.reset();
		return -EIO;
	}

	// Load the icon.
	pImage = d->loadIcon();
	return (pImage) ? 0 : -EIO;
}

/** Special XDBF accessor functions **/

/**
 * Add the various XDBF string fields.
 * @param fields RomFields*
 * @return 0 on success; non-zero on error.
 */
int Xbox360_XDBF::addFields_strings(LibRpBase::RomFields *fields) const
{
	RP_D(const Xbox360_XDBF);
	return d->addFields_strings(fields);
}

/**
 * Get a particular string property for RomMetaData.
 * @param property Property
 * @return String, or empty string if not found.
 */
string Xbox360_XDBF::getString(LibRpBase::Property property) const
{
	uint16_t string_id = 0;
	switch (property) {
		case LibRpBase::Property::Title:
			string_id = XDBF_ID_TITLE;
			break;
		default:
			break;
	}

	assert(string_id != 0);
	if (string_id == 0) {
		// Not supported.
		return {};
	}

	RP_D(Xbox360_XDBF);
	string s_ret;
	switch (d->xdbfType) {
		default:
			assert(!"Unsupported XDBF type.");
			break;
		case Xbox360_XDBF_Private::XDBFType::SPA:
			s_ret = d->loadString_SPA(d->getLanguageID(), string_id);
			break;
		case Xbox360_XDBF_Private::XDBFType::GPD:
			s_ret = d->loadString_GPD(string_id);
			break;
	}
	return s_ret;
}

} // namespace LibRomData
