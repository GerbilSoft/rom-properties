/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GcnPartition.cpp: GameCube partition reader.                            *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#include "GcnPartition.hpp"
#include "config.libromdata.h"

#include "byteswap.h"
#include "gcn_structs.h"
#include "IDiscReader.hpp"
#include "GcnFst.hpp"
#include "PartitionFile.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <memory>
using std::unique_ptr;

#include "GcnPartitionPrivate.hpp"

namespace LibRomData {

/** GcnPartition **/

/**
 * Construct a GcnPartition with the specified IDiscReader.
 *
 * NOTE: The IDiscReader *must* remain valid while this
 * GcnPartition is open.
 *
 * TODO: TGC support?
 *
 * @param discReader IDiscReader.
 * @param partition_offset Partition start offset.
 */
GcnPartition::GcnPartition(IDiscReader *discReader, int64_t partition_offset)
	: d_ptr(new GcnPartitionPrivate(this, discReader, partition_offset))
{ }

GcnPartition::~GcnPartition()
{
	delete d_ptr;
}

/**
 * Construct a GcnPartition. (subclass version)
 * @param d GcnPartitionPrivate subclass.
 */
GcnPartition::GcnPartition(GcnPartitionPrivate *d)
	: d_ptr(d)
{ }

/** IDiscReader **/

/**
 * Is the partition open?
 * This usually only returns false if an error occurred.
 * @return True if the partition is open; false if it isn't.
 */
bool GcnPartition::isOpen(void) const
{
	const GcnPartitionPrivate *d = reinterpret_cast<const GcnPartitionPrivate*>(d_ptr);
	return (d->discReader && d->discReader->isOpen());
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t GcnPartition::read(void *ptr, size_t size)
{
	GcnPartitionPrivate *d = reinterpret_cast<GcnPartitionPrivate*>(d_ptr);
	assert(d->discReader != nullptr);
	assert(d->discReader->isOpen());
	if (!d->discReader || !d->discReader->isOpen()) {
		m_lastError = EBADF;
		return 0;
	}

	// GCN partitions are stored as-is.
	// TODO: data_size checks?
	return d->discReader->read(ptr, size);
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int GcnPartition::seek(int64_t pos)
{
	GcnPartitionPrivate *d = reinterpret_cast<GcnPartitionPrivate*>(d_ptr);
	assert(d->discReader != nullptr);
	assert(d->discReader->isOpen());
	if (!d->discReader ||  !d->discReader->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// Use the IDiscReader directly for GCN partitions.
	return d->discReader->seek(d->data_offset + pos);
}

/**
 * Seek to the beginning of the partition.
 */
void GcnPartition::rewind(void)
{
	seek(0);
}

/**
 * Get the data size.
 * This size does not include the partition header,
 * and it's adjusted to exclude hashes.
 * @return Data size, or -1 on error.
 */
int64_t GcnPartition::size(void)
{
	// TODO: Errors?
	const GcnPartitionPrivate *d = reinterpret_cast<const GcnPartitionPrivate*>(d_ptr);
	return d->data_size;
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
int64_t GcnPartition::partition_size(void) const
{
	// TODO: Errors?
	const GcnPartitionPrivate *d = reinterpret_cast<const GcnPartitionPrivate*>(d_ptr);
	return d->partition_size;
}

/** GcnPartition **/

/** GcnFst wrapper functions. **/

/**
 * Open a directory.
 * @param path	[in] Directory path. [TODO; always reads "/" right now.]
 * @return IFst::Dir*, or nullptr on error.
 */
IFst::Dir *GcnPartition::opendir(const rp_char *path)
{
	GcnPartitionPrivate *d = reinterpret_cast<GcnPartitionPrivate*>(d_ptr);
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
IFst::DirEnt *GcnPartition::readdir(IFst::Dir *dirp)
{
	GcnPartitionPrivate *d = reinterpret_cast<GcnPartitionPrivate*>(d_ptr);
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
int GcnPartition::closedir(IFst::Dir *dirp)
{
	GcnPartitionPrivate *d = reinterpret_cast<GcnPartitionPrivate*>(d_ptr);
	if (!d->fst) {
		// TODO: Errors?
		return -EBADF;
	}

	return d->fst->closedir(dirp);
}

/**
 * Open a file. (read-only)
 * @param filename Filename.
 * @return IRpFile*, or nullptr on error.
 */
IRpFile *GcnPartition::open(const rp_char *filename)
{
	// TODO: File reference counter.
	// This might be difficult to do because GcnFile is a separate class.
	GcnPartitionPrivate *d = reinterpret_cast<GcnPartitionPrivate*>(d_ptr);
	if (!d->fst) {
		// FST isn't loaded.
		if (d->loadFst() != 0) {
			// FST load failed.
			m_lastError = EIO;
			return nullptr;
		}
	}

	if (!filename) {
		// No filename.
		m_lastError = EINVAL;
		return nullptr;
	}

	// Find the file in the FST.
	IFst::DirEnt dirent;
	int ret = d->fst->find_file(filename, &dirent);
	if (ret != 0) {
		// File not found.
		m_lastError = ENOENT;
		return nullptr;
	}

	// Make sure this is a regular file.
	if (dirent.type != DT_REG) {
		// Not a regular file.
		m_lastError = (dirent.type == DT_DIR ? EISDIR : EPERM);
		return nullptr;
	}

	// Make sure the file is in bounds.
	if (dirent.offset >= d->partition_size ||
	    dirent.offset > d->partition_size - dirent.size)
	{
		// File is out of bounds.
		m_lastError = EIO;
		return nullptr;
	}

	// Create the PartitionFile.
	// This is an IRpFile implementation that uses an
	// IPartition as the reader and takes an offset
	// and size as the file parameters.
	return new PartitionFile(this, dirent.offset, dirent.size);
}

}
