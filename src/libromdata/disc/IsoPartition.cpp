/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IsoPartition.cpp: ISO-9660 partition reader.                            *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "IsoPartition.hpp"
#include "iso_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::string;
using std::unordered_map;

// TODO: HSFS/CDI support?

namespace LibRomData {

class IsoPartitionPrivate
{
public:
	IsoPartitionPrivate(IsoPartition *q, off64_t partition_offset, int iso_start_offset);
	~IsoPartitionPrivate();

private:
	RP_DISABLE_COPY(IsoPartitionPrivate)
protected:
	IsoPartition *const q_ptr;

public:
	// Partition start offset (in bytes)
	off64_t partition_offset;
	off64_t partition_size;		// Calculated partition size

	// ISO primary volume descriptor
	ISO_Primary_Volume_Descriptor pvd;

	// Directories
	// - Key: Directory name, WITHOUT leading slash. (Root == empty string) [cp1252]
	// - Value: Directory entries.
	// NOTE: Directory entries are variable-length, so this
	// is a byte array, not an ISO_DirEntry array.
	typedef rp::uvector<uint8_t> DirData_t;
	unordered_map<string, DirData_t> dir_data;

	// ISO start offset (in blocks)
	// -1 == unknown
	int iso_start_offset;

	// IFst::Dir* reference counter
	int fstDirCount;

	/**
	 * Find the last slash or backslash in a path.
	 * @param path Path
	 * @return Last slash or backslash, or nullptr if not found.
	 */
	static inline const char *findLastSlash(const char *path)
	{
		const char *sl = strrchr(path, '/');
		const char *const bs = strrchr(path, '\\');
		if (sl && bs) {
			if (bs > sl) {
				sl = bs;
			}
		}
		return (sl) ? sl : bs;
	}

	/**
	 * Look up a directory entry from a base filename and directory.
	 * @param pDir		[in] Directory
	 * @param filename	[in] Base filename [cp1252]
	 * @param bFindDir	[in] True to find a subdirectory; false to find a file.
	 * @return ISO directory entry.
	 */
	const ISO_DirEntry *lookup_int(const DirData_t *pDir, const char *filename, bool bFindDir);

	/**
	 * Get a directory.
	 * @param path		[in] Pathname [cp1252] (For root, specify "" or "/".)
	 * @param pError	[out] POSIX error code on error.
	 * @return Directory on success; nullptr on error.
	 */
	const DirData_t *getDirectory(const char *path, int *pError = nullptr);

	/**
	 * Look up a directory entry from a filename.
	 * @param filename Filename [UTF-8]
	 * @return ISO directory entry
	 */
	const ISO_DirEntry *lookup(const char *filename);

