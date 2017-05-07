/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PEResourceReader.cpp: Portable Executable resource reader.              *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "PEResourceReader.hpp"
#include "../exe_structs.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "../file/IRpFile.hpp"
#include "PartitionFile.hpp"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData {

class PEResourceReaderPrivate
{
	public:
		PEResourceReaderPrivate(PEResourceReader *q, IRpFile *file,
			uint32_t rsrc_addr, uint32_t rsrc_size, uint32_t rsrc_va);

	private:
		RP_DISABLE_COPY(PEResourceReaderPrivate)
	protected:
		PEResourceReader *const q_ptr;

	public:
		// EXE file.
		IRpFile *file;

		// .rsrc section.
		uint32_t rsrc_addr;
		uint32_t rsrc_size;
		uint32_t rsrc_va;

		// Read position.
		int64_t pos;

		// Resource directory entry.
		struct ResDirEntry {
			uint16_t id;	// Resource ID.
			uint32_t addr;	// Address of the IMAGE_RESOURCE_DIRECTORY or
					// IMAGE_RESOURCE_DATA_ENTRY, relative to rsrc_addr.
					// NOTE: If the high bit is set, this is a subdirectory.
		};
		typedef ao::uvector<ResDirEntry> rsrc_dir_t;

		// Resource types. (Top-level directory.)
		rsrc_dir_t res_types;

		// Cached top-level directories. (type)
		// Key: type
		// Value: Resources contained within the directory.
		unordered_map<uint16_t, rsrc_dir_t> type_dirs;

