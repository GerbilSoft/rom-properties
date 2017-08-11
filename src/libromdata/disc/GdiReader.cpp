/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GdiReader.hpp: GD-ROM reader for Dreamcast GDI images.                  *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "GdiReader.hpp"
#include "librpbase/disc/SparseDiscReader_p.hpp"
#include "../cdrom_structs.h"

// librpbase
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
using namespace LibRpBase;

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>
#include <cctype>

// FIXME: Put this in a compatibility header.
#ifdef _MSC_VER
# define strtok_r(str, delim, saveptr) strtok_s(str, delim, saveptr)
#endif

// C++ includes.
#include <memory>
#include <vector>
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class GdiReaderPrivate : public SparseDiscReaderPrivate {
	public:
		GdiReaderPrivate(GdiReader *q, IRpFile *file);
		virtual ~GdiReaderPrivate();

	private:
		typedef SparseDiscReaderPrivate super;
		RP_DISABLE_COPY(GdiReaderPrivate)

	public:
		// Block range mapping.
		// NOTE: This currently *only* contains data tracks.
		struct BlockRange {
			unsigned int blockStart;	// First LBA.
			unsigned int blockEnd;		// Last LBA. (inclusive) (0 if the file hasn't been opened yet)
			uint16_t sectorSize;		// 2048 or 2352
			uint8_t trackNumber;		// 01 through 99
			uint8_t reserved;
			// TODO: Mode1/Mode2 designation?
			// TODO: Data vs. audio?
			rp_string filename;		// Relative to the .gdi file. Cleared on error.
			IRpFile *file;
		};
		vector<BlockRange> blockRanges;

		// Track to blockRanges mappings.
		// Index = track# (minus 1)
		// Value = pointer to BlockRange in blockRanges.
		vector<BlockRange*> trackMappings;

		/**
		 * Close all opened files.
		 */
		void close(void);

		/**
		 * Parse a GDI file.
		 * @param gdibuf NULL-terminated string containing the GDI file. (Must be writable!)
		 * NOTE: gdibuf is modified by strtok_r().
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int parseGdiFile(char *gdibuf);
};

/** GdiReaderPrivate **/

GdiReaderPrivate::GdiReaderPrivate(GdiReader *q, IRpFile *file)
	: super(q, file)
{
	if (!this->file) {
		// File could not be dup()'d.
		return;
	}

	// GDI file should be 4k or less.
	int64_t fileSize = file->size();
	if (fileSize <= 0 || fileSize > 4096) {
		// Invalid GDI file size.
		delete this->file;
		this->file = nullptr;
		q->m_lastError = EIO;
		return;
	}

	// Read the GDI and parse the track information.
	unsigned int gdisize = (unsigned int)fileSize;
	unique_ptr<char[]> gdibuf(new char[gdisize+1]);
	file->rewind();
	size_t size = file->read(gdibuf.get(), gdisize);
	if (size != gdisize) {
		// Read error.
		delete this->file;
		this->file = nullptr;
		q->m_lastError = EIO;
		return;
	}

	// Make sure the string is NULL-terminated.
	gdibuf[gdisize] = 0;

	// Parse the GDI file.
	int ret = parseGdiFile(gdibuf.get());
	if (ret != 0) {
		// Error parsing the GDI file.
		close();
		q->m_lastError = EIO;
		return;
	}
}

GdiReaderPrivate::~GdiReaderPrivate()
{
	close();
}

/**
 * Close all opened files.
 */
void GdiReaderPrivate::close(void)
{
	for (auto iter = blockRanges.begin(); iter != blockRanges.end(); ++iter) {
		delete iter->file;
	}
	blockRanges.clear();
	trackMappings.clear();

	// GDI file.
	delete this->file;
	this->file = nullptr;
}

/**
 * Parse a GDI file.
 * @param gdibuf NULL-terminated string containing the GDI file. (Must be writable!)
 * NOTE: gdibuf is modified by strtok_r().
 * @return 0 on success; negative POSIX error code on error.
 */
int GdiReaderPrivate::parseGdiFile(char *gdibuf)
{
	assert(blockRanges.empty());
	assert(trackMappings.empty());
	if (!blockRanges.empty() || !trackMappings.empty()) {
		// GDI is already loaded...
		return -EEXIST;
	}

	// First line should contain the number of tracks.
	char *linesaveptr;
	const char *linetoken = strtok_r(gdibuf, "\n", &linesaveptr);
	if (!linetoken || linetoken[0] == 0) {
		return -EIO;
	}

	char *endptr;
	int trackCount = strtol(linetoken, &endptr, 10);
	if (trackCount <= 0 || trackCount > 99 || (*endptr != 0 && *endptr != '\r')) {
		// Track count is invalid.
		return -EIO;
	}

	blockRanges.reserve((size_t)trackCount);
	trackMappings.resize((size_t)trackCount);

	// Remainder of file is the track list.
	// Format: Track# LBA Type SectorSize Filename ???
	// - Track#: Track number.
	// - LBA: Starting LBA. (not counting 2-second lead-in, so track 01 is LBA 0)
	// - Type: Track type. (0 == audio, 4 == data)
	// - SectorSize: Sector size. (usually 2048 or 2352)
	// - Filename: Relative filename, e.g. "track01.bin" or "track02.raw".
	// - ???: Unknown.
	while ((linetoken = strtok_r(nullptr, "\n", &linesaveptr)) != nullptr) {
		if (linetoken[0] == '\r' || linetoken[0] == 0) {
			// Empty line.
			continue;
		}

		int trackNumber, blockStart, type, sectorSize;
		int reserved;
		char filename[64];	// Filenames shouldn't be that long...
		int count = sscanf(linetoken, "%d %d %d %d %64s %d",
			&trackNumber, &blockStart, &type, &sectorSize, filename, &reserved);
		if (count != 6) {
			// Invalid line.
			return -EIO;
		}

		// Verify fields.
		// 2097152 blocks == 4 GB if using 2048-byte sectors.
		if (blockStart < 0 || blockStart > 2097152 ||
		    (sectorSize != 2048 && sectorSize != 2352) ||
		    filename[0] == 0 || reserved != 0)
		{
			// Invalid fields.
			return -EIO;
		}

		// Check the track type.
		if (type == 0) {
			// Audio track.
			// It's valid, but we're ignoring it.
			continue;
		} else if (type != 4) {
			// Not a data track.
			// Disc image isn't supported.
			return -EIO;
		}

		// Validate the track number:
		// - Should be between 1 and trackCount.
		// - Should not be duplicated.
		if (trackNumber <= 0 || trackNumber > trackCount) {
			// Out of range.
			return -EIO;
		} else if (trackMappings[trackNumber-1] != nullptr) {
			// Duplicate.
			return -EIO;
		}

		// Save the track information.
		int idx = (int)blockRanges.size();
		blockRanges.resize(idx+1);
		BlockRange &blockRange = blockRanges[idx];
		blockRange.blockStart = (unsigned int)blockStart;
		blockRange.blockEnd = 0;	// will be filled in when loaded
		blockRange.sectorSize = (uint16_t)sectorSize;
		blockRange.trackNumber = (uint8_t)trackNumber;
		blockRange.reserved = 0;
		// FIXME: UTF-8 or Latin-1?
		filename[sizeof(filename)-1] = 0;
		blockRange.filename = latin1_to_rp_string(filename, -1);
		blockRange.file = nullptr;

		// Save the track mapping.
		trackMappings[trackNumber-1] = &blockRange;
	}

	// Done parsing the GDI.
	return 0;
}

/** GdiReader **/

GdiReader::GdiReader(IRpFile *file)
	: super(new GdiReaderPrivate(this, file))
{ }

/**
 * Is a disc image supported by this class?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
 */
int GdiReader::isDiscSupported_static(const uint8_t *pHeader, size_t szHeader)
{
	// NOTE: There's no magic number, so we'll just check if the
	// first line seems to be correct.
	if (szHeader < 4) {
		// Not enough data to check.
		return -1;
	}

	int trackCount = 0;
	for (int i = 0; i < 4; i++) {
		if (pHeader[i] == '\r' || pHeader[i] == '\n') {
			// End of line.
			break;
		} else if (isdigit(pHeader[i])) {
			// Digit.
			trackCount *= 10;
			trackCount += (pHeader[i] & 0xF);
		} else {
			// Invalid character.
			trackCount = 0;
			break;
		}
	}

	if (trackCount >= 1 && trackCount <= 99) {
		// Valid track count.
		return 0;
	}

	// Invalid track count.
	return -1;
}

/**
 * Is a disc image supported by this object?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GdiReader::isDiscSupported(const uint8_t *pHeader, size_t szHeader) const
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
int GdiReader::readBlock(uint32_t blockIdx, void *ptr, int pos, size_t size)
{
	// FIXME: Update for blockRanges.
	return 0;
#if 0
	// Read 'size' bytes of block 'blockIdx', starting at 'pos'.
	// NOTE: This can only be called by SparseDiscReader,
	// so the main assertions are already checked there.
	RP_D(GdiReader);
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
#endif
}

}
