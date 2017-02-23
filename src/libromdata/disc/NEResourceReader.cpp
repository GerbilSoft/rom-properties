/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NEResourceReader.cpp: New Executable resource reader.                   *
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

#include "NEResourceReader.hpp"
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

class NEResourceReaderPrivate
{
	public:
		NEResourceReaderPrivate(NEResourceReader *q, IRpFile *file,
			uint32_t rsrc_tbl_addr, uint32_t rsrc_tbl_size);

	private:
		RP_DISABLE_COPY(NEResourceReaderPrivate)
	protected:
		NEResourceReader *const q_ptr;

	public:
		// EXE file.
		IRpFile *file;

		// Resource segment table.
		uint32_t rsrc_tbl_addr;
		uint32_t rsrc_tbl_size;
		uint32_t rscAlign;	// Resource alignment.

		// Resource table entry.
		struct ResTblEntry {
			uint16_t id;	// Resource ID.
			uint32_t addr;	// Address of the resource data. (0 = start of EXE)
			uint32_t len;	// Length of the resource data.
		};
		typedef ao::uvector<ResTblEntry> rsrc_dir_t;

		// Resource types.
		unordered_map<uint16_t, rsrc_dir_t> res_types;

		/**
		 * Load the resource table.
		 * @return 0 on success; non-zero on error.
		 */
		int loadResTbl(void);
};

/** NEResourceReaderPrivate **/

NEResourceReaderPrivate::NEResourceReaderPrivate(
		NEResourceReader *q, IRpFile *file,
		uint32_t rsrc_tbl_addr,
		uint32_t rsrc_tbl_size)
	: q_ptr(q)
	, file(file)
	, rsrc_tbl_addr(rsrc_tbl_addr)
	, rsrc_tbl_size(rsrc_tbl_size)
{
	if (!file) {
		this->file = nullptr;
		q->m_lastError = -EBADF;
		return;
	} else if (rsrc_tbl_addr == 0) {
		this->file = nullptr;
		q->m_lastError = -EIO;
		return;
	} else if (rsrc_tbl_size < 6 || rsrc_tbl_size >= 65536) {
		// 64 KB is the segment size, so this shouldn't be possible.
		// Also, it should be at least 6 bytes.
		// (TODO: Larger minimum size?)
		this->file = nullptr;
		q->m_lastError = -EIO;
		return;
	}

	// NOTE: Win16 executables don't have a separate
	// .rsrc section, so we have to use the entire
	// size of the file as the resource size.

	// Validate the starting address and size.
	const int64_t fileSize = file->size();
	if ((int64_t)rsrc_tbl_addr >= fileSize) {
		// Starting address is past the end of the file.
		this->file = nullptr;
		q->m_lastError = -EIO;
		return;
	} else if (((int64_t)rsrc_tbl_addr + (int64_t)rsrc_tbl_size) > fileSize) {
		// Resource ends past the end of the file.
		this->file = nullptr;
		q->m_lastError = -EIO;
		return;
	}

	// Load the resource table.
	int ret = loadResTbl();
	if (ret != 0) {
		// No resources, or an error occurred.
		this->file = nullptr;
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
	int ret = file->seek(rsrc_tbl_addr);
	if (ret != 0) {
		// Seek error.
		q->m_lastError = file->lastError();
		return q->m_lastError;
	}
	unique_ptr<uint8_t[]> rsrcTblData(new uint8_t[rsrc_tbl_size]);
	size_t size = file->read(rsrcTblData.get(), rsrc_tbl_size);
	if (size != rsrc_tbl_size) {
		// Read error.
		q->m_lastError = file->lastError();
		return q->m_lastError;
	}

	// Get the shift alignment. (power of 2)
	const uint16_t shift_exp = (rsrcTblData[0] | (rsrcTblData[1] << 8));
	if (shift_exp >= 16) {
		// 64 KB or higher shift alignment is
		// probably out of range.
		return -EIO;
	}
	unsigned int pos = 2;

	// Initialize the resource types list.
	res_types.clear();

	// TODO: Overflow prevention.
	// TODO: Use pointers for pos and endpos?
	ret = -EIO;
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
		auto iter_type = res_types.insert(std::make_pair(rtTypeID, rsrc_dir_t()));
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
			entry.addr = le32_to_cpu(nameInfo->rnOffset) << shift_exp;
			entry.len = le16_to_cpu(nameInfo->rnLength) << shift_exp;
			entriesRead++;
		}
		if (isErr)
			break;

		// Shrink the vector in case we skipped some entries.
		dir.resize(entriesRead);
	}

	return ret;
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
NEResourceReader::NEResourceReader(IRpFile *file, uint32_t rsrc_tbl_addr, uint32_t rsrc_tbl_size)
	: d_ptr(new NEResourceReaderPrivate(this, file, rsrc_tbl_addr, rsrc_tbl_size))
{ }

