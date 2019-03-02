/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NASOSReader.hpp: GameCube/Wii NASOS (.iso.dec) disc image reader.       *
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

// References:
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CISOBlob.cpp
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CISOBlob.h

#include "NASOSReader.hpp"
#include "librpbase/disc/SparseDiscReader_p.hpp"
#include "nasos_gcn.h"

// librpbase
#include "librpbase/byteswap.h"
#include "librpbase/file/IRpFile.hpp"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData {

class NASOSReaderPrivate : public SparseDiscReaderPrivate {
	public:
		NASOSReaderPrivate(NASOSReader *q);

	private:
		typedef SparseDiscReaderPrivate super;
		RP_DISABLE_COPY(NASOSReaderPrivate)

	public:
		// NASOS header.
		union {
			NASOSHeader nasos;
			NASOSHeader_GCML gcml;
			NASOSHeader_WIIx wiix;
		} header;

		enum DiscType {
			DT_UNKNOWN = 0,
			DT_GCML = 1,
			DT_WIIx = 2,
		};
		DiscType discType;

		// Block map.
		// Values are absolute block addresses, possibly with a shift amount.
		// Special value: 0xFFFFFFFF == empty block
		ao::uvector<uint32_t> blockMap;

		// Block address shift.
		// - GCML: 0
		// - WIIx: 8
		uint8_t blockMapShift;
};

/** NASOSReaderPrivate **/

NASOSReaderPrivate::NASOSReaderPrivate(NASOSReader *q)
	: super(q)
	, discType(DT_UNKNOWN)
	, blockMapShift(0)
{
	// Clear the NASOSHeader structs.
	memset(&header, 0, sizeof(header));

	if (!q->m_file) {
		// File could not be ref()'d.
		return;
	}

	// Read the NASOS header.
	q->m_file->rewind();
	size_t sz = q->m_file->read(&header, sizeof(header));
	if (sz != sizeof(header)) {
		// Error reading the NASOS header.
		q->m_file->unref();
		q->m_file = nullptr;
		q->m_lastError = EIO;
		return;
	}

	// Verify the NASOS header.
	// TODO: WII9 magic?
	// TODO: Check the actual disc header magic?
	unsigned int blockMapStart, blockCount;
	if (header.nasos.magic == cpu_to_be32(NASOS_MAGIC_GCML)) {
		discType = DT_GCML;
		block_size = 2048;			// NOTE: Not stored in the header.
		blockMapStart = sizeof(header.gcml);
		blockCount = NASOS_GCML_BlockCount;	// NOTE: Not stored in the header.
		blockMapShift = 0;
	} else if ((header.nasos.magic == cpu_to_be32(NASOS_MAGIC_WII5)) ||
		   (header.nasos.magic == cpu_to_be32(NASOS_MAGIC_WII9)))
	{
		discType = DT_WIIx;
		block_size = 1024;	// TODO: Is this stored in the header?
		blockMapStart = sizeof(header.wiix);
		// TODO: Verify against WII5 (0x460900) and WII9 (0x7ED380).
		blockCount = le32_to_cpu(header.wiix.block_count) >> 8;
		blockMapShift = 8;
	} else {
		// Invalid magic.
		q->m_file->unref();
		q->m_file = nullptr;
		q->m_lastError = EIO;
		return;
	}

	// TODO: Other checks.

	// Read the block map.
	// TODO: Restrict the maximum block count?
	blockMap.resize(blockCount);
	const size_t sz_blockMap = blockMap.size() * sizeof(uint32_t);
	sz = q->m_file->seekAndRead(blockMapStart, blockMap.data(), sz_blockMap);
	if (sz != sz_blockMap) {
		// Error reading the block map.
		blockMap.clear();
		q->m_file->unref();
		q->m_file = nullptr;
		q->m_lastError = EIO;
		return;
	}

	// Disc size is based on the block map size.
	disc_size = static_cast<int64_t>(blockMap.size()) * block_size;

	// Reset the disc position.
	pos = 0;
}

/** NASOSReader **/

NASOSReader::NASOSReader(IRpFile *file)
	: super(new NASOSReaderPrivate(this), file)
{ }

/**
 * Is a disc image supported by this class?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
 */
int NASOSReader::isDiscSupported_static(const uint8_t *pHeader, size_t szHeader)
{
	if (szHeader < sizeof(NASOSHeader)) {
		// Not enough data to check.
		return -1;
	}

	// Check the NASOS magic.
	// TODO: WII9 magic?
	const NASOSHeader *const nasosHeader =
		reinterpret_cast<const NASOSHeader*>(pHeader);
	if (nasosHeader->magic != cpu_to_be32(NASOS_MAGIC_GCML) &&
	    nasosHeader->magic != cpu_to_be32(NASOS_MAGIC_WII5))
	{
		// Invalid magic.
		return -1;
	}

	// TODO: Other checks.

	// This is a valid NASOS image.
	return 0;
}

/**
 * Is a disc image supported by this object?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int NASOSReader::isDiscSupported(const uint8_t *pHeader, size_t szHeader) const
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
int64_t NASOSReader::getPhysBlockAddr(uint32_t blockIdx) const
{
	// Make sure the block index is in range.
	RP_D(NASOSReader);
	assert(blockIdx < d->blockMap.size());
	if (blockIdx >= d->blockMap.size()) {
		// Out of range.
		return -1;
	}

	// Get the physical block address.
	const uint32_t physBlockAddr = d->blockMap[blockIdx];
	if (physBlockAddr == 0xFFFFFFFF) {
		// Empty block.
		return 0;
	}

	// Adjust to bytes and return.
	return static_cast<int64_t>(physBlockAddr) << d->blockMapShift;
}

}
