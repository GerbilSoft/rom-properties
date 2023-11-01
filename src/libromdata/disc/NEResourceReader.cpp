/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NEResourceReader.cpp: New Executable resource reader.                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "NEResourceReader.hpp"

#include "../Other/exe_ne_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::string;
using std::unique_ptr;
using std::unordered_map;

namespace LibRomData {

class NEResourceReaderPrivate
{
public:
	NEResourceReaderPrivate(NEResourceReader *q,
		uint32_t rsrc_tbl_addr, uint32_t rsrc_tbl_size);

private:
	RP_DISABLE_COPY(NEResourceReaderPrivate)
protected:
	NEResourceReader *const q_ptr;

public:
	// Resource segment table
	uint32_t rsrc_tbl_addr;
	uint32_t rsrc_tbl_size;

	// Resource table entry
	struct ResTblEntry {
		uint16_t id;	// Resource ID
		uint32_t addr;	// Address of the resource data (0 = start of EXE)
		uint32_t len;	// Length of the resource data
	};
	typedef rp::uvector<ResTblEntry> rsrc_dir_t;

	// Resource types
	unordered_map<uint16_t, rsrc_dir_t> res_types;

	/**
	 * Load the resource table.
	 * @return 0 on success; non-zero on error.
	 */
	int loadResTbl(void);

	/**
	 * Read the section header in an NE version resource.
	 *
	 * The file pointer will be advanced past the header.
	 *
	 * @param file		[in] PE version resource.
	 * @param key		[in] Expected header name.
	 * @param pChildLen	[out,opt] Total length of the section.
	 * @param pValueLen	[out,opt] Value length.
	 * @return 0 if the header matches; non-zero on error.
	 */
	static int load_VS_VERSION_INFO_header(IRpFile *file, const char *key, uint16_t *pLen, uint16_t *pValueLen);