NEResourceReader::~NEResourceReader()
{
	delete d_ptr;
}

/** IDiscReader **/

/**
 * Is the partition open?
 * This usually only returns false if an error occurred.
 * @return True if the partition is open; false if it isn't.
 */
bool NEResourceReader::isOpen(void) const
{
	RP_D(NEResourceReader);
	return (d->file && d->file->isOpen());
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t NEResourceReader::read(void *ptr, size_t size)
{
	RP_D(NEResourceReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
		m_lastError = EBADF;
		return 0;
	}

	// There isn't a separate resource "section" in
	// NE executables, so forward all read requests
	// to the underlying file.
	return d->file->read(ptr, size);
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int NEResourceReader::seek(int64_t pos)
{
	RP_D(NEResourceReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// There isn't a separate resource "section" in
	// NE executables, so forward all read requests
	// to the underlying file.
	return d->file->seek(pos);
}

/**
 * Seek to the beginning of the partition.
 */
void NEResourceReader::rewind(void)
{
	seek(0);
}

/**
 * Get the data size.
 * This size does not include the partition header,
 * and it's adjusted to exclude hashes.
 * @return Data size, or -1 on error.
 */
int64_t NEResourceReader::size(void)
{
	// TODO: Errors?
	RP_D(NEResourceReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// There isn't a separate resource "section" in
	// NE executables, so forward all read requests
	// to the underlying file.
	return (int64_t)d->file->size();
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
int64_t NEResourceReader::partition_size(void) const
{
	// TODO: Errors?
	RP_D(const NEResourceReader);

	// There isn't a separate resource "section" in
	// NE executables, so forward all read requests
	// to the underlying file.
	return (int64_t)d->file->size();
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
int64_t NEResourceReader::partition_size_used(void) const
{
	// TODO: Errors?
	RP_D(const NEResourceReader);

	// There isn't a separate resource "section" in
	// NE executables, so forward all read requests
	// to the underlying file.
	return (int64_t)d->file->size();
}

/** Resource access functions. **/

/**
 * Open a resource.
 * @param type Resource type ID.
 * @param id Resource ID. (-1 for "first entry")
 * @param lang Language ID. (-1 for "first entry")
 * @return IRpFile*, or nullptr on error.
 */
IRpFile *NEResourceReader::open(uint16_t type, int id, int lang)
{
	RP_D(NEResourceReader);

	// NOTE: The language ID is not used in NE resources.
	((void)lang);

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
	
	NEResourceReaderPrivate::ResTblEntry *entry = nullptr;
	if (id == -1) {
		// Get the first ID for this type.
		entry = &dir[0];
	} else {
		// Search for the ID.
		// TODO: unordered_map?
		for (unsigned int i = 0; i < (unsigned int)dir.size(); i++) {
			if (dir[i].id == (uint16_t)id) {
				// Found the ID.
				entry = &dir[i];
				break;
			}
		}

		if (!entry) {
			// ID not found.
			return nullptr;
		}
	}

	// Create the PartitionFile.
	// This is an IRpFile implementation that uses an
	// IPartition as the reader and takes an offset
	// and size as the file parameters.
	// TODO: Set the codepage somewhere?
	return new PartitionFile(this, entry->addr, entry->len);
}

/**
 * Load a VS_VERSION_INFO resource.
 * @param id		[in] Resource ID. (-1 for "first entry")
 * @param lang		[in] Language ID. (-1 for "first entry")
 * @param pVsFfi	[out] VS_FIXEDFILEINFO (host-endian)
 * @param pVsSfi	[out] StringFileInfo section.
 * @return 0 on success; non-zero on error.
 */
int NEResourceReader::load_VS_VERSION_INFO(int id, int lang, VS_FIXEDFILEINFO *pVsFfi, StringFileInfo *pVsSfi)
{
	// TODO
	return -1;
}

}
