/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PEResourceReader.cpp: Portable Executable resource reader.              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "PEResourceReader.hpp"

#include "../Other/exe_pe_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::string;
using std::unique_ptr;
using std::unordered_map;

// Uninitialized vector class
#include "uvector.h"

namespace LibRomData {

class PEResourceReaderPrivate
{
public:
	PEResourceReaderPrivate(PEResourceReader *q,
		uint32_t rsrc_addr, uint32_t rsrc_size, uint32_t rsrc_va);

private:
	RP_DISABLE_COPY(PEResourceReaderPrivate)
protected:
	PEResourceReader *const q_ptr;

public:
	// Read position
	off64_t pos;

	// .rsrc section
	uint32_t rsrc_addr;
	uint32_t rsrc_size;
	uint32_t rsrc_va;

	// Resource directory entry
	struct ResDirEntry {
		uint16_t id;	// Resource ID
		uint32_t addr;	// Address of the IMAGE_RESOURCE_DIRECTORY or
				// IMAGE_RESOURCE_DATA_ENTRY, relative to rsrc_addr.
				// NOTE: If the high bit is set, this is a subdirectory.
	};
	typedef rp::uvector<ResDirEntry> rsrc_dir_t;

	// Resource types (Top-level directory)
	rsrc_dir_t res_types;

	// Cached top-level directories (type)
	// Key: type
	// Value: Resources contained within the directory.
	unordered_map<uint16_t, rsrc_dir_t> type_dirs;

	// Cached second-level directories (type and ID)
	// Key: LOWORD == type, HIWORD == id
	// Value: Resources contained within the directory.
	unordered_map<uint32_t, rsrc_dir_t> type_and_id_dirs;

	/**
	 * Load a resource directory.
	 *
	 * NOTE: Only numeric resources and/or subdirectories are loaded.
	 * Named resources and/or subdirectores are ignored.
	 *
	 * @param addr	[in] Starting address of the directory. (relative to the start of .rsrc)
	 * @param dir	[out] Resource directory.
	 * @return Number of entries loaded, or negative POSIX error code on error.
	 */
	int loadResDir(uint32_t addr, rsrc_dir_t &dir);

	/**
	 * Get the resource directory for the specified type.
	 * @param type Resource type.
	 * @return Resource directory, or nullptr if not found.
	 */
	const rsrc_dir_t *getTypeDir(uint16_t type);

	/**
	 * Get the resource directory for the specified type and ID.
	 * @param type Resource type.
	 * @param id Resource ID.
	 * @return Resource directory, or nullptr if not found.
	 */
	const rsrc_dir_t *getTypeIdDir(uint16_t type, uint16_t id);

	/**
	 * Read the section header in a PE version resource.
	 *
	 * The file pointer will be advanced past the header.
	 *
	 * @param file		[in] PE version resource.
	 * @param key		[in] Expected header name.
	 * @param type		[in] Expected data type. (0 == binary, 1 == text)
	 * @param pChildLen	[out,opt] Total length of the section.
	 * @param pValueLen	[out,opt] Value length.
	 * @return 0 if the header matches; non-zero on error.
	 */
	static int load_VS_VERSION_INFO_header(IRpFile *file, const char16_t *key, uint16_t type, uint16_t *pLen, uint16_t *pValueLen);

