/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XDVDFSPartition.cpp: Microsoft Xbox XDVDFS partition reader.            *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XDVDFSPartition.hpp"
#include "xdvdfs_structs.h"

// Other rom-properties libraries
#include "librpbase/timeconv.h"
using namespace LibRpBase;
using namespace LibRpText;
using LibRpFile::IRpFile;

// C++ STL classes
using std::shared_ptr;
using std::string;
using std::unordered_map;

namespace LibRomData {

class XDVDFSPartitionPrivate
{
	public:
		XDVDFSPartitionPrivate(XDVDFSPartition *q,
			off64_t partition_offset, off64_t partition_size);

	private:
		RP_DISABLE_COPY(XDVDFSPartitionPrivate)
	protected:
		XDVDFSPartition *const q_ptr;

	public:
		// Partition start offset. (in bytes)
		off64_t partition_offset;
		off64_t partition_size;		// Calculated partition size.

		// XDVDFS header.
		// All fields are byteswapped in the constructor.
		XDVDFS_Header xdvdfsHeader;

		// Cached directories.
		// - Key: Directory. ("/" for root)
		// - Value: Raw directory table from the disc.
		// NOTE: Directory entries are variable-length, so this
		// is a byte array, not an ISO_DirEntry array.
		unordered_map<std::string, ao::uvector<uint8_t> > dirTables;

		/**
		 * Get an entry within a specified directory table.
		 * @param dirTable Directory table.
		 * @param filename Filename to find, without subdirectories.
		 * @return Pointer to XDVDFS_DirEntry within dirTable, or nullptr if not found.
		 */
		const XDVDFS_DirEntry *getDirEntry(const ao::uvector<uint8_t> *dirTable, const char *filename);

		/**
		 * Get the specified directory.
		 * This should *only* be the directory, not a filename.
		 * @param path Directory path.
		 * @return Pointer to directory table (ao::uvector), or nullptr if not found.
		 */
		const ao::uvector<uint8_t> *getDirectory(const char *path);

