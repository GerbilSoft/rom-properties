/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CisoGcnReader.hpp: GameCube/Wii CISO disc image reader.                 *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
#include "ciso_gcn.h"

#include "byteswap.h"
#include "file/IRpFile.hpp"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

namespace LibRomData {

class CisoGcnReaderPrivate {
	public:
		CisoGcnReaderPrivate(IRpFile *file);
		~CisoGcnReaderPrivate();

	private:
		CisoGcnReaderPrivate(const CisoGcnReaderPrivate &other);
		CisoGcnReaderPrivate &operator=(const CisoGcnReaderPrivate &other);

	public:
		IRpFile *file;
		int lastError;

		int64_t disc_size;	// Virtual disc image size.
		int64_t pos;		// Read position.

		// CISO magic.
		static const char CISO_MAGIC[4];

		// CISO header.
		CISOHeader cisoHeader;
		uint32_t block_size;

		// Block map.
		// 0x0000 == first block after CISO header.
		// 0xFFFF == empty block.
		uint16_t blockMap[CISO_MAP_SIZE];
};

// CISO magic.
const char CisoGcnReaderPrivate::CISO_MAGIC[4] = {'C','I','S','O'};

/** CisoGcnReaderPrivate **/

CisoGcnReaderPrivate::CisoGcnReaderPrivate(IRpFile *file)
	: file(nullptr)
	, lastError(0)
	, disc_size(0)
	, block_size(0)
{
	if (!file) {
		lastError = EBADF;
		return;
	}
	this->file = file->dup();

	// Read the CISO header.
	static_assert(sizeof(CISOHeader) == CISO_HEADER_SIZE,
		"CISOHeader is the wrong size. (Should be 32,768 bytes.)");
	this->file->rewind();
	size_t sz = this->file->read(&cisoHeader, sizeof(cisoHeader));
	if (sz != sizeof(cisoHeader)) {
		// Error reading the CISO header.
		delete this->file;
		this->file = nullptr;
		lastError = EIO;
		return;
	}

	// Verify the CISO header.
	if (memcmp(cisoHeader.magic, CISO_MAGIC, sizeof(CISO_MAGIC)) != 0) {
		// Invalid magic.
		delete this->file;
		this->file = nullptr;
		lastError = EIO;
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
		lastError = EIO;
		return;
	}

	// Clear the CISO block map initially.
	memset(&blockMap, 0xFF, sizeof(blockMap));

	// Parse the CISO block map.
	uint16_t physBlockIdx = 0;
	int maxLogicalBlockUsed = -1;
	for (int i = 0; i < ARRAY_SIZE(cisoHeader.map); i++) {
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
				lastError = EIO;
				return;
		}
	}

	// Calculate the disc size based on the highest logical block index.
	disc_size = (int64_t)(maxLogicalBlockUsed+1) * (int64_t)block_size;
}

CisoGcnReaderPrivate::~CisoGcnReaderPrivate()
{
	delete file;
}

/** CisoGcn **/

CisoGcnReader::CisoGcnReader(IRpFile *file)
	: d(new CisoGcnReaderPrivate(file))
{ }

