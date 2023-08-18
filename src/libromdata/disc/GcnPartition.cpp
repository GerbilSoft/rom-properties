/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GcnPartition.cpp: GameCube partition reader.                            *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GcnPartition.hpp"
#include "GcnFst.hpp"

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;

// C++ STL classes
using std::shared_ptr;

#include "GcnPartition_p.hpp"
namespace LibRomData {

/** GcnPartition **/

/**
 * Construct a GcnPartition with the specified IDiscReader.
 *
 * NOTE: The IDiscReader *must* remain valid while this
 * GcnPartition is open.
 *
 * @param discReader IDiscReader.
 * @param partition_offset Partition start offset.
 */
GcnPartition::GcnPartition(IDiscReader *discReader, off64_t partition_offset)
	: super(discReader)
	, d_ptr(new GcnPartitionPrivate(this, partition_offset, discReader->size()))
{ }

GcnPartition::~GcnPartition()
{
	delete d_ptr;
}

/**
 * Construct a GcnPartition. (subclass version)
 * @param d GcnPartitionPrivate subclass.
 * @param discReader IDiscReader.
 */
GcnPartition::GcnPartition(GcnPartitionPrivate *d, IDiscReader *discReader)
	: super(discReader)
	, d_ptr(d)
{ }

/** IDiscReader **/

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t GcnPartition::read(void *ptr, size_t size)
{
	assert(m_discReader != nullptr);
	assert(m_discReader->isOpen());
	if (!m_discReader || !m_discReader->isOpen()) {
		m_lastError = EBADF;
		return 0;
	}

	// GCN partitions are stored as-is.
	// TODO: data_size checks?
	return m_discReader->read(ptr, size);
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int GcnPartition::seek(off64_t pos)
{
	RP_D(GcnPartition);
	assert(m_discReader != nullptr);
	assert(m_discReader->isOpen());
	if (!m_discReader ||  !m_discReader->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// Use the IDiscReader directly for GCN partitions.
	int ret = m_discReader->seek(d->data_offset + pos);
	if (ret != 0) {
		m_lastError = m_discReader->lastError();
	}
	return ret;
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
off64_t GcnPartition::tell(void)
{
	assert(m_discReader != nullptr);
	assert(m_discReader->isOpen());
	if (!m_discReader || !m_discReader->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// Use the IDiscReader directly for GCN partitions.
	off64_t ret = m_discReader->tell();
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
off64_t GcnPartition::size(void)
{
	// TODO: Errors?
	RP_D(const GcnPartition);
	return d->data_size;
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
off64_t GcnPartition::partition_size(void) const
{
	// TODO: Errors?
	RP_D(const GcnPartition);
	return d->partition_size;
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
off64_t GcnPartition::partition_size_used(void) const
{
	RP_D(const GcnPartition);
	int ret = const_cast<GcnPartitionPrivate*>(d)->loadBootBlockAndInfo();
	if (ret != 0) {
		// Error loading the boot block.
		return -1;
	}
	if (!d->fst) {
		// FST isn't loaded.
		if (const_cast<GcnPartitionPrivate*>(d)->loadFst() != 0) {
			// FST load failed.
			// TODO: Errors?
			return -1;
		}
	}

	// FST/DOL offset and size.
	off64_t size;
	if (d->bootBlock.dol_offset > d->bootBlock.fst_offset) {
		// DOL is after the FST.
		// TODO: Get the DOL size. (This case is unlikely, though...)
		size = static_cast<off64_t>(d->bootBlock.dol_offset);
	} else {
		// FST is after the DOL.
		size = static_cast<off64_t>(d->bootBlock.fst_offset) +
		       static_cast<off64_t>(d->bootBlock.fst_size);
	}
	size <<= d->offsetShift;

	// Get the FST used size.
	size += d->fst->totalUsedSize();

	// Add the difference between partition and data sizes.
	size += (d->partition_size - d->data_size);

	// We're done here.
	return size;
}

/** GcnPartition **/

/** GcnFst wrapper functions. **/

/**
 * Open a directory.
 * @param path	[in] Directory path.
 * @return IFst::Dir*, or nullptr on error.
 */
IFst::Dir *GcnPartition::opendir(const char *path)
{
	RP_D(GcnPartition);
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
	RP_D(GcnPartition);
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
	RP_D(GcnPartition);
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
shared_ptr<IRpFile> GcnPartition::open(const char *filename)
{
	// TODO: File reference counter.
	// This might be difficult to do because GcnFile is a separate class.
	RP_D(GcnPartition);
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
	return std::make_shared<PartitionFile>(this, dirent.offset, dirent.size);
}

}