		/**
		 * XDVDFS strcasecmp() implementation.
		 * Uses generic ASCII handling instead of locale-specific case folding.
		 * @param s1 String 1
		 * @param s2 String 2
		 * @return 0 (==), negative (<), or positive (>).
		 */
		static int xdvdfs_strcasecmp(const char *s1, const char *s2);
};

/** XDVDFSPartitionPrivate **/

XDVDFSPartitionPrivate::XDVDFSPartitionPrivate(XDVDFSPartition *q,
	off64_t partition_offset, off64_t partition_size)
	: q_ptr(q)
	, partition_offset(partition_offset)
	, partition_size(partition_size)
{
	// Clear the XDVDFS header struct.
	memset(&xdvdfsHeader, 0, sizeof(xdvdfsHeader));

	if (!q->m_discReader) {
		q->m_lastError = EIO;
		return;
	} else if (!q->m_discReader->isOpen()) {
		q->m_lastError = q->m_discReader->lastError();
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		UNREF_AND_NULL_NOCHK(q->m_discReader);
		return;
	}

	// Load the XDVDFS header.
	size_t size = q->m_discReader->seekAndRead(
		partition_offset + (XDVDFS_HEADER_LBA_OFFSET * XDVDFS_BLOCK_SIZE),
		&xdvdfsHeader, sizeof(xdvdfsHeader));
	if (size != sizeof(xdvdfsHeader)) {
		// Seek and/or read error.
		memset(&xdvdfsHeader, 0, sizeof(xdvdfsHeader));
		UNREF_AND_NULL_NOCHK(q->m_discReader);
		return;
	}

	// Verify the magic strings.
	if (memcmp(xdvdfsHeader.magic, XDVDFS_MAGIC, sizeof(xdvdfsHeader.magic)) != 0 ||
	    memcmp(xdvdfsHeader.magic_footer, XDVDFS_MAGIC, sizeof(xdvdfsHeader.magic_footer)) != 0)
	{
		// Invalid XDVDFS header.
		memset(&xdvdfsHeader, 0, sizeof(xdvdfsHeader));
		UNREF_AND_NULL_NOCHK(q->m_discReader);
		return;
	}

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	// Byteswap the fields.
	xdvdfsHeader.root_dir_sector	= le32_to_cpu(xdvdfsHeader.root_dir_sector);
	xdvdfsHeader.root_dir_size	= le32_to_cpu(xdvdfsHeader.root_dir_size);
	xdvdfsHeader.timestamp		= le64_to_cpu(xdvdfsHeader.timestamp);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

	// Load the root directory.
	getDirectory("/");
}

/**
 * XDVDFS strcasecmp() implementation.
 * Uses generic ASCII handling instead of locale-specific case folding.
 * @param s1 String 1
 * @param s2 String 2
 * @return 0 (==), negative (<), or positive (>).
 */
int XDVDFSPartitionPrivate::xdvdfs_strcasecmp(const char *s1, const char *s2)
{
	// Reference: https://github.com/XboxDev/extract-xiso/blob/master/extract-xiso.c
	// av1_compare_key()
	while (true) {
		char a = *s1++;
		char b = *s2++;

		// Convert to uppercase.
		if (a >= 'a' && a <= 'z') a &= ~0x20;
		if (b >= 'a' && b <= 'z') b &= ~0x20;

		if (a) {
			if (b) {
				if (a < b) return -1;
				if (a > b) return 1;
			} else return 1;
		} else return (b ? -1 : 0);
	}
}

/**
 * Get an entry within a specified directory table.
 * @param dirTable Directory table.
 * @param filename Filename to find, without subdirectories.
 * @return Pointer to XDVDFS_DirEntry within dirTable, or nullptr if not found.
 */
const XDVDFS_DirEntry *XDVDFSPartitionPrivate::getDirEntry(const ao::uvector<uint8_t> *dirTable, const char *filename)
{
	assert(dirTable != nullptr);
	assert(filename != nullptr);
	if (unlikely(!dirTable || !filename)) {
		RP_Q(XDVDFSPartition);
		q->m_lastError = EINVAL;
		return nullptr;
	}

	// Convert the filename portion to cp1252 before searching.
	const string s_filename = utf8_to_cp1252(filename, -1);

	// Find the file in the specified directory.
	// NOTE: Filenames are case-insensitive.
	const XDVDFS_DirEntry *dirEntry_found = nullptr;
	const uint8_t *const p_start = dirTable->data();
	const uint8_t *const p_end = p_start + dirTable->size();
	const uint8_t *p = p_start;
	string s_entry_filename;	// Temporary for NULL termination.
	while (p < p_end) {
		const XDVDFS_DirEntry *dirEntry = reinterpret_cast<const XDVDFS_DirEntry*>(p);
		const char *entry_filename = reinterpret_cast<const char*>(p) + sizeof(*dirEntry);
		if (entry_filename + dirEntry->name_length > reinterpret_cast<const char*>(p_end)) {
			// Filename is out of bounds.
			break;
		}

		// NOTE: Filename might not be NULL-terminated.
		// Temporarily cache the filename.
		s_entry_filename.assign(entry_filename, dirEntry->name_length);

		// Check the filename.
		uint16_t subtree_offset = 0;
		const int cmp = xdvdfs_strcasecmp(s_filename.c_str(), s_entry_filename.c_str());
		if (cmp == 0) {
			// Found it!
			dirEntry_found = dirEntry;
			break;
		} else if (cmp < 0) {
			// Left subtree.
			subtree_offset = le16_to_cpu(dirEntry->left_offset);
		} else if (cmp > 0) {
			// Right subtree.
			subtree_offset = le16_to_cpu(dirEntry->right_offset);
		}

		if (subtree_offset == 0 || subtree_offset == 0xFFFF) {
			// End of directory.
			break;
		}
		p = p_start + (subtree_offset * sizeof(uint32_t));
	}

	if (!dirEntry_found) {
		// Not found.
		return nullptr;
	}

	// Make sure the file is in bounds.
	const uint32_t file_size = le32_to_cpu(dirEntry_found->file_size);
	const off64_t file_addr = static_cast<off64_t>(
		le32_to_cpu(dirEntry_found->start_sector)) * XDVDFS_BLOCK_SIZE;
	if (file_addr >= (this->partition_size + this->partition_offset) ||
	    file_addr > (this->partition_size + this->partition_offset - file_size))
	{
		// File is out of bounds.
		RP_Q(XDVDFSPartition);
		q->m_lastError = EIO;
		return nullptr;
	}

	// Return the directory entry.
	return dirEntry_found;
}

/**
 * Get the specified directory.
 * This should *only* be the directory, not a filename.
 * @param path Directory path.
 * @return Pointer to directory table (ao::uvector), or nullptr if not found.
 */
const ao::uvector<uint8_t> *XDVDFSPartitionPrivate::getDirectory(const char *path)
{
	RP_Q(XDVDFSPartition);
	if (unlikely(!path || path[0] != '/')) {
		// Invalid path.
		q->m_lastError = EINVAL;
		return nullptr;
	} else if (unlikely(xdvdfsHeader.magic[0] == '\0')) {
		// XDVDFS isn't loaded.
		q->m_lastError = EIO;
		return nullptr;
	}

	// TODO: Remove unnecessary leading and trailing slashes.

	// Is this directory table already loaded?
	auto iter = dirTables.find(path);
	if (iter != dirTables.end()) {
		// Directory table is already loaded.
		return &(iter->second);
	}

	// DiscReader must be available now.
	if (unlikely(!q->m_discReader)) {
		// DiscReader isn't open.
		q->m_lastError = EIO;
		return nullptr;
	}

	// Directory table address and size.
	off64_t dir_addr = 0;
	uint32_t dir_size = 0;

	if (!strcmp(path, "/")) {
		// Special handling for the root directory.

		// Root directory size should be less than 16 MB.
		if (xdvdfsHeader.root_dir_size > 16*1024*1024) {
			// Root directory is too big.
			q->m_lastError = EIO;
			return nullptr;
		}

		// Root directory offsets.
		dir_addr = partition_offset + (
			static_cast<off64_t>(xdvdfsHeader.root_dir_sector) * XDVDFS_BLOCK_SIZE);
		dir_size = xdvdfsHeader.root_dir_size;
	} else {
		// Get the parent directory.
		// TODO
		return nullptr;
	}

	// Read the directory.
	ao::uvector<uint8_t> dirTable(dir_size);
	size_t size = q->m_discReader->seekAndRead(dir_addr, dirTable.data(), dirTable.size());
	if (size != dirTable.size()) {
		// Seek and/or read error.
		q->m_lastError = q->m_discReader->lastError();
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		return nullptr;
	}

	// Save the directory table for later.
	auto ins_iter = dirTables.emplace(path, std::move(dirTable));

	// Root directory loaded.
	return &(ins_iter.first->second);
}

/** XDVDFSPartition **/

/**
 * Construct an XDVDFSPartition with the specified IDiscReader.
 *
 * NOTE: The IDiscReader *must* remain valid while this
 * XDVDFSPartition is open.
 *
 * @param discReader IDiscReader.
 * @param partition_offset Partition start offset.
 * @param partition_size Partition size.
 */
XDVDFSPartition::XDVDFSPartition(IDiscReader *discReader, off64_t partition_offset, off64_t partition_size)
	: super(discReader)
	, d_ptr(new XDVDFSPartitionPrivate(this, partition_offset, partition_size))
{ }

XDVDFSPartition::~XDVDFSPartition()
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
size_t XDVDFSPartition::read(void *ptr, size_t size)
{
	assert(m_discReader != nullptr);
	assert(m_discReader->isOpen());
	if (!m_discReader || !m_discReader->isOpen()) {
		m_lastError = EBADF;
		return 0;
	}

	// XDVDFS partitions are stored as-is.
	// TODO: data_size checks?
	return m_discReader->read(ptr, size);
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int XDVDFSPartition::seek(off64_t pos)
{
	RP_D(XDVDFSPartition);
	assert(m_discReader != nullptr);
	assert(m_discReader->isOpen());
	if (!m_discReader ||  !m_discReader->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	int ret = m_discReader->seek(d->partition_offset + pos);
	if (ret != 0) {
		m_lastError = m_discReader->lastError();
	}
	return ret;
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
off64_t XDVDFSPartition::tell(void)
{
	RP_D(XDVDFSPartition);
	assert(m_discReader != nullptr);
	assert(m_discReader->isOpen());
	if (!m_discReader ||  !m_discReader->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	off64_t ret = m_discReader->tell() - d->partition_offset;
	if (ret < 0) {
		m_lastError = m_discReader->lastError();
	}
	return ret;
}

/**
 * Get the data size.
 * This size does not include the partition header,
 * and it's adjusted to exclude hashes.
 * @return Data size, or -1 on error.
 */
off64_t XDVDFSPartition::size(void)
{
	// TODO: Restrict partition size?
	RP_D(const XDVDFSPartition);
	if (!m_discReader)
		return -1;
	return d->partition_size;
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
off64_t XDVDFSPartition::partition_size(void) const
{
	// TODO: Restrict partition size?
	RP_D(const XDVDFSPartition);
	if (!m_discReader)
		return -1;
	return d->partition_size;
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
off64_t XDVDFSPartition::partition_size_used(void) const
{
	// TODO: Implement for ISO?
	// For now, just use partition_size().
	return partition_size();
}

/** XDVDFSPartition **/

/** IFst wrapper functions. **/

// TODO

#if 0
/**
 * Open a directory.
 * @param path	[in] Directory path.
 * @return IFst::Dir*, or nullptr on error.
 */
IFst::Dir *XDVDFSPartition::opendir(const char *path)
{
	RP_D(XDVDFSPartition);
	if (!d->fst) {
		// FST isn't loaded.
		if (d->loadFst() != 0) {
			// FST load failed.
			// TODO: Errors?
			return nullptr;
		}
	}

	return d->fst->opendir(path);
}

/**
 * Read a directory entry.
 * @param dirp FstDir pointer.
 * @return IFst::DirEnt*, or nullptr if end of directory or on error.
 * (TODO: Add lastError()?)
 */
IFst::DirEnt *XDVDFSPartition::readdir(IFst::Dir *dirp)
{
	RP_D(XDVDFSPartition);
	if (!d->fst) {
		// TODO: Errors?
		return nullptr;
	}

	return d->fst->readdir(dirp);
}

/**
 * Close an opened directory.
 * @param dirp FstDir pointer.
 * @return 0 on success; negative POSIX error code on error.
 */
int XDVDFSPartition::closedir(IFst::Dir *dirp)
{
	RP_D(XDVDFSPartition);
	if (!d->fst) {
		// TODO: Errors?
		return -EBADF;
	}

	return d->fst->closedir(dirp);
}
#endif

/**
 * Open a file. (read-only)
 * @param filename Filename.
 * @return IRpFile*, or nullptr on error.
 */
shared_ptr<IRpFile> XDVDFSPartition::open(const char *filename)
{
	// TODO: File reference counter.
	// This might be difficult to do because PartitionFile is a separate class.

	// TODO: Support subdirectories.

	// Filename must be valid, and must start with a slash.
	// Only absolute paths are supported.
	if (!filename || filename[0] != '/') {
		// No filename and/or does not start with a slash.
		// TODO: Prepend a slash like GcnFst, or remove slash prepending from GcnFst?
		m_lastError = EINVAL;
		return nullptr;
	}

	// Remove leading slashes.
	while (*filename == '/') {
		filename++;
	}
	if (filename[0] == 0) {
		// Nothing but slashes...
		return nullptr;
	}

	// TODO: Handle subdirectories.
	// For now, assuming the file is in the root directory.
	RP_D(XDVDFSPartition);
	const ao::uvector<uint8_t> *const dirTable = d->getDirectory("/");
	if (!dirTable) {
		// Directory not found.
		// getDirectory() has already set m_lastError.
		return nullptr;
	}

	// Find the file in the root directory.
	const XDVDFS_DirEntry *const dirEntry = d->getDirEntry(dirTable, filename);
	if (!dirEntry) {
		// File not found.
		// getDirEntry() has already set m_lastError.
		return nullptr;
	}

	// Make sure this is a regular file.
	// TODO: Check for XDVDFS_ATTR_NORMAL?
	if (dirEntry->attributes & XDVDFS_ATTR_DIRECTORY) {
		// Not a regular file.
		m_lastError = EISDIR;
		return nullptr;
	}

	const uint32_t file_size = le32_to_cpu(dirEntry->file_size);
	const off64_t file_addr = static_cast<off64_t>(le32_to_cpu(dirEntry->start_sector)) * XDVDFS_BLOCK_SIZE;

	// Create the PartitionFile.
	// This is an IRpFile implementation that uses an
	// IPartition as the reader and takes an offset
	// and size as the file parameters.
	return shared_ptr<IRpFile>(new PartitionFile(this, file_addr, file_size));
}

/** XDVDFSPartition **/

/**
 * Get the XDVDFS timestamp.
 * @return XDVDFS timestamp, or -1 on error.
 */
time_t XDVDFSPartition::xdvdfsTimestamp(void) const
{
	RP_D(const XDVDFSPartition);
	assert(m_discReader != nullptr);
	assert(m_discReader->isOpen());
	if (!m_discReader ||  !m_discReader->isOpen()) {
		// XDVDFS isn't loaded.
		const_cast<XDVDFSPartition*>(this)->m_lastError = EBADF;
		return -1;
	}

	// NOTE: Timestamp is stored in Windows FILETIME format,
	// which is 100ns units since 1601/01/01 00:00:00 UTC.
	return WindowsTimeToUnixTime(d->xdvdfsHeader.timestamp);
}

}
