/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SparseDiscReader.cpp: Disc reader base class for disc image formats     *
 * that use sparse and/or compressed blocks, e.g. CISO, WBFS, GCZ.         *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include "SparseDiscReader.hpp"
#include "SparseDiscReader_p.hpp"

#include "../file/IRpFile.hpp"

// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData {

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
	this->file = file->dup();

	// disc_size, pos, and block_size must be
	// set by the subclass.
}

SparseDiscReaderPrivate::~SparseDiscReaderPrivate()
{
	delete file;
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
	if (d->pos + (int64_t)size >= d->disc_size) {
		size = (size_t)(d->disc_size - d->pos);
	}

	// Check if we're not starting on a block boundary.
	const uint32_t block_size = d->block_size;
	const uint32_t blockStartOffset = d->pos % block_size;
	if (blockStartOffset != 0) {
		// Not a block boundary.
		// Read the end of the block.
		uint32_t read_sz = block_size - blockStartOffset;
		if (size < (size_t)read_sz) {
			read_sz = (uint32_t)size;
		}

		const unsigned int blockIdx = (unsigned int)(d->pos / block_size);
		int rd = this->readBlock(blockIdx, ptr8, blockStartOffset, read_sz);
		if (rd < 0 || rd != (int)read_sz) {
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
		const unsigned int blockIdx = (unsigned int)(d->pos / block_size);
		int rd = this->readBlock(blockIdx, ptr8, 0, block_size);
		if (rd < 0 || rd != (int)block_size) {
			// Error reading the data.
			return ret + (rd > 0 ? rd : 0);
		}
	}

	// Check if we still have data left. (not a full block)
	if (size > 0) {
		// Not a full block.
		assert(d->pos % block_size == 0);

		// Read the start of the block.
		const unsigned int blockIdx = (unsigned int)(d->pos / block_size);
		int rd = this->readBlock(blockIdx, ptr8, 0, size);
		if (rd < 0 || rd != (int)size) {
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
	// TODO: How does POSIX behave?
	if (pos < 0) {
		d->pos = 0;
	} else if (pos >= d->disc_size) {
		d->pos = d->disc_size;
	} else {
		d->pos = pos;
	}
	return 0;
}

/**
 * Seek to the beginning of the disc image.
 */
void SparseDiscReader::rewind(void)
{
	RP_D(SparseDiscReader);
	assert(d->file != nullptr);
	assert(d->disc_size > 0);
	assert(d->pos >= 0);
	assert(d->block_size != 0);
	if (!d->file || d->disc_size <= 0 || d->pos < 0 || d->block_size == 0) {
		// Disc image wasn't initialized properly.
		m_lastError = EBADF;
		// TODO: Return a value?
		return;
	}

	d->pos = 0;
}

/**
 * Get the disc image position.
 * @return Disc image position on success; -1 on error.
 */
int64_t SparseDiscReader::tell(void)
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

	return d->pos;
}

/**
 * Get the disc image size.
 * @return Disc image size, or -1 on error.
 */
int64_t SparseDiscReader::size(void)
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

	return d->disc_size;
}

}