	/**
	 * Load a string table.
	 * @param file		[in] PE version resource.
	 * @param st		[out] String Table.
	 * @param langID	[out] Language ID.
	 * @return 0 on success; non-zero on error.
	 */
	static int load_StringTable(IRpFile *file, IResourceReader::StringTable &st, uint32_t *langID);
};

/** PEResourceReaderPrivate **/

PEResourceReaderPrivate::PEResourceReaderPrivate(
		PEResourceReader *q,
		uint32_t rsrc_addr, uint32_t rsrc_size,
		uint32_t rsrc_va)
	: q_ptr(q)
	, pos(0)
	, rsrc_addr(rsrc_addr)
	, rsrc_size(rsrc_size)
	, rsrc_va(rsrc_va)
{
	if (!q->m_file) {
		q->m_lastError = -EBADF;
		return;
	} else if (rsrc_addr == 0 || rsrc_size == 0) {
		q->m_file.reset();
		q->m_lastError = -EIO;
		return;
	}

	// Validate the starting address and size.
	static constexpr uint32_t fileSize_MAX = 2U*1024*1024*1024;
	const off64_t fileSize_o64 = q->m_file->size();
	if (fileSize_o64 > fileSize_MAX) {
		// A Win32/Win64 executable larger than 2 GB doesn't make any sense.
		q->m_file.reset();
		q->m_lastError = -EIO;
		return;
	}

	const uint32_t fileSize = static_cast<uint32_t>(fileSize_o64);
	if (rsrc_addr >= fileSize ||
	    rsrc_size >= fileSize_MAX ||
	    ((rsrc_addr + rsrc_size) > fileSize))
	{
		// Starting address is past the end of the file,
		// or resource ends past the end of the file.
		q->m_file.reset();
		q->m_lastError = -EIO;
		return;
	}

	// Load the root resource directory.
	int ret = loadResDir(0, res_types);
	if (ret <= 0) {
		// No resources, or an error occurred.
		q->m_file.reset();
	}
}

/**
 * Load a resource directory.
 *
 * NOTE: Only numeric resources and/or subdirectories are loaded.
 * Named resources and/or subdirectores are ignored.
 *
 * @param addr	[in] Starting address of the directory. (relative to the start of .rsrc)
 * @param dir	[out] Resource directory.
 * @return Number of entries loaded, or negative POSIX error code on error.
 */
int PEResourceReaderPrivate::loadResDir(uint32_t addr, rsrc_dir_t &dir)
{
	RP_Q(PEResourceReader);

	IMAGE_RESOURCE_DIRECTORY root;
	size_t size = q->m_file->seekAndRead(rsrc_addr + addr, &root, sizeof(root));
	if (size != sizeof(root)) {
		// Seek and/or read error.
		q->m_lastError = q->m_file->lastError();
		return q->m_lastError;
	}

	// Total number of entries.
	unsigned int entryCount = le16_to_cpu(root.NumberOfNamedEntries) + le16_to_cpu(root.NumberOfIdEntries);
	assert(entryCount <= 8192);
	if (entryCount > 8192) {
		// Sanity check; constrain to 64 entries.
		entryCount = 8192;
	}
	uint32_t szToRead = static_cast<uint32_t>(entryCount * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));
	unique_ptr<IMAGE_RESOURCE_DIRECTORY_ENTRY[]> irdEntries(new IMAGE_RESOURCE_DIRECTORY_ENTRY[entryCount]);
	size = q->m_file->read(irdEntries.get(), szToRead);
	if (size != szToRead) {
		// Read error.
		q->m_lastError = q->m_file->lastError();
		return q->m_lastError;
	}

	// Read each directory header.
	dir.resize(entryCount);
	const IMAGE_RESOURCE_DIRECTORY_ENTRY *irdEntry = irdEntries.get();
	unsigned int entriesRead = 0;
	for (unsigned int i = 0; i < entryCount; i++, irdEntry++) {
		// Skipping any root directory entry that isn't an ID.
		const uint32_t id = le32_to_cpu(irdEntry->Name);
		if (id > 0xFFFF) {
			// Not an ID.
			continue;
		}

		// Entry data.
		auto &entry = dir[entriesRead];
		entry.id = static_cast<uint16_t>(id);
		// addr points to IMAGE_RESOURCE_DIRECTORY
		// or IMAGE_RESOURCE_DATA_ENTRY.
		entry.addr = le32_to_cpu(irdEntry->OffsetToData);

		// Next entry.
		entriesRead++;
	}

	// Shrink the vector in case we skipped some types.
	dir.resize(entriesRead);
	return (int)entriesRead;
}

/**
 * Get the resource directory for the specified type.
 * @param type Resource type.
 * @return Resource directory, or nullptr if not found.
 */
