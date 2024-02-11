/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GdiReader.hpp: GD-ROM reader for Dreamcast GDI images.                  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GdiReader.hpp"
#include "librpbase/disc/SparseDiscReader_p.hpp"

#include "../cdrom_structs.h"
#include "IsoPartition.hpp"

// Other rom-properties libraries
#include "librpfile/RelatedFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// Other RomData subclasses
#include "Other/ISO.hpp"

// C++ STL classes
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class GdiReaderPrivate final : public SparseDiscReaderPrivate
{
public:
	explicit GdiReaderPrivate(GdiReader *q);
	~GdiReaderPrivate() final;

private:
	typedef SparseDiscReaderPrivate super;
	RP_DISABLE_COPY(GdiReaderPrivate)

public:
	// GDI filename
	string filename;

	// Block range mapping
	// NOTE: This currently *only* contains data tracks.
	struct BlockRange {
		unsigned int blockStart;	// First LBA
		unsigned int blockEnd;		// Last LBA (inclusive) (0 if the file hasn't been opened yet)
		uint16_t sectorSize;		// 2048 or 2352 (TODO: Make this a bool?)
		uint8_t trackNumber;		// 01 through 99
		uint8_t reserved;
		// TODO: Data vs. audio?
		string filename;		// Relative to the .gdi file. Cleared on error.
		IRpFile *file;			// Internal only; not using shared_ptr here.
	};
	vector<BlockRange> blockRanges;

	// Track to blockRanges mappings.
	// Index = track# (minus 1)
	// Value = pointer to BlockRange in blockRanges.
	vector<BlockRange*> trackMappings;

	// Number of logical 2048-byte blocks.
	// Determined by the highest data track.
	unsigned int blockCount;

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

	/**
	 * Open a track.
	 * @param trackNumber Track number. (starts with 1)
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int openTrack(int trackNumber);

	/**
	 * Get the starting LBA and size of the specified track number.
	 * @param trackNumber	[in] Track number (1-based)
	 * @param lba_start	[out] Starting LBA
	 * @param lba_size	[out] Length of track, in LBAs
	 * @return 0 on success; non-zero on error.
	 */
	int getTrackLBAInfo(int trackNumber, unsigned int &lba_start, unsigned int &lba_size);
};

/** GdiReaderPrivate **/

GdiReaderPrivate::GdiReaderPrivate(GdiReader *q)
	: super(q)
	, blockCount(0)
{ }

GdiReaderPrivate::~GdiReaderPrivate()
{
	close();
}

/**
 * Close all opened files.
 */
void GdiReaderPrivate::close(void)
{
	for (BlockRange &blockRange : blockRanges) {
		delete blockRange.file;
	}
	blockRanges.clear();
	trackMappings.clear();

	// GDI file.
	RP_Q(GdiReader);
	q->m_file.reset();
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
	char *linesaveptr = nullptr;
	const char *linetoken = strtok_r(gdibuf, "\n", &linesaveptr);
	if (!linetoken || linetoken[0] == 0) {
		return -EIO;
	}

	char *endptr = nullptr;
	const int trackCount = strtol(linetoken, &endptr, 10);
	if (trackCount <= 0 || trackCount > 99 || (*endptr != 0 && *endptr != '\r')) {
		// Track count is invalid.
		return -EIO;
	}

	blockRanges.reserve(static_cast<size_t>(trackCount));
	trackMappings.resize(static_cast<size_t>(trackCount));

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
		char filename[65];	// Filenames shouldn't be that long...
		int count = sscanf(linetoken, "%d %d %d %d %64s %d",
			&trackNumber, &blockStart, &type, &sectorSize, filename, &reserved);
		if (count != 6) {
			// Invalid line.
			return -EIO;
		}
		filename[sizeof(filename)-1] = 0;

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
		const size_t idx = blockRanges.size();
		blockRanges.resize(idx+1);
		BlockRange &blockRange = blockRanges[idx];
		blockRange.blockStart = static_cast<unsigned int>(blockStart);
		blockRange.blockEnd = 0;	// will be filled in when loaded
		blockRange.sectorSize = static_cast<uint16_t>(sectorSize);
		blockRange.trackNumber = static_cast<uint8_t>(trackNumber);
		blockRange.reserved = 0;
		// FIXME: UTF-8 or Latin-1?
		filename[sizeof(filename)-1] = 0;
		blockRange.filename = latin1_to_utf8(filename, -1);
		blockRange.file = nullptr;

		// Save the track mapping.
		trackMappings[trackNumber-1] = &blockRange;
	}

	// Done parsing the GDI.
	// TODO: Sort by LBA?
	return 0;
}

