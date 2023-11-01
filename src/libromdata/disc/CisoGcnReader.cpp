/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CisoGcnReader.hpp: GameCube/Wii CISO disc image reader.                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CISOBlob.cpp
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CISOBlob.h

#include "stdafx.h"
#include "CisoGcnReader.hpp"
#include "librpbase/disc/SparseDiscReader_p.hpp"
#include "ciso_gcn.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;

// C++ STL classes
using std::array;

namespace LibRomData {

class CisoGcnReaderPrivate : public SparseDiscReaderPrivate
{
public:
	explicit CisoGcnReaderPrivate(CisoGcnReader *q);

private:
	typedef SparseDiscReaderPrivate super;
	RP_DISABLE_COPY(CisoGcnReaderPrivate)

public:
	// CISO header
	CISOHeader cisoHeader;

	// Block map
	// 0x0000 == first block after CISO header.
	// 0xFFFF == empty block.
	array<uint16_t, CISO_MAP_SIZE> blockMap;

	// Index of the last used block.
	int maxLogicalBlockUsed;
};

/** CisoGcnReaderPrivate **/

CisoGcnReaderPrivate::CisoGcnReaderPrivate(CisoGcnReader *q)
	: super(q)
	, maxLogicalBlockUsed(-1)
{
	// Clear the CISO header struct.
	memset(&cisoHeader, 0, sizeof(cisoHeader));
	// Clear the CISO block map initially.
	blockMap.fill(0xFFFF);
}

/** CisoGcnReader **/

CisoGcnReader::CisoGcnReader(const IRpFilePtr &file)
	: super(new CisoGcnReaderPrivate(this), file)
{
	if (!m_file) {
		// File could not be ref()'d.
		return;
	}

	// Read the CISO header.
	RP_D(CisoGcnReader);
	static_assert(sizeof(CISOHeader) == CISO_HEADER_SIZE,
		"CISOHeader is the wrong size. (Should be 32,768 bytes.)");
	m_file->rewind();
	size_t sz = m_file->read(&d->cisoHeader, sizeof(d->cisoHeader));
	if (sz != sizeof(d->cisoHeader)) {
		// Error reading the CISO header.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	// Verify the CISO header.
	if (d->cisoHeader.magic != cpu_to_be32(CISO_MAGIC)) {
		// Invalid magic.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	// Check if the block size is a supported power of two.
	// - Minimum: CISO_BLOCK_SIZE_MIN (32 KB, 1 << 15)
	// - Maximum: CISO_BLOCK_SIZE_MAX (16 MB, 1 << 24)
	d->block_size = le32_to_cpu(d->cisoHeader.block_size);
	if (!isPow2(d->block_size) ||
	    d->block_size < CISO_BLOCK_SIZE_MIN || d->block_size > CISO_BLOCK_SIZE_MAX)
	{
		// Block size is out of range.
		// If the block size is 0x18, then this is
		// actually a PSP CISO, and this field is
		// the CISO header size.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	// Parse the CISO block map.
	uint16_t physBlockIdx = 0;
	for (int i = 0; i < static_cast<int>(d->blockMap.size()); i++) {
		switch (d->cisoHeader.map[i]) {
			case 0:
				// Empty block.
				continue;
			case 1:
				// Used block.
				d->blockMap[i] = physBlockIdx;
				physBlockIdx++;
				d->maxLogicalBlockUsed = i;
				break;
			default:
				// Invalid entry.
				m_file.reset();
				m_lastError = EIO;
				return;
		}
	}

	// Calculate the disc size based on the highest logical block index.
	d->disc_size = (static_cast<off64_t>(d->maxLogicalBlockUsed) + 1) * static_cast<off64_t>(d->block_size);

	// Reset the disc position.
	d->pos = 0;
}

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
	const CISOHeader *const cisoHeader = reinterpret_cast<const CISOHeader*>(pHeader);
	if (cisoHeader->magic != cpu_to_be32(CISO_MAGIC)) {
		// Invalid magic.
		return -1;
	}

	// Check if the block size is a supported power of two.
	// - Minimum: CISO_BLOCK_SIZE_MIN (32 KB, 1 << 15)
	// - Maximum: CISO_BLOCK_SIZE_MAX (16 MB, 1 << 24)
	const uint32_t block_size = le32_to_cpu(cisoHeader->block_size);
	if (!isPow2(block_size) ||
	    block_size < CISO_BLOCK_SIZE_MIN || block_size > CISO_BLOCK_SIZE_MAX)
	{
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

/** SparseDiscReader functions **/

/**
 * Get the physical address of the specified logical block index.
 *
 * @param blockIdx	[in] Block index.
 * @return Physical address. (0 == empty block; -1 == invalid block index)
 */
off64_t CisoGcnReader::getPhysBlockAddr(uint32_t blockIdx) const
{
	// Make sure the block index is in range.
	// TODO: Check against maxLogicalBlockUsed?
	RP_D(const CisoGcnReader);
	assert(blockIdx < d->blockMap.size());
	if (blockIdx >= d->blockMap.size()) {
		// Out of range.
		return -1;
	}

	// Get the physical block index.
	const unsigned int physBlockIdx = d->blockMap[blockIdx];
	if (physBlockIdx >= 0xFFFF) {
		// Empty block.
		return 0;
	}

	// Convert to a physical block address and return.
	return static_cast<off64_t>(sizeof(d->cisoHeader)) +
	      (static_cast<off64_t>(physBlockIdx) * d->block_size);
}

}