const PEResourceReaderPrivate::rsrc_dir_t *PEResourceReaderPrivate::getTypeDir(uint16_t type)
{
	// Check if the type is already cached.
	auto iter_td = type_dirs.find(type);
	if (iter_td != type_dirs.end()) {
		// Type is already cached.
		return &(iter_td->second);
	}

	// Not cached. Find the type in the root directory.
	auto iter = std::find_if(res_types.cbegin(), res_types.cend(),
		[type](const ResDirEntry &entry) noexcept -> bool {
			return (entry.id == type);
		}
	);

	uint32_t type_addr = 0;
	if (iter != res_types.cend()) {
		// Make sure this type is a subdirectory.
		if (iter->addr & 0x80000000) {
			// This is a subdirectory.
			type_addr = (iter->addr & ~0x80000000);
		}
	}

	if (type_addr == 0) {
		// Not found.
		return nullptr;
	}

	// Load the directory.
	auto iter_type = type_dirs.emplace(type, rsrc_dir_t());
	if (!iter_type.second) {
		// Error adding to the map.
		return nullptr;
	}
	rsrc_dir_t *type_dir = &(iter_type.first->second);
	loadResDir(type_addr, *type_dir);
	return type_dir;
}

/**
 * Get the resource directory for the specified type and ID.
 * @param type Resource type.
 * @param id Resource ID.
 * @return Resource directory, or nullptr if not found.
 */
const PEResourceReaderPrivate::rsrc_dir_t *PEResourceReaderPrivate::getTypeIdDir(uint16_t type, uint16_t id)
{
	// Check if the type and ID is already cached.
	const uint32_t type_and_id = (type | (static_cast<uint32_t>(id) << 16));
	auto iter_td = type_and_id_dirs.find(type_and_id);
	if (iter_td != type_and_id_dirs.end()) {
		// Type and ID is already cached.
		return &(iter_td->second);
	}

	// Not cached. Find the type in the root directory.
	const rsrc_dir_t *type_dir = getTypeDir(type);
	if (!type_dir) {
		// Type not found.
		return nullptr;
	}

	// Find the ID in the type directory.
	auto iter = std::find_if(type_dir->cbegin(), type_dir->cend(),
		[id](const ResDirEntry &entry) noexcept -> bool {
			return (entry.id == id);
		}
	);

	uint32_t id_addr = 0;
	if (iter != type_dir->cend()) {
		// Make sure this ID is a subdirectory.
		if (iter->addr & 0x80000000) {
			// This is a subdirectory.
			id_addr = (iter->addr & ~0x80000000);
		}
	}

	if (id_addr == 0) {
		// Not found.
		return nullptr;
	}

	// Load the directory.
	auto iter_type_and_id = type_and_id_dirs.emplace(type_and_id, rsrc_dir_t());
	if (!iter_type_and_id.second) {
		// Error adding to the map.
		return nullptr;
	}
	rsrc_dir_t *id_dir = &(iter_type_and_id.first->second);
	loadResDir(id_addr, *id_dir);
	return id_dir;
}

/**
 * Read the section header in a PE version resource.
 *
 * The file pointer will be advanced past the header.
 *
 * @param file		[in] PE version resource.
 * @param key		[in] Expected header name.
 * @param type		[in] Expected data type. (0 == binary, 1 == text)
 * @param pChildLen	[out] Total length of the section.
 * @param pValueLen	[out] Value length.
 * @return 0 if the header matches; non-zero on error.
 */
