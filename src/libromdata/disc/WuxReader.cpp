/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WuxReader.cpp: Wii U .wux disc image reader.                            *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

// References:
// - https://gbatemp.net/threads/wii-u-image-wud-compression-tool.397901/

#include "WuxReader.hpp"
#include "librpbase/disc/SparseDiscReader_p.hpp"
#include "wux_structs.h"

// librpbase
#include "librpbase/byteswap.h"
#include "librpbase/file/IRpFile.hpp"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <array>
using std::array;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData {

class WuxReaderPrivate : public SparseDiscReaderPrivate {
	public:
		WuxReaderPrivate(WuxReader *q, IRpFile *file);

	private:
		typedef SparseDiscReaderPrivate super;
		RP_DISABLE_COPY(WuxReaderPrivate)

	public:
		// .wux header.
		wuxHeader_t wuxHeader;

		// Index table.
		// Starts immediately after wuxHeader.
		ao::uvector<uint32_t> idxTbl;

		// Data start position.
		// Starts immediately after the index table.
		int64_t dataOffset;
};

/** WuxReaderPrivate **/

WuxReaderPrivate::WuxReaderPrivate(WuxReader *q, IRpFile *file)
	: super(q, file)
	, dataOffset(0)
{
	// Clear the .wux header struct.
	memset(&wuxHeader, 0, sizeof(wuxHeader));

	if (!this->file) {
		// File could not be dup()'d.
		return;
	}

	// Read the .wux header.
	this->file->rewind();
	size_t sz = this->file->read(&wuxHeader, sizeof(wuxHeader));
	if (sz != sizeof(wuxHeader)) {
		// Error reading the .wux header.
		delete this->file;
		this->file = nullptr;
		q->m_lastError = EIO;
		return;
	}

	// Verify the .wux header.
	if (wuxHeader.magic[0] != cpu_to_le32(WUX_MAGIC_0) ||
	    wuxHeader.magic[1] != cpu_to_le32(WUX_MAGIC_1))
	{
		// Invalid magic.
		delete this->file;
		this->file = nullptr;
		q->m_lastError = EIO;
		return;
	}

	// Check if the block size is a supported power of two.
	// - Minimum: WUX_BLOCK_SIZE_MIN (256 bytes, 1 << 8)
	// - Maximum: WUX_BLOCK_SIZE_MAX (128 MB, 1 << 28)
	block_size = le32_to_cpu(wuxHeader.sectorSize);
	bool isPow2 = false;
	for (unsigned int i = 8; i <= 28; i++) {
		if (block_size == (1U << i)) {
			isPow2 = true;
			break;
		}
	}
	if (!isPow2) {
		// Block size is out of range.
		delete this->file;
		this->file = nullptr;
		q->m_lastError = EIO;
		return;
	}

	// Read the index table.
	disc_size = static_cast<int64_t>(le64_to_cpu(wuxHeader.uncompressedSize));
	if (disc_size < 0 || disc_size > 50LL*1024*1024*1024) {
		// Disc size is out of range.
		delete this->file;
		this->file = nullptr;
		q->m_lastError = EIO;
		return;
	}

	const uint32_t idxTbl_count = static_cast<uint32_t>(
		(disc_size + (block_size-1)) / block_size);
	const size_t idxTbl_size = idxTbl_count * sizeof(uint32_t);
	idxTbl.resize(idxTbl_count);
	sz = this->file->read(idxTbl.data(), idxTbl_size);
	if (sz != idxTbl_size) {
		// Read error.
		idxTbl.clear();
		disc_size = 0;
		delete this->file;
		this->file = nullptr;
		q->m_lastError = EIO;
		return;
	}

	// Data starts after the index table,
	// aligned to a block_size boundary.
	dataOffset = sizeof(wuxHeader) + (idxTbl_count * sizeof(uint32_t));
	dataOffset = ALIGN(block_size, dataOffset);

	// Reset the disc position.
	pos = 0;
}

/** WuxReader **/

WuxReader::WuxReader(IRpFile *file)
	: super(new WuxReaderPrivate(this, file))
{ }

/**
 * Is a disc image supported by this class?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
 */
int WuxReader::isDiscSupported_static(const uint8_t *pHeader, size_t szHeader)
{
	if (szHeader < sizeof(wuxHeader_t)) {
		// Not enough data to check.
		return -1;
	}

	// Check the .wux magic.
	const wuxHeader_t *const wuxHeader = reinterpret_cast<const wuxHeader_t*>(pHeader);
	if (wuxHeader->magic[0] != cpu_to_le32(WUX_MAGIC_0) ||
	    wuxHeader->magic[1] != cpu_to_le32(WUX_MAGIC_1))
	{
		// Invalid magic.
		return -1;
	}

	// Check if the block size is a supported power of two.
	// - Minimum: WUX_BLOCK_SIZE_MIN (256 bytes, 1 << 8)
	// - Maximum: WUX_BLOCK_SIZE_MAX (128 MB, 1 << 28)
	const unsigned int block_size = le32_to_cpu(wuxHeader->sectorSize);
	bool isPow2 = false;
	for (unsigned int i = 8; i <= 28; i++) {
		if (block_size == (1U << i)) {
			isPow2 = true;
			break;
		}
	}
	if (!isPow2) {
		// Block size is out of range.
		return -1;
	}

	// This is a valid .wux image.
	return 0;
}

/**
 * Is a disc image supported by this object?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WuxReader::isDiscSupported(const uint8_t *pHeader, size_t szHeader) const
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
int WuxReader::readBlock(uint32_t blockIdx, void *ptr, int pos, size_t size)
{
	// Read 'size' bytes of block 'blockIdx', starting at 'pos'.
	// NOTE: This can only be called by SparseDiscReader,
	// so the main assertions are already checked there.
	RP_D(WuxReader);
	assert(pos >= 0 && pos < (int)d->block_size);
	assert(size <= d->block_size);
	// TODO: Make sure overflow doesn't occur.
	assert((int64_t)(pos + size) <= (int64_t)d->block_size);
	if (pos < 0 || pos >= static_cast<int>(d->block_size) ||
		size > d->block_size ||
		static_cast<int64_t>(pos + size) > static_cast<int64_t>(d->block_size))
	{
		// pos+size is out of range.
		return -1;
	}

	if (unlikely(size == 0)) {
		// Nothing to read.
		return 0;
	}

	// Get the physical block number first.
	assert(blockIdx < d->idxTbl.size());
	if (blockIdx >= d->idxTbl.size()) {
		// Out of range.
		return -1;
	}

	// NOTE: .wux only supports deduplication.
	// There's no special indicator for a "zero" block.
	const uint32_t physBlockIdx = le32_to_cpu(d->idxTbl[blockIdx]);

	// Go to the block.
	// TODO: Make sure it isn't out of range?
	const int64_t phys_pos = d->dataOffset +
		(static_cast<int64_t>(physBlockIdx) * d->block_size) + pos;
	size_t sz_read = d->file->seekAndRead(phys_pos, ptr, size);
	m_lastError = d->file->lastError();
	return (sz_read > 0 ? (int)sz_read : -1);
}

}
