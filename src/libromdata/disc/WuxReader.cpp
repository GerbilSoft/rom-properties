/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WuxReader.cpp: Wii U .wux disc image reader.                            *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://gbatemp.net/threads/wii-u-image-wud-compression-tool.397901/

#include "stdafx.h"
#include "WuxReader.hpp"
#include "librpbase/disc/SparseDiscReader_p.hpp"
#include "wux_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;

namespace LibRomData {

class WuxReaderPrivate : public SparseDiscReaderPrivate {
	public:
		explicit WuxReaderPrivate(WuxReader *q);

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
		off64_t dataOffset;
};

/** WuxReaderPrivate **/

WuxReaderPrivate::WuxReaderPrivate(WuxReader *q)
	: super(q)
	, dataOffset(0)
{
	// Clear the .wux header struct.
	memset(&wuxHeader, 0, sizeof(wuxHeader));
}

/** WuxReader **/

WuxReader::WuxReader(const IRpFilePtr &file)
	: super(new WuxReaderPrivate(this), file)
{
	if (!m_file) {
		// File could not be ref()'d.
		return;
	}

	// Read the .wux header.
	RP_D(WuxReader);
	m_file->rewind();
	size_t sz = m_file->read(&d->wuxHeader, sizeof(d->wuxHeader));
	if (sz != sizeof(d->wuxHeader)) {
		// Error reading the .wux header.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	// Verify the .wux header.
	if (d->wuxHeader.magic[0] != cpu_to_be32(WUX_MAGIC_0) ||
	    d->wuxHeader.magic[1] != cpu_to_be32(WUX_MAGIC_1))
	{
		// Invalid magic.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	// Check if the block size is a supported power of two.
	// - Minimum: WUX_BLOCK_SIZE_MIN (256 bytes, 1 << 8)
	// - Maximum: WUX_BLOCK_SIZE_MAX (128 MB, 1 << 28)
	d->block_size = le32_to_cpu(d->wuxHeader.sectorSize);
	if (!isPow2(d->block_size) ||
	    d->block_size < WUX_BLOCK_SIZE_MIN || d->block_size > WUX_BLOCK_SIZE_MAX)
	{
		// Block size is out of range.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	// Read the index table.
	d->disc_size = static_cast<off64_t>(le64_to_cpu(d->wuxHeader.uncompressedSize));
	if (d->disc_size < 0 || d->disc_size > 50LL*1024*1024*1024) {
		// Disc size is out of range.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	const uint32_t idxTbl_count = static_cast<uint32_t>(
		(d->disc_size + (d->block_size-1)) / d->block_size);
	const size_t idxTbl_size = idxTbl_count * sizeof(uint32_t);
	d->idxTbl.resize(idxTbl_count);
	sz = m_file->read(d->idxTbl.data(), idxTbl_size);
	if (sz != idxTbl_size) {
		// Read error.
		d->idxTbl.clear();
		d->disc_size = 0;
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	// Data starts after the index table,
	// aligned to a block_size boundary.
	d->dataOffset = sizeof(d->wuxHeader) + (idxTbl_count * sizeof(uint32_t));
	d->dataOffset = ALIGN_BYTES(d->block_size, d->dataOffset);

	// Reset the disc position.
	d->pos = 0;
}

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
	if (wuxHeader->magic[0] != cpu_to_be32(WUX_MAGIC_0) ||
	    wuxHeader->magic[1] != cpu_to_be32(WUX_MAGIC_1))
	{
		// Invalid magic.
		return -1;
	}

	// Check if the block size is a supported power of two.
	// - Minimum: WUX_BLOCK_SIZE_MIN (256 bytes, 1 << 8)
	// - Maximum: WUX_BLOCK_SIZE_MAX (128 MB, 1 << 28)
	const unsigned int block_size = le32_to_cpu(wuxHeader->sectorSize);
	if (!isPow2(block_size) ||
	    block_size < WUX_BLOCK_SIZE_MIN || block_size > WUX_BLOCK_SIZE_MAX)
	{
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
 * Get the physical address of the specified logical block index.
 *
 * @param blockIdx	[in] Block index.
 * @return Physical address. (0 == empty block; -1 == invalid block index)
 */
off64_t WuxReader::getPhysBlockAddr(uint32_t blockIdx) const
{
	// Make sure the block index is in range.
	RP_D(const WuxReader);
	assert(blockIdx < d->idxTbl.size());
	if (blockIdx >= d->idxTbl.size()) {
		// Out of range.
		return -1;
	}

	// Get the physical block index.
	// NOTE: .wux only supports deduplication.
	// There's no special indicator for a "zero" block.
	const uint32_t physBlockIdx = le32_to_cpu(d->idxTbl[blockIdx]);

	// Convert to a physical block address and return.
	return d->dataOffset + (static_cast<off64_t>(physBlockIdx) * d->block_size);
}

}
