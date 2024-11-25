/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUH3Reader.hpp: Wii U H3 content reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/config.librpbase.h"
#include "librpbase/disc/IDiscReader.hpp"
#include "librpfile/IRpFile.hpp"

namespace LibRomData {

class WiiUH3ReaderPrivate;
class WiiUH3Reader final : public LibRpBase::IPartition
{
public:
	/**
	 * Construct a WiiUH3Reader with the specified IRpFile.
	 * @param file		[in] IRpFile
	 * @param pKey		[in] Encryption key
	 * @param keyLen	[in] Length of pKey  (should be 16)
	 */
	ATTR_ACCESS_SIZE(read_only, 3, 4)
	WiiUH3Reader(const LibRpFile::IRpFilePtr &discReader, const uint8_t *pKey, size_t keyLen);

private:
	typedef IPartition super;
	RP_DISABLE_COPY(WiiUH3Reader)

protected:
	friend class WiiUH3ReaderPrivate;
	WiiUH3ReaderPrivate *const d_ptr;

public:
	/** IDiscReader **/

	/**
	 * Read data from the partition.
	 * @param ptr Output data buffer.
	 * @param size Amount of data to read, in bytes.
	 * @return Number of bytes read.
	 */
	ATTR_ACCESS_SIZE(write_only, 2, 3)
	size_t read(void *ptr, size_t size) final;

	/**
	 * Set the partition position.
	 * @param pos Partition position.
	 * @return 0 on success; -1 on error.
	 */
	int seek(off64_t pos) final;

	/**
	 * Get the partition position.
	 * @return Partition position on success; -1 on error.
	 */
	off64_t tell(void) final;

	/**
	 * Get the data size.
	 * This size does not include hashes.
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
	off64_t partition_size_used(void) const override;
};

typedef std::shared_ptr<WiiUH3Reader> WiiUH3ReaderPtr;

}