	/**
	 * Load a string table.
	 * @param file		[in] PE version resource.
	 * @param st		[out] String Table.
	 * @param langID	[out] Language ID.
	 * @return 0 on success; non-zero on error.
	 */
	static int load_StringTable(IRpFile *file, IResourceReader::StringTable &st, uint32_t *langID);
};

/** NEResourceReaderPrivate **/

NEResourceReaderPrivate::NEResourceReaderPrivate(
		NEResourceReader *q,
		uint32_t rsrc_tbl_addr,
		uint32_t rsrc_tbl_size)
	: q_ptr(q)
	, rsrc_tbl_addr(rsrc_tbl_addr)
	, rsrc_tbl_size(rsrc_tbl_size)
{
	if (!q->m_file) {
		q->m_lastError = -EBADF;
		return;
	} else if (rsrc_tbl_addr == 0) {
		q->m_file.reset();
		q->m_lastError = -EIO;
		return;
	} else if (rsrc_tbl_size < 6 || rsrc_tbl_size >= 65536) {
		// 64 KB is the segment size, so this shouldn't be possible.
		// Also, it should be at least 6 bytes.
		// (TODO: Larger minimum size?)
		q->m_file.reset();
		q->m_lastError = -EIO;
		return;
	}

	// NOTE: Win16 executables don't have a separate
	// .rsrc section, so we have to use the entire
	// size of the file as the resource size.
	static const uint32_t fileSize_MAX = 16U*1024*1024;

	// Validate the starting address and size.
	const off64_t fileSize_o64 = q->m_file->size();
	if (fileSize_o64 > fileSize_MAX) {
		// A Win16 executable larger than 16 MB doesn't make any sense.
		q->m_file.reset();
		q->m_lastError = -EIO;
		return;
	}

	const uint32_t fileSize = static_cast<uint32_t>(fileSize_o64);
	if (rsrc_tbl_addr >= fileSize ||
	    rsrc_tbl_size >= fileSize_MAX ||
	    ((rsrc_tbl_addr + rsrc_tbl_size) > fileSize))
	{
		// Starting address is past the end of the file,
		// or resource ends past the end of the file.
		q->m_file.reset();
		q->m_lastError = -EIO;
		return;
	}

	// Load the resource table.
	int ret = loadResTbl();
	if (ret != 0) {
		// No resources, or an error occurred.
		q->m_file.reset();
	}
}

/**
 * Load the resource table.
 *
 * NOTE: Only numeric resources are loaded.
 * Named resources are ignored.
 *
 * @param addr	[in] Starting address of the directory. (relative to the start of .rsrc)
 * @param dir	[out] Resource directory.
 * @return Number of entries loaded, or negative POSIX error code on error.
 */
int NEResourceReaderPrivate::loadResTbl(void)
{
	RP_Q(NEResourceReader);
	// Reference: https://www.x-ways.net/winhex/kb/ff/NE_EXE.txt

	// Resource table layout:
	// WORD	    rscAlignShift;
	// TYPEINFO rscTypes[];
	// WORD     rscEndTypes;
	// BYTE     rscResourceNames[];
	// BYTE     rscEndNames;

	// We're only interested in the first three sections.

	// Load the resource table.
	unique_ptr<uint8_t[]> rsrcTblData(new uint8_t[rsrc_tbl_size]);
	size_t size = q->m_file->seekAndRead(rsrc_tbl_addr, rsrcTblData.get(), rsrc_tbl_size);
	if (size != rsrc_tbl_size) {
		// Seek and/or read error.
		q->m_lastError = q->m_file->lastError();
		return q->m_lastError;
	}

	// Get the shift alignment. (power of 2)
	const uint16_t rscAlignShift = (rsrcTblData[0] | (rsrcTblData[1] << 8));
	if (rscAlignShift >= 16) {
		// 64 KB or higher shift alignment is
		// probably out of range.
		return -EIO;
	}
	unsigned int pos = 2;

	// Initialize the resource types list.
	res_types.clear();

	// TODO: Overflow prevention.
	// TODO: Use pointers for pos and endpos?
	int ret = -EIO;
	while (pos < rsrc_tbl_size) {
		// Read the next type ID.
		if ((pos + 2) >= rsrc_tbl_size) {
			// I/O error; should be at least 2 bytes left...
			break;
		}
		const NE_TYPEINFO *typeInfo = reinterpret_cast<const NE_TYPEINFO*>(&rsrcTblData[pos]);
		const uint16_t rtTypeID = le16_to_cpu(typeInfo->rtTypeID);
		if (rtTypeID == 0) {
			// End of rscTypes[].
			ret = 0;
			break;
		}

		// FIXME: If !(rtTypeId & 0x8000), it's a named type
		// and should be skipped. (Or, keep it and add named
		// lookup later?)

		// Must have at least 6 bytes for the resource type information.
		if ((pos + sizeof(NE_TYPEINFO)) >= rsrc_tbl_size) {
			// I/O error; not enough space for NE_TYPEINFO.
			break;
		}
		pos += sizeof(NE_TYPEINFO);

		// Check if the resource type already exists.
		auto iter_find = res_types.find(rtTypeID);
		assert(iter_find == res_types.end());
		if (iter_find != res_types.end()) {
			// Multiple table entries for the same resource type.
			break;
		}

		// Create a new vector for the resource type.
		auto iter_type = res_types.emplace(rtTypeID, rsrc_dir_t());
		assert(iter_type.second);
		if (!iter_type.second) {
			// Error adding to the map.
			ret = -ENOMEM;
			break;
		}
		rsrc_dir_t &dir = iter_type.first->second;
		const unsigned int resCount = le16_to_cpu(typeInfo->rtResourceCount);
		dir.resize(resCount);
		unsigned int entriesRead = 0;
		bool isErr = false;
		for (unsigned int i = 0; i < resCount; i++) {
			// Read a NAMEINFO struct.
			if ((pos + sizeof(NE_NAMEINFO)) >= rsrc_tbl_size) {
				// I/O error; not enough space for NE_NAMEINFO.
				isErr = true;
				break;
			}
			const NE_NAMEINFO *nameInfo = reinterpret_cast<const NE_NAMEINFO*>(&rsrcTblData[pos]);
			pos += sizeof(NE_NAMEINFO);

			const uint16_t rnID = le16_to_cpu(nameInfo->rnID);
			if (!(rnID & 0x8000)) {
				// Resource name is a string. Not supported.
				continue;
			}

			// Add the resource information.
			auto &entry = dir[entriesRead];
			entry.id = rnID;
			// NOTE: Wine shifts both addr and len; all documentation
			// I can find says only addr is shifted, but then the len
			// value is too small...
			entry.addr = le32_to_cpu(nameInfo->rnOffset) << rscAlignShift;
			entry.len = le16_to_cpu(nameInfo->rnLength) << rscAlignShift;
			entriesRead++;
		}
		if (isErr)
			break;

		// Shrink the vector in case we skipped some entries.
		dir.resize(entriesRead);
	}

	return ret;
}

/**
 * Read the section header in an NE version resource.
 *
 * The file pointer will be advanced past the header.
 *
 * @param file		[in] PE version resource.
 * @param key		[in] Expected header name.
 * @param pChildLen	[out,opt] Total length of the section.
 * @param pValueLen	[out,opt] Value length.
 * @return 0 if the header matches; non-zero on error.
 */
int NEResourceReaderPrivate::load_VS_VERSION_INFO_header(IRpFile *file, const char *key, uint16_t *pLen, uint16_t *pValueLen)
{
	// Read fields.
	uint16_t fields[2];	// wLength, wValueLength
	size_t size = file->read(fields, sizeof(fields));
	if (size != sizeof(fields)) {
		// Read error.
		return -EIO;
	}

	// Check the key name.
	// NOTE: NE uses SBCS/MBCS/DBCS, so the length is in bytes.
	const unsigned int key_len = static_cast<unsigned int>(strlen(key));
	// DWORD alignment: Make sure we end on a multiple of 4 bytes.
	// NOTE: sizeof(fields) == 4, so it's already WORD-aligned.
	unsigned int keyData_len = key_len + 1;
	keyData_len = ALIGN_BYTES(4, keyData_len);
	unique_ptr<char[]> keyData(new char[keyData_len]);
	size = file->read(keyData.get(), keyData_len);
	if (size != keyData_len) {
		// Read error.
		return -EIO;
	}

	// Verify that the strings are equal.
	if (strncmp(keyData.get(), key, key_len) != 0) {
		// Key mismatch.
		return -EIO;
	}
	if (keyData[key_len] != 0) {
		// Not NULL terminated.
		return -EIO;
	}

	// Header read successfully.
	*pLen = le16_to_cpu(fields[0]);
	*pValueLen = le16_to_cpu(fields[1]);
	return 0;
}

/**
 * Load a string table.
 * @param file		[in] PE version resource.
 * @param st		[out] String Table.
 * @param langID	[out] Language ID.
 * @return 0 on success; non-zero on error.
 */
int NEResourceReaderPrivate::load_StringTable(IRpFile *file, IResourceReader::StringTable &st, uint32_t *langID)
{
	// References:
	// - String: https://docs.microsoft.com/en-us/windows/win32/menurc/string-str
	// - StringTable: https://docs.microsoft.com/en-us/windows/win32/menurc/stringtable

	// NOTE: 16-bit version resources use DWORD alignment, not WORD alignment.
	// I'm guessing this is because it was originally developed for Windows NT.
	// Reference: https://devblogs.microsoft.com/oldnewthing/20061220-15/?p=28653

	// Read fields.
	const off64_t pos_start = file->tell();
	uint16_t fields[2];	// wLength, wValueLength
	size_t size = file->read(fields, sizeof(fields));
	if (size != sizeof(fields)) {
		// Read error.
		return -EIO;
	}

	// wLength contains the total string table length.
	// wValueLength should be 0.
	if (fields[1] != cpu_to_le16(0)) {
		// Not a string table.
		return -EIO;
	}

	// Read the 8-character language ID.
	// Format: 040904E4
	// - 0409: Language (US English)
	// - 04E4: Code page (1252)
	char s_langID[9];
	size = file->read(s_langID, sizeof(s_langID));
	if (size != sizeof(s_langID) || s_langID[8] != 0) {
		// Read error, or not NULL terminated.
		return -EIO;
	}

	// Parse using strtoul().
	char *endptr;
	*langID = static_cast<unsigned int>(strtoul(s_langID, &endptr, 16));
	if (*langID == 0 || endptr != &s_langID[8]) {
		// Not valid.
		// TODO: Better error code?
		return -EIO;
	}

	// Get the code page from the language ID.
	uint16_t codepage = (*langID & 0xFFFF);
	assert(codepage != 0);
	if (codepage == 0) {
		// TODO: More extensive code page validation?
		codepage = 1252;
	}

	// DWORD alignment.
	IResourceReader::alignFileDWORD(file);

	// Total string table size (in bytes) is wLength - (pos_strings - pos_start).
	const off64_t pos_strings = file->tell();
	const int strTblData_len = static_cast<int>(fields[0]) - static_cast<int>(pos_strings - pos_start);
	if (strTblData_len <= 0) {
		// Error...
		return -EIO;
	}

	// Read the string table.
	unique_ptr<uint8_t[]> strTblData(new uint8_t[strTblData_len]);
	size = file->read(strTblData.get(), strTblData_len);
	if (size != static_cast<size_t>(strTblData_len)) {
		// Read error.
		return -EIO;
	}

	// Parse the string table.
	// TODO: Optimizations.
	st.clear();
	int tblPos = 0;
	while (tblPos < strTblData_len) {
		// wLength, wValueLength
		memcpy(fields, &strTblData[tblPos], sizeof(fields));

		const uint16_t wLength = le16_to_cpu(fields[0]);
		if (wLength < sizeof(fields)) {
			// Invalid length.
			return -EIO;
		}
		const uint16_t wValueLength = le16_to_cpu(fields[1]);
		if (wValueLength >= wLength || wLength > (strTblData_len - tblPos)) {
			// Not valid.
			return -EIO;
		}

		// Key length, in bytes: wLength - wValueLength - sizeof(fields)
		// Last character must be NULL.
		tblPos += sizeof(fields);
		const int key_len = (wLength - wValueLength - sizeof(fields)) - 1;
		if (key_len <= 0) {
			// Invalid key length.
			return -EIO;
		}
		const char *key = reinterpret_cast<const char*>(&strTblData[tblPos]);
		if (key[key_len] != 0) {
			// Not NULL-terminated.
			return -EIO;
		}

		// DWORD alignment is required here.
		tblPos += (key_len + 1);
		tblPos  = ALIGN_BYTES(4, tblPos);

		// Value must be NULL-terminated.
		const char *value = reinterpret_cast<const char*>(&strTblData[tblPos]);
		const int value_len = wValueLength - 1;
		if (value_len <= 0) {
			// Empty value.
			static const char str_empty[1] = {0};
			value = str_empty;
		} else if (value[value_len] != 0) {
			// Not NULL-terminated.
			return -EIO;
		}

		string key_utf8 = cpN_to_utf8(codepage, key, key_len);
		assert(!key_utf8.empty());
		if (key_utf8.empty()) {
			// Code page conversion failed.
			// Default to 1252.
			key_utf8 = cp1252_to_utf8(key, key_len);
		}

		string value_utf8;
		if (value_len > 0) {
			value_utf8 = cpN_to_utf8(codepage, value, value_len);
			assert(!value_utf8.empty());
			if (value_utf8.empty()) {
				// Code page conversion failed.
				// Default to 1252.
				value_utf8 = cp1252_to_utf8(value, value_len);
			}
		}

		// NOTE: Only converting the value from DOS to UNIX line endings.
		// The key shouldn't have newlines.
		st.emplace_back(std::move(key_utf8), dos2unix(value_utf8));

		// DWORD alignment is required here.
		tblPos += wValueLength;
		tblPos  = ALIGN_BYTES(4, tblPos);
	}

	// String table loaded successfully.
	return 0;
}

/** NEResourceReader **/

/**
 * Construct an NEResourceReader with the specified IRpFile.
 *
 * NOTE: The IRpFile *must* remain valid while this
 * NEResourceReader is open.
 *
 * @param file IRpFile.
 * @param rsrc_tbl_addr Resource table start address.
 * @param rsrc_tbl_size Resource table size.
 */
NEResourceReader::NEResourceReader(const IRpFilePtr &file, uint32_t rsrc_tbl_addr, uint32_t rsrc_tbl_size)
	: super(file)
	, d_ptr(new NEResourceReaderPrivate(this, rsrc_tbl_addr, rsrc_tbl_size))
{ }

NEResourceReader::~NEResourceReader()
{
	delete d_ptr;
}

/** IDiscReader **/

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t NEResourceReader::read(void *ptr, size_t size)
{
	assert((bool)m_file);
	assert(m_file->isOpen());
	if (!m_file || !m_file->isOpen()) {
		m_lastError = EBADF;
		return 0;
	}

	// There isn't a separate resource "section" in
	// NE executables, so forward all read requests
	// to the underlying file.
	return m_file->read(ptr, size);
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int NEResourceReader::seek(off64_t pos)
{
	assert((bool)m_file);
	assert(m_file->isOpen());
	if (!m_file || !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// There isn't a separate resource "section" in
	// NE executables, so forward all read requests
	// to the underlying file.
	return m_file->seek(pos);
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
off64_t NEResourceReader::tell(void)
{
	assert((bool)m_file);
	assert(m_file->isOpen());
	if (!m_file || !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// There isn't a separate resource "section" in
	// NE executables, so forward all read requests
	// to the underlying file.
	return m_file->tell();
}

/**
 * Get the data size.
 * This size does not include the partition header,
 * and it's adjusted to exclude hashes.
 * @return Data size, or -1 on error.
 */
off64_t NEResourceReader::size(void)
{
	// TODO: Errors?
	assert((bool)m_file);
	assert(m_file->isOpen());
	if (!m_file || !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// There isn't a separate resource "section" in
	// NE executables, so forward all read requests
	// to the underlying file.
	return static_cast<off64_t>(m_file->size());
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
off64_t NEResourceReader::partition_size(void) const
{
	// TODO: Errors?

	// There isn't a separate resource "section" in
	// NE executables, so forward all read requests
	// to the underlying file.
	return static_cast<off64_t>(m_file->size());
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
off64_t NEResourceReader::partition_size_used(void) const
{
	// TODO: Errors?

	// There isn't a separate resource "section" in
	// NE executables, so forward all read requests
	// to the underlying file.
	return static_cast<off64_t>(m_file->size());
}

/** Resource access functions **/

/**
 * Open a resource.
 * @param type Resource type ID.
 * @param id Resource ID. (-1 for "first entry")
 * @param lang Language ID. (-1 for "first entry")
 * @return IRpFile*, or nullptr on error.
 */
IRpFilePtr NEResourceReader::open(uint16_t type, int id, int lang)
{
	RP_D(NEResourceReader);

	// NOTE: The language ID is not used in NE resources.
	RP_UNUSED(lang);

	// NOTE: Type and resource IDs have the high bit set for integers.
	// We're only supporting integer IDs, so set the high bits here.
	type |= 0x8000;
	id |= 0x8000;

	// Get the directory for the specified type.
	auto iter_dir = d->res_types.find(type);
	if (iter_dir == d->res_types.end())
		return nullptr;
	auto &dir = iter_dir->second;
	if (dir.empty())
		return nullptr;
	
	const NEResourceReaderPrivate::ResTblEntry *entry = nullptr;
	if (id == -1) {
		// Get the first ID for this type.
		entry = &dir[0];
	} else {
		// Search for the ID.
		// TODO: unordered_map?
		auto iter = std::find_if(dir.cbegin(), dir.cend(),
			[id](const NEResourceReaderPrivate::ResTblEntry &entry) noexcept -> bool {
				return (entry.id == static_cast<uint16_t>(id));
			}
		);
		if (iter != dir.cend()) {
			entry = &(*iter);
		} else {
			// ID not found.
			return nullptr;
		}
	}

	// Create the PartitionFile.
	// This is an IRpFile implementation that uses an
	// IPartition as the reader and takes an offset
	// and size as the file parameters.
	// TODO: Set the codepage somewhere?
	return std::make_shared<PartitionFile>(this, entry->addr, entry->len);
}

/**
 * Load a VS_VERSION_INFO resource.
 * Data will be byteswapped to host-endian if necessary.
 * @param id		[in] Resource ID. (-1 for "first entry")
 * @param lang		[in] Language ID. (-1 for "first entry")
 * @param pVsFfi	[out] VS_FIXEDFILEINFO (host-endian)
 * @param pVsSfi	[out] StringFileInfo section.
 * @return 0 on success; non-zero on error.
 */
int NEResourceReader::load_VS_VERSION_INFO(int id, int lang, VS_FIXEDFILEINFO *pVsFfi, StringFileInfo *pVsSfi)
{
	assert(pVsFfi != nullptr);
	assert(pVsSfi != nullptr);
	if (!pVsFfi || !pVsSfi) {
		// Invalid parameters.
		return -EINVAL;
	}

	// Open the VS_VERSION_INFO resource.
	const IRpFilePtr f_ver(this->open(RT_VERSION, id, lang));
	if (!f_ver) {
		// Not found.
		return -ENOENT;
	}

	// Read the version header.
	static const char vsvi[] = "VS_VERSION_INFO";
	uint16_t len, valueLen;
	int ret = NEResourceReaderPrivate::load_VS_VERSION_INFO_header(f_ver.get(), vsvi, &len, &valueLen);
	if (ret != 0) {
		// Header is incorrect.
		return ret;
	}

	// Verify the value size.
	// (Value should be VS_FIXEDFILEINFO.)
	if (valueLen != sizeof(*pVsFfi)) {
		// Wrong size.
		return -EIO;
	}

	// Read the version information.
	size_t size = f_ver->read(pVsFfi, sizeof(*pVsFfi));
	if (size != sizeof(*pVsFfi)) {
		// Read error.
		return -EIO;
	}

	// Verify the signature and structure version.
	pVsFfi->dwSignature	= le32_to_cpu(pVsFfi->dwSignature);
	pVsFfi->dwStrucVersion	= le32_to_cpu(pVsFfi->dwStrucVersion);
	if (pVsFfi->dwSignature != VS_FFI_SIGNATURE ||
	    pVsFfi->dwStrucVersion != VS_FFI_STRUCVERSION)
	{
		// Signature and/or structure version is incorrect.
		// TODO: Better error code?
		return -EIO;
	}

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	// Byteswap the remaining fields.
	pVsFfi->dwFileVersionMS		= le32_to_cpu(pVsFfi->dwFileVersionMS);
	pVsFfi->dwFileVersionLS		= le32_to_cpu(pVsFfi->dwFileVersionLS);
	pVsFfi->dwProductVersionMS	= le32_to_cpu(pVsFfi->dwProductVersionMS);
	pVsFfi->dwProductVersionLS	= le32_to_cpu(pVsFfi->dwProductVersionLS);
	pVsFfi->dwFileFlagsMask		= le32_to_cpu(pVsFfi->dwFileFlagsMask);
	pVsFfi->dwFileFlags		= le32_to_cpu(pVsFfi->dwFileFlags);
	pVsFfi->dwFileOS		= le32_to_cpu(pVsFfi->dwFileOS);
	pVsFfi->dwFileType		= le32_to_cpu(pVsFfi->dwFileType);
	pVsFfi->dwFileSubtype		= le32_to_cpu(pVsFfi->dwFileSubtype);
	pVsFfi->dwFileDateMS		= le32_to_cpu(pVsFfi->dwFileDateMS);
	pVsFfi->dwFileDateLS		= le32_to_cpu(pVsFfi->dwFileDateLS);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

	// DWORD alignment, if necessary.
	alignFileDWORD(f_ver.get());

	// Read the StringFileInfo section header.
	static const char vssfi[] = "StringFileInfo";
	ret = NEResourceReaderPrivate::load_VS_VERSION_INFO_header(f_ver.get(), vssfi, &len, &valueLen);
	if (ret != 0) {
		// No StringFileInfo section.
		return 0;
	}

	// Read a string table.
	// TODO: Verify StringFileInfo length.
	// May need to skip over additional string tables
	// in order to read VarFileInfo.
	StringTable st;
	uint32_t langID;
	ret = NEResourceReaderPrivate::load_StringTable(f_ver.get(), st, &langID);
	if (ret == 0) {
		// String table read successfully.
		pVsSfi->emplace(langID, std::move(st));
	}

	// Version information read successfully.
	return 0;
}

}