int PEResourceReaderPrivate::load_VS_VERSION_INFO_header(IRpFile *file, const char16_t *key, uint16_t type, uint16_t *pLen, uint16_t *pValueLen)
{
	// Read fields.
	uint16_t fields[3];	// wLength, wValueLength, wType
	size_t size = file->read(fields, sizeof(fields));
	if (size != sizeof(fields)) {
		// Read error.
		return -EIO;
	}

	// Validate the data type.
	assert(type == 0 || type == 1);
	if (type != le16_to_cpu(fields[2])) {
		// Wrong data type.
		return -EIO;
	}

	// Check the key name.
	const unsigned int key_len = static_cast<unsigned int>(u16_strlen(key));
	// DWORD alignment: Make sure we end on a multiple of 4 bytes.
	unsigned int keyData_len = (key_len+1) * sizeof(char16_t);
	keyData_len = ALIGN_BYTES(4, keyData_len + sizeof(fields)) - sizeof(fields);
	unique_ptr<char16_t[]> keyData(new char16_t[keyData_len/sizeof(char16_t)]);
	size = file->read(keyData.get(), keyData_len);
	if (size != keyData_len) {
		// Read error.
		return -EIO;
	}

	// Verify that the strings are equal.
	// NOTE: Win32 is always UTF-16LE, so we have to
	// adjust for endianness.
	const char16_t *pKeyData = keyData.get();
	for (unsigned int i = key_len; i > 0; i--, pKeyData++, key++) {
		if (le16_to_cpu(*pKeyData) != *key) {
			// Key mismatch.
			return -EIO;
		}
	}
	if (*pKeyData != 0) {
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
int PEResourceReaderPrivate::load_StringTable(IRpFile *file, IResourceReader::StringTable &st, uint32_t *langID)
{
	// References:
	// - String: https://docs.microsoft.com/en-us/windows/win32/menurc/string-str
	// - StringTable: https://docs.microsoft.com/en-us/windows/win32/menurc/stringtable

	// Read fields.
	const off64_t pos_start = file->tell();
	uint16_t fields[3];	// wLength, wValueLength, wType
	size_t size = file->read(fields, sizeof(fields));
	if (size != sizeof(fields)) {
		// Read error.
		return -EIO;
	}

	// wLength contains the total string table length.
	// wValueLength should be 0.
	// wType should be 1, indicating a language ID string.
	if (fields[1] != cpu_to_le16(0) || fields[2] != cpu_to_le16(1)) {
		// Not a string table.
		return -EIO;
	}

	// Read the 8-character language ID.
	char16_t s_langID[9];
	size = file->read(s_langID, sizeof(s_langID));
	if (size != sizeof(s_langID) ||
	   (s_langID[8] != cpu_to_le16(0)))
	{
		// Read error, or not NULL terminated.
		return -EIO;
	}

	// Convert to UTF-8.
	const string str_langID = utf16le_to_utf8(s_langID, 8);
	// Parse using strtoul().
	char *endptr;
	*langID = static_cast<unsigned int>(strtoul(str_langID.c_str(), &endptr, 16));
	if (*langID == 0 || endptr != (str_langID.c_str() + 8)) {
		// Not valid.
		// TODO: Better error code?
		return -EIO;
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
	// DWORD alignment.
	IResourceReader::alignFileDWORD(file);

	// Parse the string table.
	// TODO: Optimizations.
	st.clear();
	int tblPos = 0;
	while (tblPos < strTblData_len) {
		// wLength, wValueLength, wType
		memcpy(fields, &strTblData[tblPos], sizeof(fields));
		if (fields[2] != cpu_to_le16(1)) {
			// Not a string...
			return -EIO;
		}
		// NOTE: wValueLength is number of *words* (aka UTF-16 characters).
		// Hence, we're multiplying by two to get bytes.
		const uint16_t wLength = le16_to_cpu(fields[0]);
		if (wLength < sizeof(fields)) {
			// Invalid length.
			return -EIO;
		}
		const int wValueLength = le16_to_cpu(fields[1]) * sizeof(char16_t);
		if (wValueLength >= wLength || wLength > (strTblData_len - tblPos)) {
			// Not valid.
			return -EIO;
		}

		// Key length, in bytes: wLength - wValueLength - sizeof(fields)
		// Last Unicode character must be NULL.
		tblPos += sizeof(fields);
		const int key_len = ((wLength - wValueLength - sizeof(fields)) / sizeof(char16_t)) - 1;
		if (key_len <= 0) {
			// Invalid key length.
			return -EIO;
		}
		const char16_t *key = reinterpret_cast<const char16_t*>(&strTblData[tblPos]);
		if (key[key_len] != cpu_to_le16(0)) {
			// Not NULL-terminated.
			return -EIO;
		}

		// DWORD alignment is required here.
		tblPos += ((key_len + 1) * 2);
		tblPos  = ALIGN_BYTES(4, tblPos);

		// Value must be NULL-terminated.
		const char16_t *value = reinterpret_cast<const char16_t*>(&strTblData[tblPos]);
		const int value_len = (wValueLength / 2) - 1;
		if (value_len <= 0) {
			// Empty value.
			static constexpr char16_t u16_empty[1] = {0};
			value = u16_empty;
		} else if (value[value_len] != cpu_to_le16(0)) {
			// Not NULL-terminated.
			return -EIO;
		}

		// NOTE: Only converting the value from DOS to UNIX line endings.
		// The key shouldn't have newlines.
		st.emplace_back(utf16le_to_utf8(key, key_len),
			dos2unix(utf16le_to_utf8(value, value_len)));

		// DWORD alignment is required here.
		tblPos += wValueLength;
		tblPos  = ALIGN_BYTES(4, tblPos);
	}

	// String table loaded successfully.
	return 0;
}

/** PEResourceReader **/

/**
 * Construct a PEResourceReader with the specified IRpFile.
 *
 * NOTE: The IRpFile *must* remain valid while this
 * PEResourceReader is open.
 *
 * @param file IRpFile.
 * @param rsrc_addr .rsrc section start offset.
 * @param rsrc_size .rsrc section size.
 * @param rsrc_va .rsrc virtual address.
 */
PEResourceReader::PEResourceReader(const IRpFilePtr &file, uint32_t rsrc_addr, uint32_t rsrc_size, uint32_t rsrc_va)
	: super(file)
	, d_ptr(new PEResourceReaderPrivate(this, rsrc_addr, rsrc_size, rsrc_va))
{ }

PEResourceReader::~PEResourceReader()
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
size_t PEResourceReader::read(void *ptr, size_t size)
{
	RP_D(PEResourceReader);
	assert((bool)m_file);
	assert(m_file->isOpen());
	if (!m_file || !m_file->isOpen()) {
		m_lastError = EBADF;
		return 0;
	}

	// Are we already at the end of the .rsrc section?
	if (d->pos >= static_cast<off64_t>(d->rsrc_size)) {
		// End of the .rsrc section.
		return 0;
	}

	// Make sure d->pos + size <= d->rsrc_size.
	// If it isn't, we'll do a short read.
	if (d->pos + static_cast<off64_t>(size) >= static_cast<off64_t>(d->rsrc_size)) {
		size = static_cast<size_t>(static_cast<off64_t>(d->rsrc_size) - d->pos);
	}

	// Read the data.
	size_t read = m_file->seekAndRead(static_cast<off64_t>(d->rsrc_addr) + static_cast<off64_t>(d->pos), ptr, size);
	if (read != size) {
		// Seek and/or read error.
		m_lastError = m_file->lastError();
	}
	d->pos += read;
	return read;
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int PEResourceReader::seek(off64_t pos)
{
	RP_D(PEResourceReader);
	assert((bool)m_file);
	assert(m_file->isOpen());
	if (!m_file || !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// Handle out-of-range cases.
	if (pos < 0) {
		// Negative is invalid.
		m_lastError = EINVAL;
		return -1;
	} else if (pos >= static_cast<off64_t>(d->rsrc_size)) {
		d->pos = static_cast<off64_t>(d->rsrc_size);
	} else {
		d->pos = pos;
	}
	return 0;
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
off64_t PEResourceReader::tell(void)
{
	RP_D(const PEResourceReader);
	assert((bool)m_file);
	assert(m_file->isOpen());
	if (!m_file || !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	return d->pos;
}

/**
 * Get the data size.
 * This size does not include the partition header,
 * and it's adjusted to exclude hashes.
 * @return Data size, or -1 on error.
 */
off64_t PEResourceReader::size(void)
{
	// TODO: Errors?
	RP_D(const PEResourceReader);
	return static_cast<off64_t>(d->rsrc_size);
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
off64_t PEResourceReader::partition_size(void) const
{
	// TODO: Errors?
	RP_D(const PEResourceReader);
	return static_cast<off64_t>(d->rsrc_size);
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
off64_t PEResourceReader::partition_size_used(void) const
{
	RP_D(const PEResourceReader);
	return static_cast<off64_t>(d->rsrc_size);
}

/** Resource access functions **/

/**
 * Open a resource.
 * @param type Resource type ID.
 * @param id Resource ID. (-1 for "first entry")
 * @param lang Language ID. (-1 for "first entry")
 * @return IRpFile*, or nullptr on error.
 */
IRpFilePtr PEResourceReader::open(uint16_t type, int id, int lang)
{
	// Check if the directory has been cached.
	RP_D(PEResourceReader);

	if (id == -1) {
		// Get the first ID for this type.
		const PEResourceReaderPrivate::rsrc_dir_t *type_dir = d->getTypeDir(type);
		if (!type_dir || type_dir->empty())
			return nullptr;
		id = type_dir->at(0).id;
	}

	// Get the directory for the type and ID.
	const PEResourceReaderPrivate::rsrc_dir_t *type_id_dir =
		d->getTypeIdDir(type, static_cast<uint16_t>(id));
	if (!type_id_dir || type_id_dir->empty())
		return nullptr;

	const PEResourceReaderPrivate::ResDirEntry *dirEntry = nullptr;
	if (lang == -1) {
		// Get the first language for this type.
		dirEntry = &type_id_dir->at(0);
	} else {
		// Find the specified language ID.
		auto iter = std::find_if(type_id_dir->cbegin(), type_id_dir->cend(),
			[lang](const PEResourceReaderPrivate::ResDirEntry &entry) noexcept -> bool {
				return (entry.id == static_cast<uint16_t>(lang));
			}
		);

		if (iter == type_id_dir->cend()) {
			// Not found.
			return nullptr;
		}

		// Found the language ID.
		dirEntry = &(*iter);
	}

	// Make sure this is a file.
	assert(!(dirEntry->addr & 0x80000000));
	if (dirEntry->addr & 0x80000000) {
		// This is a subdirectory.
		return nullptr;
	}

	// Get the IMAGE_RESOURCE_DATA_ENTRY.
	IMAGE_RESOURCE_DATA_ENTRY irdata;
	size_t size = m_file->seekAndRead(d->rsrc_addr + dirEntry->addr, &irdata, sizeof(irdata));
	if (size != sizeof(irdata)) {
		// Seek and/or read error.
		m_lastError = m_file->lastError();
		return nullptr;
	}

	// NOTE: OffsetToData is an RVA, not relative to the physical address.
	// NOTE: Address 0 in IDiscReader equals rsrc_addr.
	const uint32_t data_addr = le32_to_cpu(irdata.OffsetToData) - d->rsrc_va;

	// Create the PartitionFile.
	// This is an IRpFile implementation that uses an
	// IPartition as the reader and takes an offset
	// and size as the file parameters.
	// TODO: Set the codepage somewhere?
	return std::make_shared<PartitionFile>(this->shared_from_this(), data_addr, le32_to_cpu(irdata.Size));
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
int PEResourceReader::load_VS_VERSION_INFO(int id, int lang, VS_FIXEDFILEINFO *pVsFfi, StringFileInfo *pVsSfi)
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
	static constexpr char16_t vsvi[] = {'V','S','_','V','E','R','S','I','O','N','_','I','N','F','O',0};
	uint16_t len, valueLen;
	int ret = PEResourceReaderPrivate::load_VS_VERSION_INFO_header(f_ver.get(), vsvi, 0, &len, &valueLen);
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
	static constexpr char16_t vssfi[] = {'S','t','r','i','n','g','F','i','l','e','I','n','f','o',0};
	ret = PEResourceReaderPrivate::load_VS_VERSION_INFO_header(f_ver.get(), vssfi, 1, &len, &valueLen);
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
	ret = PEResourceReaderPrivate::load_StringTable(f_ver.get(), st, &langID);
	if (ret == 0) {
		// String table read successfully.
		pVsSfi->emplace(langID, std::move(st));
	}

	// Version information read successfully.
	return 0;
}

}