/**
 * Open a track.
 * @param trackNumber Track number. (starts with 1)
 * @return 0 on success; negative POSIX error code on error.
 */
int GdiReaderPrivate::openTrack(int trackNumber)
{
	assert(trackNumber > 0);
	assert(trackNumber <= 99);
	if (trackNumber <= 0 || trackNumber > 99) {
		return -EINVAL;
	}

	// Check if this track exists.
	// NOTE: trackNumber starts at 1, not 0.
	if (trackNumber > static_cast<int>(trackMappings.size())) {
		// Track number is out of range.
		return -ENOENT;
	}

	BlockRange *const blockRange = trackMappings[trackNumber-1];
	if (!blockRange) {
		// No block range. Track either doesn't exist
		// or is an audio track.
		return -ENOENT;
	}

	if (blockRange->file) {
		// File is already open.
		return 0;
	}

	// Separate the file extension.
	string basename = blockRange->filename;
	string ext;
	const size_t dotpos = basename.find_last_of('.');
	if (dotpos != string::npos) {
		ext = basename.substr(dotpos);
		basename.resize(dotpos);
	} else {
		// No extension. Add one based on sector size.
		ext = (blockRange->sectorSize == 2048 ? ".iso" : ".bin");
	}

	// Open the related file.
	IRpFile *const file = FileSystem::openRelatedFile_rawptr(filename.c_str(), basename.c_str(), ext.c_str());
	if (!file) {
		// Unable to open the file.
		// TODO: Return the actual error.
		return -ENOENT;
	}

	// File opened. Get its size and calculate the end block.
	const off64_t fileSize = file->size();
	if (fileSize <= 0) {
		// Empty or invalid file...
		delete file;
		return -EIO;
	}

	// Is the file a multiple of the sector size?
	if (fileSize % blockRange->sectorSize != 0) {
		// Not a multiple of the sector size.
		delete file;
		return -EIO;
	}

	// File opened.
	blockRange->blockEnd = blockRange->blockStart + static_cast<unsigned int>(fileSize / blockRange->sectorSize) - 1;
	blockRange->file = file;
	return 0;
}

/**
 * Get the starting LBA and size of the specified track number.
 * @param trackNumber	[in] Track number (1-based)
 * @param lba_start	[out] Starting LBA
 * @param lba_size	[out] Length of track, in LBAs
 * @return 0 on success; negative POSIX error code on error.
 */
int GdiReaderPrivate::getTrackLBAInfo(int trackNumber, unsigned int &lba_start, unsigned int &lba_size)
{
	if (openTrack(trackNumber) != 0) {
		// Cannot open the track.
		return -EIO;
	}

	if (trackMappings.size() < static_cast<size_t>(trackNumber)) {
		// Invalid track number.
		return -EINVAL;
	}
	GdiReaderPrivate::BlockRange *const blockRange = trackMappings[trackNumber-1];

	// Calculate the track length.
	lba_start = blockRange->blockStart;
	lba_size = blockRange->blockEnd - lba_start + 1;
	return 0;
}

/** GdiReader **/

