/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NASOSReader.hpp: GameCube/Wii NASOS (.iso.dec) disc image reader.       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CISOBlob.cpp
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CISOBlob.h

#include "stdafx.h"
#include "NASOSReader.hpp"
#include "librpbase/disc/SparseDiscReader_p.hpp"
#include "nasos_gcn.h"

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;

// C++ STL classes
using std::shared_ptr;

namespace LibRomData {

class NASOSReaderPrivate : public SparseDiscReaderPrivate {
	public:
		explicit NASOSReaderPrivate(NASOSReader *q);

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

		enum class DiscType {
			Unknown	= -1,

			GCML	= 0,
			WIIx	= 1,

			Max
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
	, discType(DiscType::Unknown)
	, blockMapShift(0)
{
	// Clear the NASOSHeader structs.
	memset(&header, 0, sizeof(header));
}

/** NASOSReader **/

NASOSReader::NASOSReader(const shared_ptr<IRpFile> &file)
	: super(new NASOSReaderPrivate(this), file)
{
	if (!m_file) {
		// File could not be ref()'d.
		return;
	}

	// Read the NASOS header.
	RP_D(NASOSReader);
	m_file->rewind();
	size_t sz = m_file->read(&d->header, sizeof(d->header));
	if (sz != sizeof(d->header)) {
		// Error reading the NASOS header.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	// Verify the NASOS header.
	// TODO: WII9 magic?
	// TODO: Check the actual disc header magic?
	unsigned int blockMapStart, blockCount;
	if (d->header.nasos.magic == cpu_to_be32(NASOS_MAGIC_GCML)) {
		d->discType = NASOSReaderPrivate::DiscType::GCML;
		d->block_size = 2048;			// NOTE: Not stored in the header.
		blockMapStart = sizeof(d->header.gcml);
		blockCount = NASOS_GCML_BlockCount;	// NOTE: Not stored in the header.
		d->blockMapShift = 0;
	} else if ((d->header.nasos.magic == cpu_to_be32(NASOS_MAGIC_WII5)) ||
		   (d->header.nasos.magic == cpu_to_be32(NASOS_MAGIC_WII9)))
	{
		d->discType = NASOSReaderPrivate::DiscType::WIIx;
		d->block_size = 1024;	// TODO: Is this stored in the header?
		blockMapStart = sizeof(d->header.wiix);
		// TODO: Verify against WII5 (0x460900) and WII9 (0x7ED380).
		blockCount = le32_to_cpu(d->header.wiix.block_count) >> 8;
		d->blockMapShift = 8;
	} else {
		// Invalid magic.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	// TODO: Other checks.

	// Read the block map.
	// TODO: Restrict the maximum block count?
	d->blockMap.resize(blockCount);
	const size_t sz_blockMap = d->blockMap.size() * sizeof(uint32_t);
	sz = m_file->seekAndRead(blockMapStart, d->blockMap.data(), sz_blockMap);
	if (sz != sz_blockMap) {
		// Error reading the block map.
		d->blockMap.clear();
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	// Disc size is based on the block map size.
	d->disc_size = static_cast<off64_t>(d->blockMap.size()) * d->block_size;

	// Reset the disc position.
	d->pos = 0;
}

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
off64_t NASOSReader::getPhysBlockAddr(uint32_t blockIdx) const
{
	// Make sure the block index is in range.
	RP_D(const NASOSReader);
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
	return static_cast<off64_t>(physBlockAddr) << d->blockMapShift;
}

}
