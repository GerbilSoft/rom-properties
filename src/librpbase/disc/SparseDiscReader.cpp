/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SparseDiscReader.cpp: Disc reader base class for disc image formats     *
 * that use sparse and/or compressed blocks, e.g. CISO, WBFS, GCZ.         *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "SparseDiscReader.hpp"
#include "SparseDiscReader_p.hpp"

#include "../file/IRpFile.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

namespace LibRpBase {

/** SparseDiscReaderPrivate **/

SparseDiscReaderPrivate::SparseDiscReaderPrivate(SparseDiscReader *q, IRpFile *file)
	: q_ptr(q)
	, file(nullptr)
	, disc_size(0)
	, pos(-1)
	, block_size(0)
{
	if (!file) {
		q->m_lastError = EBADF;
		return;
	}
	this->file = file->ref();

	// disc_size, pos, and block_size must be
	// set by the subclass.
}

SparseDiscReaderPrivate::~SparseDiscReaderPrivate()
{
	if (this->file) {
		this->file->unref();
	}
}

/** SparseDiscReader **/

SparseDiscReader::SparseDiscReader(SparseDiscReaderPrivate *d)
	: d_ptr(d)
{ }

SparseDiscReader::~SparseDiscReader()
{
	delete d_ptr;
}

/** IDiscReader functions. **/

/**
 * Is the disc image open?
 * This usually only returns false if an error occurred.
 * @return True if the disc image is open; false if it isn't.
 */
bool SparseDiscReader::isOpen(void) const
{
	RP_D(const SparseDiscReader);
	return (d->file != nullptr);
}

/**
 * Read data from the disc image.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t SparseDiscReader::read(void *ptr, size_t size)
{
	RP_D(SparseDiscReader);
	assert(d->file != nullptr);
	assert(d->disc_size > 0);
	assert(d->pos >= 0);
	assert(d->block_size != 0);
	if (!d->file || d->disc_size <= 0 || d->pos < 0 || d->block_size == 0) {
		// Disc image wasn't initialized properly.
		m_lastError = EBADF;
		return -1;
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
	if (d->pos + static_cast<int64_t>(size) >= d->disc_size) {
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
		int rd = this->readBlock(blockIdx, ptr8, blockStartOffset, read_sz);
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
		int rd = this->readBlock(blockIdx, ptr8, 0, block_size);
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
		int rd = this->readBlock(blockIdx, ptr8, 0, size);
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
 * @param pos disc image position.
 * @return 0 on success; -1 on error.
 */
int SparseDiscReader::seek(int64_t pos)
{
	RP_D(SparseDiscReader);
	assert(d->file != nullptr);
	assert(d->disc_size > 0);
	assert(d->pos >= 0);
	assert(d->block_size != 0);
	if (!d->file || d->disc_size <= 0 || d->pos < 0 || d->block_size == 0) {
		// Disc image wasn't initialized properly.
		m_lastError = EBADF;
		return -1;
	}

	// Handle out-of-range cases.
	if (pos < 0) {
		// Negative is invalid.
		m_lastError = EINVAL;
		return -1;
	} else if (pos >= d->disc_size) {
		d->pos = d->disc_size;
	} else {
		d->pos = pos;
	}
	return 0;
}

/**
 * Get the disc image position.
 * @return Disc image position on success; -1 on error.
 */
int64_t SparseDiscReader::tell(void)
{
	RP_D(const SparseDiscReader);
	assert(d->file != nullptr);
	assert(d->disc_size > 0);
	assert(d->pos >= 0);
	assert(d->block_size != 0);
	if (!d->file || d->disc_size <= 0 || d->pos < 0 || d->block_size == 0) {
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
int64_t SparseDiscReader::size(void)
{
	RP_D(const SparseDiscReader);
	assert(d->file != nullptr);
	assert(d->disc_size > 0);
	assert(d->pos >= 0);
	assert(d->block_size != 0);
	if (!d->file || d->disc_size <= 0 || d->pos < 0 || d->block_size == 0) {
		// Disc image wasn't initialized properly.
		m_lastError = EBADF;
		return -1;
	}

	return d->disc_size;
}

/**
 * Read the specified block.
 *
 * This can read either a full block or a partial block.
 * For a full block, set pos = 0 and size = block_size.
 *
 * @param blockIdx	[in] Block index.
 * @param ptr		[out] Output data buffer.
 * @param pos		[in] Starting position. (Must be >= 0 and <= the block size!)
 * @param size		[in] Amount of data to read, in bytes. (Must be <= the block size!)
 * @return Number of bytes read, or -1 if the block index is invalid.
 */
int SparseDiscReader::readBlock(uint32_t blockIdx, void *ptr, int pos, size_t size)
{
	// Read 'size' bytes of block 'blockIdx', starting at 'pos'.
	// NOTE: This can only be called by SparseDiscReader,
	// so the main assertions are already checked there.
	RP_D(SparseDiscReader);
	assert(pos >= 0 && pos < (int)d->block_size);
	assert(size <= d->block_size);
	// TODO: Make sure overflow doesn't occur.
	assert(static_cast<int64_t>(pos + size) <= static_cast<int64_t>(d->block_size));
	if (pos < 0 || static_cast<int64_t>(pos + size) > static_cast<int64_t>(d->block_size)) {
		// pos+size is out of range.
		return -1;
	}

	if (unlikely(size == 0)) {
		// Nothing to read.
		return 0;
	}

	// Get the physical address first.
	const int64_t physBlockAddr = getPhysBlockAddr(blockIdx);
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
	size_t sz_read = d->file->seekAndRead(physBlockAddr + pos, ptr, size);
	m_lastError = d->file->lastError();
	return (sz_read > 0 ? (int)sz_read : -1);
}

}
