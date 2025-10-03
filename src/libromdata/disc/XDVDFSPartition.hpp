/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XDVDFSPartition.hpp: Microsoft Xbox XDVDFS partition reader.            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/disc/IPartition.hpp"

// C includes. (C++ namespace)
#include <ctime>

namespace LibRomData {

class XDVDFSPartitionPrivate;
class XDVDFSPartition final : public LibRpBase::IPartition
{
public:
	/**
	 * Construct an XDVDFSPartition with the specified IDiscReader.
	 *
	 * NOTE: The IDiscReader *must* remain valid while this
	 * XDVDFSPartition is open.
	 *
	 * @param discReader IDiscReader (or IRpFile)
	 * @param partition_offset Partition start offset
	 * @param partition_size Partition size
	 */
	XDVDFSPartition(const LibRpFile::IRpFilePtr &discReader, off64_t partition_offset, off64_t partition_size);
public:
	~XDVDFSPartition() final;

private:
	typedef IPartition super;
	RP_DISABLE_COPY(XDVDFSPartition)

protected:
	friend class XDVDFSPartitionPrivate;
	XDVDFSPartitionPrivate *const d_ptr;

public:
	/** IDiscReader **/

	/**
	 * Read data from the partition.
	 * @param ptr Output data buffer.
	 * @param size Amount of data to read, in bytes.
	 * @return Number of bytes read.
	 */
	ATTR_ACCESS_SIZE(write_only, 2, 3)
	size_t read(void *ptr, size_t size) override;

	/**
	 * Set the partition position.
	 * @param pos		[in] Partition position
	 * @param whence	[in] Where to seek from
	 * @return 0 on success; -1 on error.
	 */
	int seek(off64_t pos, SeekWhence whence) override;

	/**
	 * Get the partition position.
	 * @return Partition position on success; -1 on error.
	 */
	off64_t tell(void) override;

	/**
	 * Get the data size.
	 * This size does not include the partition header,
	 * and it's adjusted to exclude hashes.
	 * @return Data size, or -1 on error.
	 */
	off64_t size(void) final;

public:
	/** IPartition **/

	/**
	 * Get the partition size.
	 * This size includes the partition header and hashes.
	 * @return Partition size, or -1 on error.
	 */
	off64_t partition_size(void) const final;

	/**
	 * Get the used partition size.
	 * This size includes the partition header and hashes,
	 * but does not include "empty" sectors.
	 * @return Used partition size, or -1 on error.
	 */
	off64_t partition_size_used(void) const final;

public:
	/** IFst wrapper functions **/

	// TODO
#if 0
	/**
	 * Open a directory.
	 * @param path	[in] Directory path.
	 * @return IFst::Dir*, or nullptr on error.
	 */
	LibRpBase::IFst::Dir *opendir(const char *path) final;

	/**
	 * Read a directory entry.
	 * @param dirp IFst::Dir pointer.
	 * @return IFst::DirEnt, or nullptr if end of directory or on error.
	 * (TODO: Add lastError()?)
	 */
	LibRpBase::IFst::DirEnt *readdir(LibRpBase::IFst::Dir *dirp) final;

	/**
	 * Close an opened directory.
	 * @param dirp IFst::Dir pointer.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int closedir(LibRpBase::IFst::Dir *dirp) final;
#endif

	/**
	 * Open a file. (read-only)
	 * @param filename Filename.
	 * @return IRpFile*, or nullptr on error.
	 */
	LibRpFile::IRpFilePtr open(const char *filename) final;

public:
	/** XDVDFSPartition **/

	/**
	 * Get the XDVDFS timestamp.
	 * @return XDVDFS timestamp, or -1 on error.
	 */
	time_t xdvdfsTimestamp(void) const;
};

typedef std::shared_ptr<XDVDFSPartition> XDVDFSPartitionPtr;

}
