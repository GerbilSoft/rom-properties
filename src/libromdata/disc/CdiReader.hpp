/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CdiReader.hpp: DiscJuggler CDI image reader.                            *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "MultiTrackSparseDiscReader.hpp"
#include "IsoPartition.hpp"

// for ISOPtr
#include "../Media/ISO.hpp"

namespace LibRomData {

class CdiReaderPrivate;
class CdiReader : public MultiTrackSparseDiscReader
{
public:
	/**
	 * Construct a CdiReader with the specified file.
	 * The file is ref()'d, so the original file can be
	 * unref()'d by the caller afterwards.
	 * @param file File to read from.
	 */
	explicit CdiReader(const LibRpFile::IRpFilePtr &file);

private:
	typedef MultiTrackSparseDiscReader super;
	RP_DISABLE_COPY(CdiReader)
private:
	friend class CdiReaderPrivate;

public:
	/** Disc image detection functions **/

	/**
	 * Is a disc image supported by this class?
	 * @param pHeader Disc image header.
	 * @param szHeader Size of header.
	 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
	 */
	ATTR_ACCESS_SIZE(read_only, 1, 2)
	static int isDiscSupported_static(const uint8_t *pHeader, size_t szHeader);

	/**
	 * Is a disc image supported by this object?
	 * @param pHeader Disc image header.
	 * @param szHeader Size of header.
	 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	int isDiscSupported(const uint8_t *pHeader, size_t szHeader) const final;

protected:
	/** SparseDiscReader functions **/

	/**
	 * Get the physical address of the specified logical block index.
	 *
	 * NOTE: Not implemented in this subclass.
	 *
	 * @param blockIdx	[in] Block index.
	 * @return Physical block address. (-1 due to not being implemented)
	 */
	off64_t getPhysBlockAddr(uint32_t blockIdx) const final;

	/**
	 * Read the specified block.
	 *
	 * This can read either a full block or a partial block.
	 * For a full block, set pos = 0 and size = block_size.
	 *
	 * @param blockIdx	[in] Block index.
	 * @param pos		[in] Starting position. (Must be >= 0 and <= the block size!)
	 * @param ptr		[out] Output data buffer.
	 * @param size		[in] Amount of data to read, in bytes. (Must be <= the block size!)
	 * @return Number of bytes read, or -1 if the block index is invalid.
	 */
	ATTR_ACCESS_SIZE(write_only, 4, 5)
	int readBlock(uint32_t blockIdx, int pos, void *ptr, size_t size) final;

public:
	/** MultiTrackSparseDiscReader functions **/

	/**
	 * Get the track count.
	 * @return Track count.
	 */
	int trackCount(void) const final;

	/**
	 * Get the starting LBA of the specified track number.
	 * @param trackNumber Track number. (1-based)
	 * @return Starting LBA, or -1 if the track number is invalid.
	 */
	int startingLBA(int trackNumber) const final;

	/**
	 * Open a track using IsoPartition.
	 * @param trackNumber Track number. (1-based)
	 * @return IsoPartition, or nullptr on error.
	 */
	IsoPartitionPtr openIsoPartition(int trackNumber) final;

	/**
	 * Create an ISO RomData object for a given track number.
	 * @param trackNumber Track number. (1-based)
	 * @return ISO object, or nullptr on error.
	 */
	ISOPtr openIsoRomData(int trackNumber) final;
};

typedef std::shared_ptr<CdiReader> CdiReaderPtr;

} // namespace LibRomData
