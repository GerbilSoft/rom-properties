/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * PartitionFile.hpp: IRpFile implementation for IPartition.               *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpfile/IRpFile.hpp"
#include "IDiscReader.hpp"

namespace LibRpBase {

class PartitionFile final : public LibRpFile::IRpFile
{
public:
	/**
	 * Open a file from an IPartition.
	 * NOTE: These files are read-only.
	 *
	 * @param partition	[in] IPartition (or IDiscReader) object
	 * @param offset	[in] File starting offset
	 * @param size		[in] File size
	 */
	PartitionFile(const IDiscReaderPtr& partition, off64_t offset, off64_t size);

private:
	typedef IRpFile super;
	RP_DISABLE_COPY(PartitionFile)

public:
	/**
	 * Is the file open?
	 * This usually only returns false if an error occurred.
	 * @return True if the file is open; false if it isn't.
	 */
	bool isOpen(void) const final;

	/**
	 * Close the file.
	 */
	void close(void) final;

	/**
	 * Read data from the file.
	 * @param ptr Output data buffer.
	 * @param size Amount of data to read, in bytes.
	 * @return Number of bytes read.
	 */
	ATTR_ACCESS_SIZE(write_only, 2, 3)
	size_t read(void *ptr, size_t size) final;

	/**
	 * Write data to the file.
	 * (NOTE: Not valid for PartitionFile; this will always return 0.)
	 * @param ptr Input data buffer.
	 * @param size Amount of data to read, in bytes.
	 * @return Number of bytes written.
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	size_t write(const void *ptr, size_t size) final;

	/**
	 * Set the file position.
	 * @param pos		[in] File position
	 * @param whence	[in] Where to seek from
	 * @return 0 on success; -1 on error.
	 */
	int seek(off64_t pos, SeekWhence whence) final;

	/**
	 * Get the file position.
	 * @return File position, or -1 on error.
	 */
	off64_t tell(void) final;

public:
	/** File properties **/

	/**
	 * Get the file size.
	 * @return File size, or negative on error.
	 */
	off64_t size(void) final;

public:
	/** PartitionFile functions **/

	/**
	 * Get the underlying IDiscReader.
	 * @return IDiscReaderPtr
	 */
	inline IDiscReaderPtr getIDiscReader(void)
	{
		return m_partition;
	}

protected:
	IDiscReaderPtr m_partition;
	off64_t m_offset;	// File starting offset.
	off64_t m_size;		// File size.
	off64_t m_pos;		// Current position.
};

typedef std::shared_ptr<PartitionFile> PartitionFilePtr;

}
