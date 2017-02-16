/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CisoGcnReader.hpp: GameCube/Wii CISO disc image reader.                 *
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

// References:
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CISOBlob.cpp
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CISOBlob.h

#include "CisoGcnReader.hpp"
#include "SparseDiscReader_p.hpp"
#include "ciso_gcn.h"

#include "byteswap.h"
#include "file/IRpFile.hpp"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <array>
using std::array;

namespace LibRomData {

class CisoGcnReaderPrivate : public SparseDiscReaderPrivate {
	public:
		CisoGcnReaderPrivate(CisoGcnReader *q, IRpFile *file);

	private:
		typedef SparseDiscReaderPrivate super;
		RP_DISABLE_COPY(CisoGcnReaderPrivate)

	public:
		// CISO magic.
		static const char CISO_MAGIC[4];

		// CISO header.
		CISOHeader cisoHeader;

		// Block map.
		// 0x0000 == first block after CISO header.
		// 0xFFFF == empty block.
		std::array<uint16_t, CISO_MAP_SIZE> blockMap;

		// Index of the last used block.
		int maxLogicalBlockUsed;
};

/** CisoGcnReaderPrivate **/

// CISO magic.
const char CisoGcnReaderPrivate::CISO_MAGIC[4] = {'C','I','S','O'};

CisoGcnReaderPrivate::CisoGcnReaderPrivate(CisoGcnReader *q, IRpFile *file)
	: super(q, file)
	, maxLogicalBlockUsed(-1)
{
	// Clear the CISO header struct.
	memset(&cisoHeader, 0, sizeof(cisoHeader));

	if (!this->file) {
		// File could not be dup()'d.
		return;
	}

	// Read the CISO header.
	static_assert(sizeof(CISOHeader) == CISO_HEADER_SIZE,
		"CISOHeader is the wrong size. (Should be 32,768 bytes.)");
	this->file->rewind();
	size_t sz = this->file->read(&cisoHeader, sizeof(cisoHeader));
	if (sz != sizeof(cisoHeader)) {
		// Error reading the CISO header.
		delete this->file;
		this->file = nullptr;
		q->m_lastError = EIO;
		return;
	}

	// Verify the CISO header.
	if (memcmp(cisoHeader.magic, CISO_MAGIC, sizeof(CISO_MAGIC)) != 0) {
		// Invalid magic.
		delete this->file;
		this->file = nullptr;
		q->m_lastError = EIO;
		return;
	}

	// Check if the block size is a supported power of two.
	// - Minimum: CISO_BLOCK_SIZE_MIN (32 KB, 1 << 15)
	// - Maximum: CISO_BLOCK_SIZE_MAX (16 MB, 1 << 24)
	block_size = le32_to_cpu(cisoHeader.block_size);
	bool isPow2 = false;
	for (unsigned int i = 15; i <= 24; i++) {
		if (block_size == (1U << i)) {
			isPow2 = true;
			break;
		}
	}
	if (!isPow2) {
		// Block size is out of range.
		// If the block size is 0x18, then this is
		// actually a PSP CISO, and this field is
		// the CISO header size.
		delete this->file;
		this->file = nullptr;
		q->m_lastError = EIO;
		return;
	}

	// Clear the CISO block map initially.
	blockMap.fill(0xFFFF);

	// Parse the CISO block map.
	uint16_t physBlockIdx = 0;
	for (int i = 0; i < (int)blockMap.size(); i++) {
		switch (cisoHeader.map[i]) {
			case 0:
				// Empty block.
				continue;
			case 1:
				// Used block.
				blockMap[i] = physBlockIdx;
				physBlockIdx++;
				maxLogicalBlockUsed = i;
				break;
			default:
				// Invalid entry.
				delete this->file;
				this->file = nullptr;
				q->m_lastError = EIO;
				return;
		}
	}

	// Calculate the disc size based on the highest logical block index.
	disc_size = (int64_t)(maxLogicalBlockUsed+1) * (int64_t)block_size;

	// Reset the disc position.
	pos = 0;
}

/** CisoGcnReader **/

CisoGcnReader::CisoGcnReader(IRpFile *file)
	: super(new CisoGcnReaderPrivate(this, file))
{ }

/**
 * Is a disc image supported by this class?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
 */
int CisoGcnReader::isDiscSupported_static(const uint8_t *pHeader, size_t szHeader)
{
	if (szHeader < 8) {
		// Not enough data to check.
		return -1;
	}

	// Check the CISO magic.
	if (memcmp(pHeader, CisoGcnReaderPrivate::CISO_MAGIC, sizeof(CisoGcnReaderPrivate::CISO_MAGIC)) != 0) {
		// Invalid magic.
		return -1;
	}

	// Check if the block size is a supported power of two.
	// - Minimum: CISO_BLOCK_SIZE_MIN (32 KB, 1 << 15)
	// - Maximum: CISO_BLOCK_SIZE_MAX (16 MB, 1 << 24)
	const uint32_t *pHeader32 = reinterpret_cast<const uint32_t*>(pHeader);
	const unsigned int block_size = le32_to_cpu(pHeader32[1]);
	bool isPow2 = false;
	for (unsigned int i = 15; i <= 24; i++) {
		if (block_size == (1U << i)) {
			isPow2 = true;
			break;
		}
	}
	if (!isPow2) {
		// Block size is out of range.
		// If the block size is 0x18, then this is
		// actually a PSP CISO, and this field is
		// the CISO header size.
		return -1;
	}

	// This is a valid CISO image.
	return 0;
}

/**
 * Is a disc image supported by this object?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int CisoGcnReader::isDiscSupported(const uint8_t *pHeader, size_t szHeader) const
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
int CisoGcnReader::readBlock(uint32_t blockIdx, void *ptr, int pos, size_t size)
{
	// Read 'size' bytes of block 'blockIdx', starting at 'pos'.
	// NOTE: This can only be called by SparseDiscReader,
	// so the main assertions are already checked there.
	RP_D(CisoGcnReader);
	assert(pos >= 0 && pos < (int)d->block_size);
	assert(size <= d->block_size);
	// TODO: Make sure overflow doesn't occur.
	assert((int64_t)(pos + size) < (int64_t)d->block_size);
	if (pos < 0 || pos >= (int)d->block_size || size > d->block_size ||
	    (int64_t)(pos + size) >= (int64_t)d->block_size)
	{
		// pos+size is out of range.
		return -1;
	}

	// Get the physical block number first.
	// TODO: Check against maxLogicalBlockUsed?
	assert(blockIdx < d->blockMap.size());
	if (blockIdx >= d->blockMap.size()) {
		// Out of range.
		return -1;
	}

	const unsigned int physBlockIdx = d->blockMap[blockIdx];
	if (physBlockIdx >= 0xFFFF) {
		// Empty block.
		memset(ptr, 0, size);
		return (int)size;
	}

	// Go to the block.
	const int64_t phys_pos = sizeof(d->cisoHeader) + ((int64_t)physBlockIdx * d->block_size) + pos;
	int ret = d->file->seek(phys_pos);
	if (ret != 0) {
		// Seek failed.
		m_lastError = d->file->lastError();
		return -1;
	}

	// Read the first 'size' bytes of the block.
	ret = (int)d->file->read(ptr, size);
	m_lastError = d->file->lastError();
	return ret;
}

}