	/**
	 * Parse an ISO-9660 timestamp.
	 * @param isofiletime File timestamp
	 * @return Unix time
	 */
	static time_t parseTimestamp(const ISO_Dir_DateTime_t *isofiletime);
};

/** IsoPartitionPrivate **/

IsoPartitionPrivate::IsoPartitionPrivate(IsoPartition *q,
	off64_t partition_offset, int iso_start_offset)
	: q_ptr(q)
	, partition_offset(partition_offset)
	, partition_size(0)
	, iso_start_offset(iso_start_offset)
	, fstDirCount(0)
{
	// Clear the PVD struct.
	memset(&pvd, 0, sizeof(pvd));

	if (!q->m_file) {
		q->m_lastError = EIO;
		return;
	} else if (!q->m_file->isOpen()) {
		q->m_lastError = q->m_file->lastError();
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		q->m_file.reset();
		return;
	}

	// Calculated partition size.
	partition_size = q->m_file->size() - partition_offset;

	// Load the primary volume descriptor.
	// TODO: Assuming this is the first one.
	// Check for multiple?
	size_t size = q->m_file->seekAndRead(partition_offset + ISO_PVD_ADDRESS_2048, &pvd, sizeof(pvd));
	if (size != sizeof(pvd)) {
		// Seek and/or read error.
		q->m_file.reset();
		return;
	}

	// Verify the signature and volume descriptor type.
	if (pvd.header.type != ISO_VDT_PRIMARY || pvd.header.version != ISO_VD_VERSION ||
	    memcmp(pvd.header.identifier, ISO_VD_MAGIC, sizeof(pvd.header.identifier)) != 0)
	{
		// Invalid volume descriptor.
		q->m_file.reset();
		return;
	}

	// Load the root directory.
	getDirectory("/");
}

IsoPartitionPrivate::~IsoPartitionPrivate()
{
	assert(fstDirCount == 0);
}

/**
 * Look up a directory entry from a base filename and directory.
 * @param pDir		[in] Directory
 * @param filename	[in] Base filename [cp1252]
 * @param bFindDir	[in] True to find a subdirectory; false to find a file.
 * @return ISO directory entry.
 */
const ISO_DirEntry *IsoPartitionPrivate::lookup_int(const DirData_t *pDir, const char *filename, bool bFindDir)
{
	// Find the file in the directory.
	// NOTE: Filenames are case-insensitive.
	// NOTE: File might have a ";1" suffix.
	int err = ENOENT;
	const unsigned int filename_len = static_cast<unsigned int>(strlen(filename));
	const ISO_DirEntry *dirEntry_found = nullptr;
	const uint8_t *p = pDir->data();
	const uint8_t *const p_end = p + pDir->size();
	while ((p + sizeof(ISO_DirEntry)) < p_end) {
		const ISO_DirEntry *dirEntry = reinterpret_cast<const ISO_DirEntry*>(p);
		if (dirEntry->entry_length == 0) {
			// Directory entries cannot span multiple sectors in
			// multi-sector directories, so if needed, the rest
			// of the sector is padded with 00.
			// Find the next non-zero byte.
			for (p++; p < p_end; p++) {
				if (*p != '\0') {
					// Found a non-zero byte.
					break;
				}
			}
			if (p >= p_end) {
				// No more non-zero bytes.
				break;
			}
			continue;
		} else if (dirEntry->entry_length < sizeof(*dirEntry)) {
			// Invalid directory entry?
			break;
		}

		const char *const entry_filename = reinterpret_cast<const char*>(p) + sizeof(*dirEntry);
		if (entry_filename + dirEntry->filename_length > reinterpret_cast<const char*>(p_end)) {
			// Filename is out of bounds.
			break;
		}

		// Skip subdirectories with names "\x00" and "\x01".
		// These are Joliet "special directory identifiers".
		// TODO: Joliet subdirectory support?
		if ((dirEntry->flags & ISO_FLAG_DIRECTORY) && dirEntry->filename_length == 1) {
			if (static_cast<uint8_t>(entry_filename[0]) <= 0x01) {
				// Skip this filename.
				p += dirEntry->entry_length;
				continue;
			}
		}

		// Check the filename.
		// 1990s and early 2000s CD-ROM games usually have
		// ";1" filenames, so check for that first.
		if (dirEntry->filename_length == filename_len + 2) {
			// +2 length match.
			// This might have ";1".
			if (!strncasecmp(entry_filename, filename, filename_len)) {
				// Check for ";1".
				// TODO: Also allow other version numbers?
				if (entry_filename[filename_len]   == ';' &&
				    entry_filename[filename_len+1] == '1')
				{
					// Found it!
					// Verify directory vs. file.
					const bool isDir = !!(dirEntry->flags & ISO_FLAG_DIRECTORY);
					if (isDir == bFindDir) {
						// Directory attribute matches.
						dirEntry_found = dirEntry;
					} else {
						// Not a match.
						err = (isDir ? EISDIR : ENOTDIR);
					}
					break;
				}
			}
		} else if (dirEntry->filename_length == filename_len) {
			// Exact length match.
			if (!strncasecmp(entry_filename, filename, filename_len)) {
				// Found it!
				dirEntry_found = dirEntry;
				break;
			}
		}

		// Next entry.
		p += dirEntry->entry_length;
	}

	if (!dirEntry_found) {
		RP_Q(IsoPartition);
		q->m_lastError = err;
	}
	return dirEntry_found;
}

/**
 * Get a directory.
 * @param path		[in] Pathname [cp1252] (For root, specify "" or "/".)
 * @param pError	[out] POSIX error code on error.
 * @return Directory on success; nullptr on error.
 */
const IsoPartitionPrivate::DirData_t *IsoPartitionPrivate::getDirectory(const char *path, int *pError)
{
	RP_Q(IsoPartition);
	if (!path || !strcmp(path, "/")) {
		// Root directory. Use "".
		path = "";
	}

	// Check if this directory was already loaded.
	auto iter = dir_data.find(path);
	if (iter != dir_data.end()) {
		// Directory is already loaded.
		return &iter->second;
	}

	if (unlikely(!q->m_file)) {
		// DiscReader isn't open.
		q->m_lastError = EIO;
		if (pError) {
			*pError = EIO;
		}
		return nullptr;
	} else if (unlikely(pvd.header.type != ISO_VDT_PRIMARY || pvd.header.version != ISO_VD_VERSION)) {
		// PVD isn't loaded.
		q->m_lastError = EIO;
		if (pError) {
			*pError = EIO;
		}
		return nullptr;
	}

	// Block size.
	// Should be 2048, but other values are possible.
	const unsigned int block_size = pvd.logical_block_size.he;

	// Determine the directory size and address.
	DirData_t dir;
	off64_t dir_addr;
	if (path[0] == '\0') {
		// Loading the root directory.

		// Check the root directory entry.
		const ISO_DirEntry *const rootdir = &pvd.dir_entry_root;
		if (rootdir->size.he > 16*1024*1024) {
			// Root directory is too big.
			q->m_lastError = EIO;
			if (pError) {
				*pError = EIO;
			}
			return nullptr;
		}

		if (iso_start_offset >= 0) {
			// ISO start address was already determined.
			if (rootdir->block.he < (static_cast<unsigned int>(iso_start_offset) + 2)) {
				// Starting block is invalid.
				q->m_lastError = EIO;
				if (pError) {
					*pError = EIO;
				}
				return nullptr;
			}
		} else {
			// We didn't find the ISO start address yet.
			// This might be a 2048-byte single-track image,
			// in which case, we'll need to assume that the
			// root directory starts at block 20.
			// TODO: Better heuristics.
			// TODO: Find the block that starts with "CD001" instead of this heuristic.
			if (rootdir->block.he < 20) {
				// Starting block is invalid.
				q->m_lastError = EIO;
				if (pError) {
					*pError = EIO;
				}
				return nullptr;
			}
			iso_start_offset = static_cast<int>(rootdir->block.he - 20);
		}

		dir.resize(rootdir->size.he);
		dir_addr = partition_offset + static_cast<off64_t>(rootdir->block.he - iso_start_offset) * block_size;
	} else {
		// Get the parent directory.
		const DirData_t *pDir;
		const char *const sl = findLastSlash(path);
		if (!sl) {
			// No slash. Parent is root.
			pDir = getDirectory("");
		} else {
			// Found a slash.
			const string s_parentDir(path, (sl - path));
			path = sl + 1;
			pDir = getDirectory(s_parentDir.c_str());
		}

		if (!pDir) {
			// Can't find the parent directory.
			// getDirectory() already set q->lastError().
			return nullptr;
		}

		// Find this directory.
		const ISO_DirEntry *const entry = lookup_int(pDir, path, true);
		if (!entry) {
			// Not found.
			// lookup_int() already set q->lastError().
			return nullptr;
		}

		dir.resize(entry->size.he);
		dir_addr = partition_offset + static_cast<off64_t>(entry->block.he - iso_start_offset) * block_size;
	}

	// Load the directory.
	// NOTE: Due to variable-length entries, we need to load
	// the entire directory all at once.
	size_t size = q->m_file->seekAndRead(dir_addr, dir.data(), dir.size());
	if (size != dir.size()) {
		// Seek and/or read error.
		dir.clear();
		q->m_lastError = q->m_file->lastError();
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		if (pError) {
			*pError = q->m_lastError;
		}
		return nullptr;
	}

	// Directory loaded.
	auto ins = dir_data.emplace(path, std::move(dir));
	return &(ins.first->second);
}

/**
 * Look up a directory entry from a filename.
 * @param filename Filename [UTF-8]
 * @return ISO directory entry
 */
const ISO_DirEntry *IsoPartitionPrivate::lookup(const char *filename)
{
	assert(filename != nullptr);
	assert(filename[0] != '\0');
	RP_Q(IsoPartition);

	// Remove leading slashes.
	while (*filename == '/') {
		filename++;
	}
	if (filename[0] == 0) {
		// Nothing but slashes...
		q->m_lastError = EINVAL;
		return nullptr;
	}

	// TODO: Which encoding?
	// Assuming cp1252...
	const DirData_t *pDir;

	// Is this file in a subdirectory?
	const char *const sl = findLastSlash(filename);
	if (sl) {
		// This file is in a subdirectory.
		const string s_parentDir = utf8_to_cp1252(filename, static_cast<int>(sl - filename));
		filename = sl + 1;
		pDir = getDirectory(s_parentDir.c_str());
	} else {
		// Not in a subdirectory.
		// Parent directory is root.
		pDir = getDirectory("");
	}

	if (!pDir) {
		// Error getting the directory.
		// getDirectory() has already set q->lastError.
		return nullptr;
	}

	// Find the file in the directory.
	const string s_filename = utf8_to_cp1252(filename, -1);
	return lookup_int(pDir, s_filename.c_str(), false);
}

/**
 * Parse an ISO-9660 timestamp.
 * @param isofiletime File timestamp
 * @return Unix time
 */
time_t IsoPartitionPrivate::parseTimestamp(const ISO_Dir_DateTime_t *isofiletime)
{
	// Convert to Unix time.
	// NOTE: struct tm has some oddities:
	// - tm_year: year - 1900
	// - tm_mon: 0 == January
	struct tm isotime;

	isotime.tm_year = isofiletime->year;
	isotime.tm_mon  = isofiletime->month - 1;
	isotime.tm_mday = isofiletime->day;

	isotime.tm_hour = isofiletime->hour;
	isotime.tm_min = isofiletime->minute;
	isotime.tm_sec = isofiletime->second;

	// tm_wday and tm_yday are output variables.
	isotime.tm_wday = 0;
	isotime.tm_yday = 0;
	isotime.tm_isdst = 0;

	// If conversion fails, this will return -1.
	time_t unixtime = timegm(&isotime);
	if (unixtime == -1) {
		return unixtime;
	}

	// Adjust for the timezone offset.
	// NOTE: Restricting to [-52, 52] as per the Linux kernel's isofs module.
	if (-52 <= isofiletime->tz_offset && isofiletime->tz_offset <= 52) {
		unixtime -= (static_cast<int>(isofiletime->tz_offset) * (15*60));
	}
	return unixtime;
}

/** IsoPartition **/

/**
 * Construct an IsoPartition with the specified IDiscReader (or IRpFile).
 *
 * @param discReader IDiscReader (or IRpFile)
 * @param partition_offset Partition start offset.
 * @param iso_start_offset ISO start offset, in blocks. (If -1, uses heuristics.)
 */
IsoPartition::IsoPartition(const IRpFilePtr &discReader, off64_t partition_offset, int iso_start_offset)
	: super(discReader)
	, d_ptr(new IsoPartitionPrivate(this, partition_offset, iso_start_offset))
{}

IsoPartition::~IsoPartition()
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
size_t IsoPartition::read(void *ptr, size_t size)
{
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	if (!m_file || !m_file->isOpen()) {
		m_lastError = EBADF;
		return 0;
	}

	// GCN partitions are stored as-is.
	// TODO: data_size checks?
	return m_file->read(ptr, size);
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int IsoPartition::seek(off64_t pos)
{
	RP_D(IsoPartition);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	if (!m_file ||  !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	int ret = m_file->seek(d->partition_offset + pos);
	if (ret != 0) {
		m_lastError = m_file->lastError();
	}
	return ret;
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
off64_t IsoPartition::tell(void)
{
	RP_D(IsoPartition);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	if (!m_file ||  !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	off64_t ret = m_file->tell() - d->partition_offset;
	if (ret < 0) {
		m_lastError = m_file->lastError();
	}
	return ret;
}

/**
 * Get the data size.
 * This size does not include the partition header,
 * and it's adjusted to exclude hashes.
 * @return Data size, or -1 on error.
 */
off64_t IsoPartition::size(void)
{
	// TODO: Restrict partition size?
	RP_D(const IsoPartition);
	if (!m_file)
		return -1;
	return d->partition_size;
}

/** Device file functions **/

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
off64_t IsoPartition::partition_size(void) const
{
	// TODO: Restrict partition size?
	RP_D(const IsoPartition);
	if (!m_file)
		return -1;
	return d->partition_size;
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
off64_t IsoPartition::partition_size_used(void) const
{
	// TODO: Implement for ISO?
	// For now, just use partition_size().
	return partition_size();
}

/** IsoPartition **/

/** IFst wrapper functions **/

/**
 * Open a directory.
 * @param path	[in] Directory path
 * @return IFst::Dir*, or nullptr on error.
 */
IFst::Dir *IsoPartition::opendir(const char *path)
{
	RP_D(IsoPartition);
	const IsoPartitionPrivate::DirData_t *const pDir = d->getDirectory(path);
	if (!pDir) {
		// Path not found.
		// TODO: Return an error code?
		return nullptr;
	}

	// FIXME: Create an IsoFst class? Cannot pass `this` as IFst*.
	IFst::Dir *const dirp = new IFst::Dir(nullptr, (void*)pDir);
	d->fstDirCount++;

	// Initialize the entry to this directory.
	// readdir() will automatically seek to the next entry.
	dirp->entry.extra = nullptr;	// temporary filename storage
	dirp->entry.ptnum = 0;		// not used for ISO
	dirp->entry.idx = 0;
	dirp->entry.type = DT_UNKNOWN;
	dirp->entry.name = nullptr;
	// offset and size are not valid for directories.
	dirp->entry.offset = 0;
	dirp->entry.size = 0;

	// Return the IFst::Dir*.
	return dirp;
}

/**
 * Read a directory entry.
 * @param dirp FstDir pointer
 * @return IFst::DirEnt*, or nullptr if end of directory or on error.
 * (TODO: Add lastError()?)
 */
const IFst::DirEnt *IsoPartition::readdir(IFst::Dir *dirp)
{
	assert(dirp != nullptr);
	//assert(dirp->parent == this);
	if (!dirp /*|| dirp->parent != this*/) {
		// No directory pointer, or the dirp
		// doesn't belong to this IFst.
		return nullptr;
	}

	const IsoPartitionPrivate::DirData_t *const pDir =
		reinterpret_cast<const IsoPartitionPrivate::DirData_t*>(dirp->dir_idx);
	const uint8_t *p = pDir->data();
	const uint8_t *const p_end = p + pDir->size();
	p += dirp->entry.idx;

	// NOTE: Using a loop in order to skip files that aren't really files.
	const ISO_DirEntry *dirEntry = nullptr;
	const char *entry_filename = nullptr;
	while (p < p_end) {
		dirEntry = reinterpret_cast<const ISO_DirEntry*>(p);
		if (dirEntry->entry_length == 0) {
			// Directory entries cannot span multiple sectors in
			// multi-sector directories, so if needed, the rest
			// of the sector is padded with 00.
			// Find the next non-zero byte.
			for (p++; p < p_end; p++) {
				if (*p != '\0') {
					// Found a non-zero byte.
					dirp->entry.idx = static_cast<int>(p - pDir->data());
					break;
				}
			}
			if (p >= p_end) {
				// No more non-zero bytes.
				dirp->entry.idx = static_cast<int>(pDir->size());
				break;
			}
			continue;
		} else if (dirEntry->entry_length < sizeof(*dirEntry)) {
			// Invalid directory entry?
			return nullptr;
		}

		entry_filename = reinterpret_cast<const char*>(p) + sizeof(*dirEntry);
		if (entry_filename + dirEntry->filename_length > reinterpret_cast<const char*>(p_end)) {
			// Filename is out of bounds.
			return nullptr;
		}

		// Skip subdirectories with names "\x00" and "\x01".
		// These are Joliet "special directory identifiers".
		// TODO: Joliet subdirectory support?
		if ((dirEntry->flags & ISO_FLAG_DIRECTORY) && dirEntry->filename_length == 1) {
			if (static_cast<uint8_t>(entry_filename[0]) <= 0x01) {
				// Skip this filename.
				dirp->entry.idx += dirEntry->entry_length;
				p += dirEntry->entry_length;
				continue;
			}
		}

		// Found a valid file.
		break;
	}
	if (!dirEntry) {
		// Could not find a valid file. (End of directory?)
		return nullptr;
	}

	const bool isDir = !!(dirEntry->flags & ISO_FLAG_DIRECTORY);
	if (isDir) {
		// Technically, offset/size are valid for directories on ISO-9660,
		// but we're going to set them to 0.
		dirp->entry.type = DT_DIR;
		dirp->entry.offset = 0;
		dirp->entry.size = 0;
	} else {
		RP_D(IsoPartition);
		const unsigned int block_size = d->pvd.logical_block_size.he;
		dirp->entry.type = DT_REG;
		dirp->entry.offset = static_cast<off64_t>(dirEntry->block.he) * block_size;
		dirp->entry.size = dirEntry->size.he;
	}

	// NOTE: Need to copy the filename in order to have NULL-termination.
	// TODO: Remove ";1" from the filename, if present?
	char *extra = static_cast<char*>(dirp->entry.extra);
	delete[] extra;
	extra = new char[dirEntry->filename_length + 1];
	memcpy(extra, entry_filename, dirEntry->filename_length);
	extra[dirEntry->filename_length] = '\0';

	dirp->entry.name = extra;
	dirp->entry.extra = extra;

	// Next file entry.
	dirp->entry.idx += dirEntry->entry_length;

	return &dirp->entry;
}

/**
 * Close an opened directory.
 * @param dirp FstDir pointer
 * @return 0 on success; negative POSIX error code on error.
 */
int IsoPartition::closedir(IFst::Dir *dirp)
{
	assert(dirp != nullptr);
	//assert(dirp->parent == this);
	if (!dirp) {
		// No directory pointer.
		// In release builds, this is a no-op.
		return 0;
	} /*else if (dirp->parent != this) {
		// The dirp doesn't belong to this IFst.
		return -EINVAL;
	}*/

	RP_D(IsoPartition);
	assert(d->fstDirCount > 0);
	if (dirp->entry.extra) {
		delete[] static_cast<char*>(dirp->entry.extra);
	}
	delete dirp;
	d->fstDirCount--;
	return 0;
}

/**
 * Open a file. (read-only)
 * @param filename Filename
 * @return IRpFile*, or nullptr on error.
 */
IRpFilePtr IsoPartition::open(const char *filename)
{
	RP_D(IsoPartition);
	assert((bool)m_file);
	assert(m_file->isOpen());
	if (!m_file ||  !m_file->isOpen()) {
		m_lastError = EBADF;
		return nullptr;
	}

	assert(filename != nullptr);
	if (!filename || filename[0] == 0) {
		// No filename.
		m_lastError = EINVAL;
		return nullptr;
	}

	// TODO: File reference counter.
	// This might be difficult to do because PartitionFile is a separate class.
	const ISO_DirEntry *const dirEntry = d->lookup(filename);
	if (!dirEntry) {
		// Not found.
		return nullptr;
	}

	// Make sure this is a regular file.
	// TODO: What is an "associated" file?
	if (dirEntry->flags & (ISO_FLAG_ASSOCIATED | ISO_FLAG_DIRECTORY)) {
		// Not a regular file.
		m_lastError = ((dirEntry->flags & ISO_FLAG_DIRECTORY) ? EISDIR : EPERM);
		return nullptr;
	}

	// Block size.
	// Should be 2048, but other values are possible.
	const unsigned int block_size = d->pvd.logical_block_size.he;

	// Make sure the file is in bounds.
	const off64_t file_addr = (static_cast<off64_t>(dirEntry->block.he) - d->iso_start_offset) * block_size;
	if (file_addr >= d->partition_size + d->partition_offset ||
	    file_addr > d->partition_size + d->partition_offset - dirEntry->size.he)
	{
		// File is out of bounds.
		m_lastError = EIO;
		return nullptr;
	}

	// Create the PartitionFile.
	// This is an IRpFile implementation that uses an
	// IPartition as the reader and takes an offset
	// and size as the file parameters.
	return std::make_shared<PartitionFile>(this->shared_from_this(), file_addr, dirEntry->size.he);
}

/** IsoPartition-specific functions **/

/**
 * Get a file's timestamp.
 * @param filename Filename
 * @return Timestamp, or -1 on error.
 */
time_t IsoPartition::get_mtime(const char *filename)
{
	RP_D(IsoPartition);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	if (!m_file ||  !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	assert(filename != nullptr);
	if (!filename || filename[0] == 0) {
		// No filename.
		m_lastError = EINVAL;
		return -1;
	}

	// TODO: File reference counter.
	// This might be difficult to do because PartitionFile is a separate class.
	const ISO_DirEntry *const dirEntry = d->lookup(filename);
	if (!dirEntry) {
		// Not found.
		return -1;
	}

	// Parse the timestamp.
	return d->parseTimestamp(&dirEntry->mtime);
}

}