CisoGcnReader::~CisoGcnReader()
{
	delete d;
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
	if (memcmp(pHeader, CisoGcnReaderPrivate::CISO_MAGIC, sizeof(CisoGcnReaderPrivate::CISO_MAGIC)) != 0) {
		// Invalid magic.
		return -1;
	}

	// Check if the block size is a supported power of two.
	// - Minimum: CISO_BLOCK_SIZE_MIN (32 KB, 1 << 15)
	// - Maximum: CISO_BLOCK_SIZE_MAX (16 MB, 1 << 24)
	const uint32_t *pHeader32 = reinterpret_cast<const uint32_t*>(pHeader);
	const uint32_t block_size = le32_to_cpu(pHeader32[1]);
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

/**
 * Is the disc image open?
 * This usually only returns false if an error occurred.
 * @return True if the disc image is open; false if it isn't.
 */
bool CisoGcnReader::isOpen(void) const
{
	return (d->file != nullptr);
}

/**
 * Get the last error.
 * @return Last POSIX error, or 0 if no error.
 */
int CisoGcnReader::lastError(void) const
{
	return d->lastError;
}

/**
 * Clear the last error.
 */
void CisoGcnReader::clearError(void)
{
	d->lastError = 0;
}

/**
 * Read data from the disc image.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t CisoGcnReader::read(void *ptr, size_t size)
{
	assert(d->file != nullptr);

	uint8_t *ptr8 = reinterpret_cast<uint8_t*>(ptr);
	size_t ret = 0;

	// Are we already at the end of the disc?
	if (d->pos >= d->disc_size)
		return 0;

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
		if (size < read_sz)
			read_sz = size;

		// Get the physical block number first.
		uint16_t blockStart = (d->pos / block_size);
		assert(blockStart < ARRAY_SIZE(d->blockMap));
		if (blockStart >= ARRAY_SIZE(d->blockMap)) {
			// Out of range.
			return 0;
		}

		uint16_t physBlockStartIdx = d->blockMap[blockStart];
		if (physBlockStartIdx == 0xFFFF) {
			// Empty block.
			memset(ptr8, 0, read_sz);
		} else {
			// Seek to the physical block address.
			int64_t physBlockStartAddr = sizeof(d->cisoHeader) +
				((int64_t)physBlockStartIdx * block_size);
			d->file->seek(physBlockStartAddr + blockStartOffset);
			// Read read_sz bytes.
			size_t rd = d->file->read(ptr8, read_sz);
			if (rd != read_sz) {
				// Error reading the data.
				return rd;
			}
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
		uint16_t blockIdx = (d->pos / block_size);
		assert(blockIdx < ARRAY_SIZE(d->blockMap));
		if (blockIdx >= ARRAY_SIZE(d->blockMap)) {
			// Out of range.
			return ret;
		}

		uint16_t physBlockIdx = d->blockMap[blockIdx];
		if (physBlockIdx == 0xFFFF) {
			// Empty block.
			memset(ptr8, 0, block_size);
		} else {
			// Seek to the physical block address.
			int64_t physBlockAddr = sizeof(d->cisoHeader) +
				((int64_t)physBlockIdx * block_size);
			d->file->seek(physBlockAddr);
			// Read one block worth of data.
			size_t rd = d->file->read(ptr8, block_size);
			if (rd != block_size) {
				// Error reading the data.
				return rd + ret;
			}
		}
	}

	// Check if we still have data left. (not a full block)
	if (size > 0) {
		// Not a full block.
		assert(d->pos % block_size == 0);

		// Get the physical block number first.
		uint16_t blockEnd = (d->pos / block_size);
		assert(blockEnd < ARRAY_SIZE(d->blockMap));
		if (blockEnd >= ARRAY_SIZE(d->blockMap)) {
			// Out of range.
			return ret;
		}

		uint16_t physBlockEndIdx = d->blockMap[blockEnd];
		if (physBlockEndIdx == 0xFFFF) {
			// Empty block.
			memset(ptr8, 0, size);
		} else {
			// Seek to the physical block address.
			int64_t physBlockEndAddr = sizeof(d->cisoHeader) +
				((int64_t)physBlockEndIdx * block_size);
			d->file->seek(physBlockEndAddr);
			// Read size bytes.
			size_t rd = d->file->read(ptr8, size);
			if (rd != size) {
				// Error reading the data.
				return rd + ret;
			}
		}

		ret += size;
		d->pos += size;
	}

	// Finished reading the data.
	return ret;
}

/**
 * Set the disc image position.
 * @param pos Disc image position.
 * @return 0 on success; -1 on error.
 */
int CisoGcnReader::seek(int64_t pos)
{
	assert(d->file != nullptr);
	if (!d->file) {
		d->lastError = EBADF;
		return -1;
	}

	// Handle out-of-range cases.
	// TODO: How does POSIX behave?
	if (pos < 0)
		d->pos = 0;
	else if (pos >= d->disc_size)
		d->pos = d->disc_size;
	else
		d->pos = pos;
	return 0;
}

/**
 * Seek to the beginning of the disc image.
 */
void CisoGcnReader::rewind(void)
{
	assert(d->file != nullptr);
	if (!d->file) {
		d->lastError = EBADF;
		return;
	}

	d->pos = 0;
}

/**
 * Get the disc image size.
 * @return Disc image size, or -1 on error.
 */
int64_t CisoGcnReader::size(void) const
{
	assert(d->file != nullptr);
	if (!d->file) {
		d->lastError = EBADF;
		return -1;
	}

	return d->disc_size;
}

}
