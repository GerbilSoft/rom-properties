/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Cdrom2352Reader.hpp: CD-ROM reader for 2352-byte sector images.         *
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

/**
 * References:
 * - https://github.com/qeedquan/ecm/blob/master/format.txt
 * - https://github.com/Karlson2k/libcdio-k2k/blob/master/include/cdio/sector.h
 */

#include "Cdrom2352Reader.hpp"
#include "librpbase/disc/SparseDiscReader_p.hpp"
#include "../cdrom_structs.h"

// librpbase
#include "librpbase/byteswap.h"
#include "librpbase/file/IRpFile.hpp"
using namespace LibRpBase;

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

namespace LibRomData {

class Cdrom2352ReaderPrivate : public SparseDiscReaderPrivate {
	public:
		Cdrom2352ReaderPrivate(Cdrom2352Reader *q, IRpFile *file);

	private:
		typedef SparseDiscReaderPrivate super;
		RP_DISABLE_COPY(Cdrom2352ReaderPrivate)

	public:
		// CD-ROM sync magic.
		static const uint8_t CDROM_2352_MAGIC[12];

		// Physical block size.
		static const unsigned int physBlockSize = 2352;

		// Number of 2352-byte blocks.
		unsigned int blockCount;
};

/** Cdrom2352ReaderPrivate **/

// CD-ROM sync magic magic.
const uint8_t Cdrom2352ReaderPrivate::CDROM_2352_MAGIC[12] =
	{0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00};

Cdrom2352ReaderPrivate::Cdrom2352ReaderPrivate(Cdrom2352Reader *q, IRpFile *file)
	: super(q, file)
	, blockCount(0)
{
	if (!this->file) {
		// File could not be dup()'d.
		return;
	}

	// Check the disc size.
	// Should be a multiple of 2352.
	int64_t fileSize = file->size();
	if (fileSize <= 0 || fileSize % 2352 != 0) {
		// Invalid disc size.
		delete this->file;
		this->file = nullptr;
		q->m_lastError = EIO;
		return;
	}

	// Disc parameters.
	// TODO: 64-bit block count?
	blockCount = (unsigned int)(fileSize / 2352);
	block_size = 2048;
	disc_size = fileSize / 2352 * 2048;

	// Reset the disc position.
	pos = 0;
}

/** Cdrom2352Reader **/

Cdrom2352Reader::Cdrom2352Reader(IRpFile *file)
	: super(new Cdrom2352ReaderPrivate(this, file))
{ }

/**
 * Is a disc image supported by this class?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
 */
int Cdrom2352Reader::isDiscSupported_static(const uint8_t *pHeader, size_t szHeader)
{
	if (szHeader < 2352) {
		// Not enough data to check.
		return -1;
	}

	// Check the CD-ROM sync magic.
	if (!memcmp(pHeader, Cdrom2352ReaderPrivate::CDROM_2352_MAGIC,
	     sizeof(Cdrom2352ReaderPrivate::CDROM_2352_MAGIC)))
	{
		// Valid CD-ROM sync magic.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Is a disc image supported by this object?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Cdrom2352Reader::isDiscSupported(const uint8_t *pHeader, size_t szHeader) const
{
	return isDiscSupported_static(pHeader, szHeader);
}

/** SparseDiscReader functions. **/

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
int Cdrom2352Reader::readBlock(uint32_t blockIdx, void *ptr, int pos, size_t size)
{
	// Read 'size' bytes of block 'blockIdx', starting at 'pos'.
	// NOTE: This can only be called by SparseDiscReader,
	// so the main assertions are already checked there.
	RP_D(Cdrom2352Reader);
	assert(pos >= 0 && pos < (int)d->block_size);
	assert(size <= d->block_size);
	assert(blockIdx < d->blockCount);
	// TODO: Make sure overflow doesn't occur.
	assert((int64_t)(pos + size) <= (int64_t)d->block_size);
	if (pos < 0 || pos >= (int)d->block_size || size > d->block_size ||
	    (int64_t)(pos + size) > (int64_t)d->block_size ||
	    blockIdx >= d->blockCount)
	{
		// pos+size is out of range.
		return -1;
	}

	if (unlikely(size == 0)) {
		// Nothing to read.
		return 0;
	}

	// Go to the block.
	// FIXME: Read the whole block so we can determine if this is Mode1 or Mode2.
	// Mode1 data starts at byte 16; Mode2 data starts at byte 24.
	const int64_t phys_pos = ((int64_t)blockIdx * d->physBlockSize) + 16 + pos;
	size_t sz_read = d->file->seekAndRead(phys_pos, ptr, size);
	m_lastError = d->file->lastError();
	return (sz_read > 0 ? (int)sz_read : -1);
}

}
