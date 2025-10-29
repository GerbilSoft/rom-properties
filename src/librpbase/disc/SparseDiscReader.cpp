/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SparseDiscReader.cpp: Disc reader base class for disc image formats     *
 * that use sparse and/or compressed blocks, e.g. CISO, WBFS, GCZ.         *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "SparseDiscReader.hpp"
#include "SparseDiscReader_p.hpp"

// librpfile
using namespace LibRpFile;

namespace LibRpBase {

/** SparseDiscReaderPrivate **/

SparseDiscReaderPrivate::SparseDiscReaderPrivate(SparseDiscReader *q)
	: q_ptr(q)
	, disc_size(0)
	, pos(-1)
	, block_size(0)
{
	// NOTE: Can't check q->m_file here.

	// disc_size, pos, and block_size must be
	// set by the subclass.

	// Clear CdromSectorInfo.
	hasCdromInfo = false;
	cdromSectorInfo.mode = 0;
	cdromSectorInfo.sector_size = 0;
	cdromSectorInfo.subchannel_size = 0;
}

/** SparseDiscReader **/

SparseDiscReader::SparseDiscReader(SparseDiscReaderPrivate *d, const IRpFilePtr &file)
	: super(file)
	, d_ptr(d)
{ }

SparseDiscReader::~SparseDiscReader()
{
	delete d_ptr;
}

/** IDiscReader functions. **/

/**
 * Read data from the disc image.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t SparseDiscReader::read(void *ptr, size_t size)
{
	RP_D(SparseDiscReader);
	assert(m_file != nullptr);
	assert(d->disc_size > 0);
	assert(d->pos >= 0);
	assert(d->block_size != 0);
	if (!m_file || d->disc_size <= 0 || d->pos < 0 || d->block_size == 0) {
		// Disc image wasn't initialized properly.
		m_lastError = EBADF;
		return 0;
	}

	uint8_t *ptr8 = static_cast<uint8_t*>(ptr);
	size_t ret = 0;

	// Are we already at the end of the disc?
	if (d->pos >= d->disc_size) {
		// End of the disc.
		return 0;
	}

	// Make sure d->pos + size <= d->disc_size.
	// If it isn't, we'll do a short read.
	if (d->pos + static_cast<off64_t>(size) >= d->disc_size) {
		size = static_cast<size_t>(d->disc_size - d->pos);
	}

	// Check if we're not starting on a block boundary.
	const uint32_t block_size = d->block_size;
	const uint32_t blockStartOffset = d->pos % block_size;
	if (blockStartOffset != 0) {
		// Not a block boundary.
		// Read the end of the block.
		uint32_t read_sz = block_size - blockStartOffset;
		if (size < static_cast<size_t>(read_sz)) {
			read_sz = static_cast<uint32_t>(size);
		}

		const unsigned int blockIdx = static_cast<unsigned int>(d->pos / block_size);
		int rd = this->readBlock(blockIdx, blockStartOffset, ptr8, read_sz);
		if (rd < 0 || rd != static_cast<int>(read_sz)) {
			// Error reading the data.
			return (rd > 0 ? rd : 0);
		}

		// Starting block read.
		size -= read_sz;
		ptr8 += read_sz;
		ret += read_sz;
		d->pos += read_sz;
	}

	// Read entire blocks.
	for (; size >= block_size;
	    size -= block_size, ptr8 += block_size,
	    ret += block_size, d->pos += block_size)
	{
		assert(d->pos % block_size == 0);
		const unsigned int blockIdx = static_cast<unsigned int>(d->pos / block_size);
		int rd = this->readBlock(blockIdx, 0, ptr8, block_size);
		if (rd < 0 || rd != static_cast<int>(block_size)) {
			// Error reading the data.
			return ret + (rd > 0 ? rd : 0);
		}
	}

	// Check if we still have data left. (not a full block)
	if (size > 0) {
		// Not a full block.
		assert(d->pos % block_size == 0);

		// Read the start of the block.
		const unsigned int blockIdx = static_cast<unsigned int>(d->pos / block_size);
		int rd = this->readBlock(blockIdx, 0, ptr8, size);
		if (rd < 0 || rd != static_cast<int>(size)) {
			// Error reading the data.
			return ret + (rd > 0 ? rd : 0);
		}

		ret += size;
		d->pos += size;
	}

	// Finished reading the data.
	return ret;
}

/**
 * Set the disc image position.
 * @param pos		[in] Disc image position
 * @param whence	[in] Where to seek from
 * @return 0 on success; -1 on error.
 */
int SparseDiscReader::seek(off64_t pos, SeekWhence whence)
{
	RP_D(SparseDiscReader);
	assert(m_file != nullptr);
	assert(d->disc_size > 0);
	assert(d->pos >= 0);
	assert(d->block_size != 0);
	if (!m_file || d->disc_size <= 0 || d->pos < 0 || d->block_size == 0) {
		// Disc image wasn't initialized properly.
		m_lastError = EBADF;
		return -1;
	}

	pos = adjust_file_pos_for_whence(pos, whence, d->pos, d->disc_size);
	if (pos < 0) {
		// Negative is invalid.
		m_lastError = EINVAL;
		return -1;
	}
	d->pos = constrain_file_pos(pos, d->disc_size);
	return 0;
}

/**
 * Get the disc image position.
 * @return Disc image position on success; -1 on error.
 */
off64_t SparseDiscReader::tell(void)
{
	RP_D(const SparseDiscReader);
	assert(m_file != nullptr);
	assert(d->disc_size > 0);
	assert(d->pos >= 0);
	assert(d->block_size != 0);
	if (!m_file || d->disc_size <= 0 || d->pos < 0 || d->block_size == 0) {
		// Disc image wasn't initialized properly.
		m_lastError = EBADF;
		return -1;
	}

	return d->pos;
}

/**
 * Get the disc image size.
 * @return Disc image size, or -1 on error.
 */
off64_t SparseDiscReader::size(void)
{
	RP_D(const SparseDiscReader);
	assert(m_file != nullptr);
	assert(d->disc_size > 0);
	assert(d->pos >= 0);
	assert(d->block_size != 0);
	if (!m_file || d->disc_size <= 0 || d->pos < 0 || d->block_size == 0) {
		// Disc image wasn't initialized properly.
		m_lastError = EBADF;
		return -1;
	}

	return d->disc_size;
}

/** SparseDiscReader-specific properties **/

// CD-ROM specific information

/**
 * Get the CD-ROM sector information.
 * @return CD-ROM sector information, or nullptr if not set.
 */
const CdromSectorInfo *SparseDiscReader::cdromSectorInfo(void) const
{
	RP_D(const SparseDiscReader);
	return (d->hasCdromInfo) ? &d->cdromSectorInfo : nullptr;
}

/** SparseDiscReader **/

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
int SparseDiscReader::readBlock(uint32_t blockIdx, int pos, void *ptr, size_t size)
{
	// Read 'size' bytes of block 'blockIdx', starting at 'pos'.
	// NOTE: This can only be called by SparseDiscReader,
	// so the main assertions are already checked there.
	RP_D(SparseDiscReader);
	assert(pos >= 0 && pos < (int)d->block_size);
	assert(size <= d->block_size);
	// TODO: Make sure overflow doesn't occur.
	assert(static_cast<off64_t>(pos + size) <= static_cast<off64_t>(d->block_size));
	if (pos < 0 || static_cast<off64_t>(pos + size) > static_cast<off64_t>(d->block_size)) {
		// pos+size is out of range.
		return -1;
	}

	if (unlikely(size == 0)) {
		// Nothing to read.
		return 0;
	}

	// Get the physical address first.
	const off64_t physBlockAddr = getPhysBlockAddr(blockIdx);
	assert(physBlockAddr >= 0);
	if (physBlockAddr < 0) {
		// Out of range.
		return -1;
	}

	if (physBlockAddr == 0) {
		// Empty block.
		memset(ptr, 0, size);
		return static_cast<int>(size);
	}

	// Read from the block.
	size_t sz_read = m_file->seekAndRead(physBlockAddr + pos, ptr, size);
	m_lastError = m_file->lastError();
	return (sz_read > 0 ? (int)sz_read : -1);
}

}