		// Cached second-level directories. (type and ID)
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
		 * DWORD alignment function.
		 * @param file	[in] File to DWORD align.
		 * @return 0 on success; non-zero on error.
		 */
		static inline int alignFileDWORD(IRpFile *file);

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
		PEResourceReader *q, IRpFile *file,
		uint32_t rsrc_addr, uint32_t rsrc_size,
		uint32_t rsrc_va)
	: q_ptr(q)
	, file(file)
	, rsrc_addr(rsrc_addr)
	, rsrc_size(rsrc_size)
	, rsrc_va(rsrc_va)
	, pos(0)
{
	if (!file) {
		this->file = nullptr;
		q->m_lastError = -EBADF;
		return;
	} else if (rsrc_addr == 0 || rsrc_size == 0) {
		this->file = nullptr;
		q->m_lastError = -EIO;
		return;
	}

	// Validate the starting address and size.
	const int64_t fileSize = file->size();
	if ((int64_t)rsrc_addr >= fileSize) {
		// Starting address is past the end of the file.
		this->file = nullptr;
		q->m_lastError = -EIO;
		return;
	} else if (((int64_t)rsrc_addr + (int64_t)rsrc_size) > fileSize) {
		// Resource ends past the end of the file.
		this->file = nullptr;
		q->m_lastError = -EIO;
		return;
	}

	// Load the root resource directory.
	int ret = loadResDir(0, res_types);
	if (ret <= 0) {
		// No resources, or an error occurred.
		this->file = nullptr;
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
	int ret = file->seek(rsrc_addr + addr);
	if (ret != 0) {
		// Seek error.
		q->m_lastError = file->lastError();
		return q->m_lastError;
	}
	size_t size = file->read(&root, sizeof(root));
	if (size != sizeof(root)) {
		// Read error;
		q->m_lastError = file->lastError();
		return q->m_lastError;
	}

	// Total number of entries.
	unsigned int entryCount = le16_to_cpu(root.NumberOfNamedEntries) + le16_to_cpu(root.NumberOfIdEntries);
	assert(entryCount <= 64);
	if (entryCount > 64) {
		// Sanity check; constrain to 64 entries.
		entryCount = 64;
	}
	uint32_t szToRead = (uint32_t)(entryCount * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));
	unique_ptr<IMAGE_RESOURCE_DIRECTORY_ENTRY[]> irdEntries(new IMAGE_RESOURCE_DIRECTORY_ENTRY[entryCount]);
	size = file->read(irdEntries.get(), szToRead);
	if (size != szToRead) {
		// Read error.
		q->m_lastError = file->lastError();
		return q->m_lastError;
	}

	// Read each directory header.
	dir.resize(entryCount);
	const IMAGE_RESOURCE_DIRECTORY_ENTRY *irdEntry = irdEntries.get();
	unsigned int entriesRead = 0;
	for (unsigned int i = 0; i < entryCount; i++, irdEntry++) {
		// Skipping any root directory entry that isn't an ID.
		uint32_t id = le32_to_cpu(irdEntry->Name);
		if (id > 0xFFFF) {
			// Not an ID.
			continue;
		}

		// Entry data.
		auto &entry = dir[entriesRead];
		entry.id = (uint16_t)id;
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
	uint32_t type_addr = 0;
	for (auto iter_root = res_types.cbegin();
	     iter_root != res_types.cend(); ++iter_root)
	{
		if (iter_root->id == type) {
			// Found the type.
			if (!(iter_root->addr & 0x80000000)) {
				// Not a subdirectory.
				return nullptr;
			}
			type_addr = (iter_root->addr & ~0x80000000);
			break;
		}
	}
	if (type_addr == 0) {
		// Not found.
		return nullptr;
	}

	// Load the directory.
	auto iter_type = type_dirs.insert(std::make_pair(type, rsrc_dir_t()));
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
	uint32_t type_and_id = (type | ((uint32_t)id << 16));
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
	uint32_t id_addr = 0;
	for (auto iter_type = type_dir->cbegin();
	     iter_type != type_dir->cend(); ++iter_type)
	{
		if (iter_type->id == id) {
			// Found the ID.
			if (!(iter_type->addr & 0x80000000)) {
				// Not a subdirectory.
				return nullptr;
			}
			id_addr = (iter_type->addr & ~0x80000000);
			break;
		}
	}
	if (id_addr == 0) {
		// Not found.
		return nullptr;
	}

	// Load the directory.
	auto iter_type_and_id = type_and_id_dirs.insert(std::make_pair(type_and_id, rsrc_dir_t()));
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
	unsigned int key_len = (unsigned int)u16_strlen(key);
	// DWORD alignment: Make sure we end on a multiple of 4 bytes.
	unsigned int keyData_len = (key_len+1) * sizeof(char16_t);
	keyData_len = ((keyData_len + sizeof(fields) + 3) & ~3) - sizeof(fields);
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
 * DWORD alignment function.
 * @param file	[in] File to DWORD align.
 * @return 0 on success; non-zero on error.
 */
inline int PEResourceReaderPrivate::alignFileDWORD(IRpFile *file)
{
	int ret = 0;
	int64_t pos = file->tell();
	if (pos % 4 != 0) {
		pos = (pos + 3) & ~3LL;
		ret = file->seek(pos);
	}
	return ret;
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
	// - String: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646987(v=vs.85).aspx
	// - StringTable: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646992(v=vs.85).aspx

	// Read fields.
	const int64_t pos_start = file->tell();
	uint16_t fields[3];	// wLength, wValueLength, wType
	size_t size = file->read(fields, sizeof(fields));
	if (size != sizeof(fields)) {
		// Read error.
		return -EIO;
	}

	// wLength contains the total string table length.
	// wValueLength should be 0.
	// wType should be 1, indicating a language ID string.
	if (le16_to_cpu(fields[1]) != 0 || le16_to_cpu(fields[2]) != 1) {
		// Not a string table.
		return -EIO;
	}

	// Read the 8-character language ID.
	char16_t s_langID[9];
	size = file->read(s_langID, sizeof(s_langID));
	if (size != sizeof(s_langID) ||
	   (le16_to_cpu(s_langID[8]) != 0))
	{
		// Read error, or not NULL terminated.
		return -EIO;
	}

	// Convert to UTF-8.
	string str_langID = utf16le_to_utf8(s_langID, 8);
	// Parse using strtoul().
	char *endptr;
	*langID = (unsigned int)strtoul(str_langID.c_str(), &endptr, 16);
	if (*langID == 0 || endptr != (str_langID.c_str() + 8)) {
		// Not valid.
		// TODO: Better error code?
		return -EIO;
	}

	// DWORD alignment.
	alignFileDWORD(file);

	// Total string table size (in bytes) is wLength - (pos_strings - pos_start).
	const int64_t pos_strings = file->tell();
	int strTblData_len = (int)fields[0] - (int)(pos_strings - pos_start);
	if (strTblData_len <= 0) {
		// Error...
		return -EIO;
	}

	// Read the string table.
	unique_ptr<uint8_t[]> strTblData(new uint8_t[strTblData_len]);
	size = file->read(strTblData.get(), strTblData_len);
	if (size != (size_t)strTblData_len) {
		// Read error.
		return -EIO;
	}
	// DWORD alignment.
	alignFileDWORD(file);

	// Parse the string table.
	// TODO: Optimizations.
	st.clear();
	int tblPos = 0;
	while (tblPos < strTblData_len) {
		// wLength, wValueLength, wType
		memcpy(fields, &strTblData[tblPos], sizeof(fields));
		if (le16_to_cpu(fields[2]) != 1) {
			// Not a string...
			return -EIO;
		}
		// TODO: Use fields[] directly?
		// NOTE: wValueLength is number of *words* (aka UTF-16 characters).
		// Hence, we're multiplying by two to get bytes.
		const uint16_t wLength = le16_to_cpu(fields[0]);
		if (wLength < sizeof(fields)) {
			// Invalid length.
			return -EIO;
		}
		const int wValueLength = le16_to_cpu(fields[1]) * 2;
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
		if (le16_to_cpu(key[key_len]) != 0) {
			// Not NULL-terminated.
			return -EIO;
		}

		// DWORD alignment is required here.
		tblPos += ((key_len + 1) * 2);
		tblPos  = (tblPos + 3) & ~3;

		// Value must be NULL-terminated.
		const char16_t *value = reinterpret_cast<const char16_t*>(&strTblData[tblPos]);
		const int value_len = (wValueLength / 2) - 1;
		if (value_len <= 0) {
			// Empty value.
			static const char16_t u16_empty[1] = {0};
			value = u16_empty;
		} else if (le16_to_cpu(value[value_len]) != 0) {
			// Not NULL-terminated.
			return -EIO;
		}

		st.push_back(std::pair<rp_string, rp_string>(
			utf16le_to_rp_string(key, key_len),
			utf16le_to_rp_string(value, value_len)));

		// DWORD alignment is required here.
		tblPos += wValueLength;
		tblPos  = (tblPos + 3) & ~3;
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
PEResourceReader::PEResourceReader(IRpFile *file, uint32_t rsrc_addr, uint32_t rsrc_size, uint32_t rsrc_va)
	: d_ptr(new PEResourceReaderPrivate(this, file, rsrc_addr, rsrc_size, rsrc_va))
{ }

PEResourceReader::~PEResourceReader()
{
	delete d_ptr;
}

/** IDiscReader **/

/**
 * Is the partition open?
 * This usually only returns false if an error occurred.
 * @return True if the partition is open; false if it isn't.
 */
bool PEResourceReader::isOpen(void) const
{
	RP_D(PEResourceReader);
	return (d->file && d->file->isOpen());
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t PEResourceReader::read(void *ptr, size_t size)
{
	RP_D(PEResourceReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
		m_lastError = EBADF;
		return 0;
	}

	// Are we already at the end of the .rsrc section?
	if (d->pos >= (int64_t)d->rsrc_size) {
		// End of the .rsrc section.
		return 0;
	}

	// Make sure d->pos + size <= d->rsrc_size.
	// If it isn't, we'll do a short read.
	if (d->pos + (int64_t)size >= (int64_t)d->rsrc_size) {
		size = (size_t)((int64_t)d->rsrc_size - d->pos);
	}

	// Seek to the position.
	int ret = d->file->seek((int64_t)d->rsrc_addr + (int64_t)d->pos);
	if (ret != 0) {
		// Seek error.
		m_lastError = d->file->lastError();
		return 0;
	}
	// Read the data.
	size_t read = d->file->read(ptr, size);
	d->pos += read;
	m_lastError = d->file->lastError();
	return read;
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int PEResourceReader::seek(int64_t pos)
{
	RP_D(PEResourceReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// Handle out-of-range cases.
	// TODO: How does POSIX behave?
	if (pos < 0) {
		d->pos = 0;
	} else if (pos >= (int64_t)d->rsrc_size) {
		d->pos = (int64_t)d->rsrc_size;
	} else {
		d->pos = pos;
	}
	return 0;
}

/**
 * Seek to the beginning of the partition.
 */
void PEResourceReader::rewind(void)
{
	seek(0);
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
int64_t PEResourceReader::tell(void)
{
	RP_D(PEResourceReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
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
int64_t PEResourceReader::size(void)
{
	// TODO: Errors?
	RP_D(PEResourceReader);
	return (int64_t)d->rsrc_size;
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
int64_t PEResourceReader::partition_size(void) const
{
	// TODO: Errors?
	RP_D(const PEResourceReader);
	return (int64_t)d->rsrc_size;
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
int64_t PEResourceReader::partition_size_used(void) const
{
	RP_D(const PEResourceReader);
	return (int64_t)d->rsrc_size;
}

/** Resource access functions. **/

/**
 * Open a resource.
 * @param type Resource type ID.
 * @param id Resource ID. (-1 for "first entry")
 * @param lang Language ID. (-1 for "first entry")
 * @return IRpFile*, or nullptr on error.
 */
IRpFile *PEResourceReader::open(uint16_t type, int id, int lang)
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
	const PEResourceReaderPrivate::rsrc_dir_t *type_id_dir = d->getTypeIdDir(type, (uint16_t)id);
	if (!type_id_dir || type_id_dir->empty())
		return nullptr;

	const PEResourceReaderPrivate::ResDirEntry *dirEntry = nullptr;
	if (lang == -1) {
		// Get the first language for this type.
		dirEntry = &type_id_dir->at(0);
	} else {
		// Find the specified language ID.
		for (auto iter = type_id_dir->cbegin();
		     iter != type_id_dir->cend(); ++iter)
		{
			if (iter->id == (uint16_t)lang) {
				// Found the language ID.
				dirEntry = &(*iter);
				break;
			}
		}

		if (!dirEntry) {
			// Not found.
			return nullptr;
		}
	}

	// Make sure this is a file.
	assert(!(dirEntry->addr & 0x80000000));
	if (dirEntry->addr & 0x80000000) {
		// This is a subdirectory.
		return nullptr;
	}

	// Get the IMAGE_RESOURCE_DATA_ENTRY.
	IMAGE_RESOURCE_DATA_ENTRY irdata;
	int ret = d->file->seek(d->rsrc_addr + dirEntry->addr);
	if (ret != 0) {
		// Seek error.
		m_lastError = d->file->lastError();
		return nullptr;
	}
	size_t size = d->file->read(&irdata, sizeof(irdata));
	if (size != sizeof(irdata)) {
		// Read error.
		m_lastError = d->file->lastError();
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
	return new PartitionFile(this, data_addr, le32_to_cpu(irdata.Size));
}

/**
 * Load a VS_VERSION_INFO resource.
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
	unique_ptr<IRpFile> f_ver(this->open(RT_VERSION, id, lang));
	if (!f_ver) {
		// Not found.
		return -ENOENT;
	}

	// Read the version header.
	static const char16_t vsvi[] = {'V','S','_','V','E','R','S','I','O','N','_','I','N','F','O',0};
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
	PEResourceReaderPrivate::alignFileDWORD(f_ver.get());

	// Read the StringFileInfo section header.
	static const char16_t vssfi[] = {'S','t','r','i','n','g','F','i','l','e','I','n','f','o',0};
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
		// TODO: Reduce copying?
		pVsSfi->insert(std::make_pair(langID, st));
	}

	// Version information read successfully.
	return 0;
}

}