GdiReader::GdiReader(const IRpFilePtr &file)
	: super(new GdiReaderPrivate(this), file)
{
	if (!m_file) {
		// File could not be ref()'d.
		return;
	}

	// Save the filename for later.
	RP_D(GdiReader);
	const char *const filename = m_file->filename();
	if (filename) {
		d->filename = filename;
	}

	// GDI file should be 4k or less.
	const off64_t fileSize = m_file->size();
	if (fileSize <= 0 || fileSize > 4096) {
		// Invalid GDI file size.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	// Read the GDI and parse the track information.
	const unsigned int gdisize = static_cast<unsigned int>(fileSize);
	unique_ptr<char[]> gdibuf(new char[gdisize+1]);
	m_file->rewind();
	size_t size = m_file->read(gdibuf.get(), gdisize);
	if (size != gdisize) {
		// Read error.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	// Make sure the string is NULL-terminated.
	gdibuf[gdisize] = 0;

	// Parse the GDI file.
	int ret = d->parseGdiFile(gdibuf.get());
	if (ret != 0) {
		// Error parsing the GDI file.
		d->close();
		m_lastError = EIO;
		return;
	}

	// Open track 03 (primary data track) and the last data track.
	if (d->trackMappings.size() >= 3) {
		ret = d->openTrack(3);
		if (ret != 0) {
			// Error opening track 03.
			d->close();
			m_lastError = -ret;
			return;
		}
	}

	// Find the last data track.
	// NOTE: Searching in reverse order.
	int lastDataTrack = 0;	// 1-based; 0 is invalid.
	std::for_each(d->trackMappings.crbegin(), d->trackMappings.crend(),
		[&lastDataTrack](const GdiReaderPrivate::BlockRange *blockRange) noexcept -> void {
			if (blockRange) {
				if (static_cast<int>(blockRange->trackNumber) > lastDataTrack) {
					lastDataTrack = blockRange->trackNumber;
				}
			}
		}
	);

	if (lastDataTrack > 0 && lastDataTrack != 3) {
		ret = d->openTrack(lastDataTrack);
		if (ret != 0) {
			// Error opening the last data track.
			d->close();
			m_lastError = -ret;
			return;
		}
	}

	const GdiReaderPrivate::BlockRange *const lastBlockRange = d->trackMappings[lastDataTrack-1];
	if (!lastBlockRange) {
		// Should not get here...
		d->close();
		m_lastError = EIO;
		return;
	}

	// Disc parameters.
	// A full Dreamcast disc has 549,150 sectors.
	d->block_size = 2048;
	d->blockCount = lastBlockRange->blockEnd + 1;
	d->disc_size = d->blockCount * 2048;

	// Reset the disc position.
	d->pos = 0;
}

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
	for (unsigned int i = 0; i < 4; i++) {
		if (pHeader[i] == '\r' || pHeader[i] == '\n') {
			// End of line.
			break;
		} else if (ISDIGIT(pHeader[i])) {
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

/** SparseDiscReader functions **/

/**
 * Get the physical address of the specified logical block index.
 *
 * NOTE: Not implemented in this subclass.
 *
 * @param blockIdx	[in] Block index.
 * @return Physical block address. (-1 due to not being implemented)
 */
off64_t GdiReader::getPhysBlockAddr(uint32_t blockIdx) const
{
	RP_UNUSED(blockIdx);
	assert(!"GdiReader::getPhysBlockAddr() is not implemented.");
	return -1;
}

/**
 * Read the specified block.
 *
 * This can read either a full block or a partial block.
 * For a full block, set pos = 0 and size = block_size.
 *
 * @param blockIdx	[in] Block index.
 * @param pos		[in] Starting position. (Must be >= 0 and <= the block size!)
 * @param ptr		[out] Output data buffer.
 * @param size		[in] Amount of data to read, in bytes. (Must be <= the block size!)
 * @return Number of bytes read, or -1 if the block index is invalid.
 */
int GdiReader::readBlock(uint32_t blockIdx, int pos, void *ptr, size_t size)
{
	// Read 'size' bytes of block 'blockIdx', starting at 'pos'.
	// NOTE: This can only be called by SparseDiscReader,
	// so the main assertions are already checked there.
	RP_D(GdiReader);
	assert(pos >= 0 && pos < (int)d->block_size);
	assert(size <= d->block_size);
	assert(blockIdx < d->blockCount);
	// TODO: Make sure overflow doesn't occur.
	assert((off64_t)(pos + size) <= (off64_t)d->block_size);
	if (pos < 0 || pos >= static_cast<int>(d->block_size) ||
		size > d->block_size ||
		static_cast<off64_t>(pos + size) > static_cast<off64_t>(d->block_size) ||
	    blockIdx >= d->blockCount)
	{
		// pos+size is out of range.
		return -1;
	}

	if (unlikely(size == 0)) {
		// Nothing to read.
		return 0;
	}

	// Find the block.
	// TODO: Cache this lookup somewhere or something.
	const GdiReaderPrivate::BlockRange *blockRange = nullptr;
	for (const GdiReaderPrivate::BlockRange &vbr : d->blockRanges) {
		if (blockIdx < vbr.blockStart) {
			// Not in this track.
			continue;
		}

		// Is the track loaded?
		if (vbr.blockEnd == 0) {
			// Track isn't loaded. Load it.
			int ret = d->openTrack(vbr.trackNumber);
			if (ret != 0) {
				// Unable to load the track.
				// Skip for now.
				continue;
			}
		}

		// Check the end block.
		if (vbr.blockEnd != 0 && blockIdx <= vbr.blockEnd) {
			// Found the track.
			blockRange = (const GdiReaderPrivate::BlockRange*)&vbr;
			break;
		}
	}

	if (!blockRange) {
		// Not found in any block range.
		return 0;
	}

	assert(blockRange->file != nullptr);
	if (!blockRange->file) {
		// File *still* isn't open...
		return 0;
	}

	// Read the full block.
	const off64_t phys_pos = (static_cast<off64_t>(blockIdx - blockRange->blockStart) * blockRange->sectorSize);
	if (blockRange->sectorSize == 2352) {
		// 2352-byte sectors.
		// TODO: Handle audio tracks properly?
		CDROM_2352_Sector_t sector;
		size_t sz_read = blockRange->file->seekAndRead(phys_pos, &sector, sizeof(sector));
		m_lastError = blockRange->file->lastError();
		if (sz_read != sizeof(sector)) {
			// Read error.
			return -1;
		}

		// NOTE: Sector user data area position depends on the sector mode.
		const uint8_t *const data = cdromSectorDataPtr(&sector);
		memcpy(ptr, &data[pos], size);
		return static_cast<int>(size);
	}

	// 2048-byte sectors.
	size_t sz_read = blockRange->file->seekAndRead(phys_pos, ptr, size);
	return (sz_read > 0 ? static_cast<int>(sz_read) : -1);
}

/** GDI-specific functions **/
// TODO: "CdromReader" class?

/**
 * Get the track count.
 * @return Track count.
 */
int GdiReader::trackCount(void) const
{
	RP_D(const GdiReader);
	return static_cast<int>(d->trackMappings.size());
}

/**
 * Get the starting LBA of the specified track number.
 * @param trackNumber Track number. (1-based)
 * @return Starting LBA, or -1 if the track number is invalid.
 */
int GdiReader::startingLBA(int trackNumber) const
{
	assert(trackNumber > 0);
	assert(trackNumber <= 99);

	RP_D(GdiReader);
	if (trackNumber <= 0 || trackNumber > static_cast<int>(d->trackMappings.size()))
		return -1;

	const GdiReaderPrivate::BlockRange *blockRange = d->trackMappings[trackNumber-1];
	if (!blockRange)
		return -1;

	return static_cast<int>(blockRange->blockStart);
}

/**
 * Open a track using IsoPartition.
 * @param trackNumber Track number. (1-based)
 * @return IsoPartition, or nullptr on error.
 */
IsoPartitionPtr GdiReader::openIsoPartition(int trackNumber)
{
	RP_D(GdiReader);

	unsigned int lba_start, lba_size;
	if (d->getTrackLBAInfo(trackNumber, lba_start, lba_size) != 0) {
		// Unable to get track LBA info.
		return nullptr;
	}

	// FIXME: IsoPartition's constructor requires an IDiscReaderPtr
	// or IRpFilePtr, but we don't have our own shared_ptr<> available.
	// Workaround: Create a PartitionFile and use that.
	PartitionFilePtr isoFile = std::make_shared<PartitionFile>(this,
		static_cast<off64_t>(lba_start) * 2048,
		static_cast<off64_t>(lba_size) * 2048);
	if (!isoFile->isOpen()) {
		// Unable to open the PartitionFile.
		return nullptr;
	}

	// Logical block size is 2048.
	// ISO starting offset is the LBA.
	return std::make_shared<IsoPartition>(isoFile, 0, lba_start);
}

/**
 * Create an ISO RomData object for a given track number.
 * @param trackNumber Track number. (1-based)
 * @return ISO object, or nullptr on error.
 */
ISOPtr GdiReader::openIsoRomData(int trackNumber)
{
	RP_D(GdiReader);

	unsigned int lba_start, lba_size;
	if (d->getTrackLBAInfo(trackNumber, lba_start, lba_size) != 0) {
		// Unable to get track LBA info.
		return nullptr;
	}

	PartitionFilePtr isoFile = std::make_shared<PartitionFile>(this,
		static_cast<off64_t>(lba_start) * 2048,
		static_cast<off64_t>(lba_size) * 2048);
	if (isoFile->isOpen()) {
		ISOPtr isoData = std::make_shared<ISO>(isoFile);
		if (isoData->isOpen()) {
			// ISO is opened.
			return isoData;
		}
	}

	// Unable to open the ISO object.
	return nullptr;
}

}
