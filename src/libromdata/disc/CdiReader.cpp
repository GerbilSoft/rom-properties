/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CdiReader.cpp: DiscJuggler CDI image reader.                            *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CdiReader.hpp"
#include "librpbase/disc/SparseDiscReader_p.hpp"

#include "../cdrom_structs.h"
#include "IsoPartition.hpp"

// Other rom-properties libraries
#include "librpfile/RelatedFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// Other RomData subclasses
#include "Media/ISO.hpp"

// C++ STL classes
#include <limits>
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class CdiReaderPrivate final : public SparseDiscReaderPrivate
{
public:
	explicit CdiReaderPrivate(CdiReader *q);

private:
	typedef SparseDiscReaderPrivate super;
	RP_DISABLE_COPY(CdiReaderPrivate)

public:
	// GDI filename
	string filename;

	// Block range mapping
	// NOTE: This currently *only* contains data tracks.
	struct BlockRange {
		unsigned int blockStart;	// First LBA
		unsigned int blockEnd;		// Last LBA (inclusive) (0 if the file hasn't been opened yet)
		unsigned int pregapLength;	// Pregap length
		uint16_t sectorSize;		// 2048, 2336, or 2352
		uint8_t trackNumber;		// 01 through 99
		uint8_t reserved;
		// TODO: Data vs. audio?
		off64_t trackStart;		// Track starting address in the .cdi file
	};
	vector<BlockRange> blockRanges;

	// Track to blockRanges mappings.
	// - Index = track# (minus 1)
	// - Value = index in blockRanges [-1 if not mapped]
	// NOTE: Must use index in blockRanges. Using pointers fails if
	// blockRanges is internally allocated.
	vector<int8_t> trackMappings;

	// Number of logical 2048-byte blocks.
	// Determined by the highest data track.
	unsigned int blockCount;

	enum class CdiVersion : uint32_t {
		CDI_V2 = 0x80000004,
		CDI_V3 = 0x80000005,
		CDI_V35 = 0x80000006,
		CDI_V4 = CDI_V35,
	};

	/**
	 * Close all opened files.
	 */
	void close(void);

	/**
	 * Parse the CDI file.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int parseCdiFile(void);

	/**
	 * Get the starting LBA and size of the specified track number.
	 * @param trackNumber	[in] Track number (1-based)
	 * @param lba_start	[out] Starting LBA
	 * @param lba_size	[out] Length of track, in LBAs
	 * @param pregap_length	[out] Pregap length, in LBAs
	 * @return 0 on success; non-zero on error.
	 */
	int getTrackLBAInfo(int trackNumber, unsigned int &lba_start, unsigned int &lba_size, unsigned int &pregap_length);
};

/** CdiReaderPrivate **/

CdiReaderPrivate::CdiReaderPrivate(CdiReader *q)
	: super(q)
	, blockCount(0)
{}

/**
 * Close all opened files.
 */
void CdiReaderPrivate::close(void)
{
	blockRanges.clear();
	trackMappings.clear();

	// CDI file
	RP_Q(CdiReader);
	q->m_file.reset();
}

/**
 * Parse the CDI file.
 * @return 0 on success; negative POSIX error code on error.
 */
int CdiReaderPrivate::parseCdiFile(void)
{
	assert(blockRanges.empty());
	assert(trackMappings.empty());
	if (!blockRanges.empty() || !trackMappings.empty()) {
		// CDI is already loaded...
		return -EEXIST;
	}

	// Based on dcparser.py: https://gist.github.com/Holzhaus/ae3dacf6a2e83dd00421

	struct {
		CdiVersion version;
		uint32_t header_offset;
	} cdiFooter;

	// Check the image version and header offset.
	RP_Q(CdiReader);
	const off64_t fileSize = q->m_file->size();
	size_t size = q->m_file->seekAndRead(fileSize - 8, &cdiFooter, sizeof(cdiFooter));
	if (size != sizeof(cdiFooter)) {
		q->m_lastError = EIO;
		return -EIO;
	}
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	cdiFooter.version = static_cast<CdiVersion>(le32_to_cpu(static_cast<uint32_t>(cdiFooter.version)));
	cdiFooter.header_offset = le32_to_cpu(cdiFooter.header_offset);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	switch (cdiFooter.version) {
		case CdiVersion::CDI_V2:
		case CdiVersion::CDI_V3:
		case CdiVersion::CDI_V35:
		//case CdiVersion::CDI_V4:
			break;
		default:
			// Not supported...
			// TODO: Better error code.
			q->m_lastError = EIO;
			return -EIO;
	}

	// Absolute header offset is filesize - cdiFooter.header_offset
	off64_t header_offset = fileSize - cdiFooter.header_offset;

	// Read the session count.
	uint16_t num_sessions;
	size = q->m_file->seekAndRead(header_offset, &num_sessions, sizeof(num_sessions));
	if (size != sizeof(num_sessions) || num_sessions == 0) {
		// Seek and/or read error, or zero sessions...
		q->m_lastError = EIO;
		return -EIO;
	}
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	num_sessions = le16_to_cpu(num_sessions);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

	// Read each session.
	int trackNumber = 1;	// starts at 1, not 0
	off64_t track_offset = 0;
	for (int session = 0; session < num_sessions; session++) {
		// Number of tracks in this session.
		uint16_t num_tracks;
		size = q->m_file->read(&num_tracks, sizeof(num_tracks));
		if (size != sizeof(num_tracks) || num_tracks == 0) {
			// Seek and/or read error, or zero tracks...
			q->m_lastError = EIO;
			return -EIO;
		}
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
		num_tracks = le16_to_cpu(num_tracks);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

		// Read each track.
		for (int track = 0; track < num_tracks; track++, trackNumber++) {
			// Parse the track data.
			uint32_t temp_value;
			size = q->m_file->read(&temp_value, sizeof(temp_value));
			if (size != sizeof(temp_value)) {
				q->m_lastError = EIO;
				return -EIO;
			}

			// Check the track start mark. (should be two instances)
			static const array<uint8_t, 10> TRACK_START_MARK = {0, 0, 0x01, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF};
			array<uint8_t, 20> current_start_mark;
			size = q->m_file->read(current_start_mark.data(), current_start_mark.size());
			if (size != current_start_mark.size() ||
			    memcmp(&current_start_mark[ 0], TRACK_START_MARK.data(), TRACK_START_MARK.size()) != 0 ||
			    memcmp(&current_start_mark[10], TRACK_START_MARK.data(), TRACK_START_MARK.size()) != 0)
			{
				q->m_lastError = EIO;
				return -EIO;
			}

			// Original filename (not saving the actual filename for now)
			uint8_t filename_length;
			q->m_file->seek_cur(4);
			size = q->m_file->read(&filename_length, sizeof(filename_length));
			if (size != sizeof(filename_length)) {
				q->m_lastError = EIO;
				return -EIO;
			}
			q->m_file->seek_cur(filename_length + 11 + 4 + 4);

			size = q->m_file->read(&temp_value, sizeof(temp_value));
			if (size != sizeof(temp_value)) {
				q->m_lastError = EIO;
				return -EIO;
			}
			if (temp_value == cpu_to_le32(0x80000000)) {
				// DiscJuggler 4: Skip the next 8 bytes.
				q->m_file->seek_cur(8);
			}
			q->m_file->seek_cur(2);

#pragma pack(2)
			struct PACKED {
				uint32_t pregap_length;
				uint32_t length;
				uint8_t unused1[6];
				uint32_t mode;
				uint8_t unused2[12];
				uint32_t start_lba;
				uint32_t total_length;
			} lengthFields;
#pragma pack()
			size = q->m_file->read(&lengthFields, sizeof(lengthFields));
			if (size != sizeof(lengthFields)) {
				q->m_lastError = EIO;
				return -EIO;
			}

			// Sector size ID
			uint32_t sectorSizeID;
			q->m_file->seek_cur(16);
			size = q->m_file->read(&sectorSizeID, sizeof(sectorSizeID));
			if (size != sizeof(sectorSizeID)) {
				q->m_lastError = EIO;
				return -EIO;
			}
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
			sectorSizeID = le32_to_cpu(sectorSizeID);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

			// Convert the sector size ID to an actual sector size.
			static const array<uint32_t, 3> sectorSizeIDtoSizeMap = {{2048, 2336, 2352}};
			if (sectorSizeID >= sectorSizeIDtoSizeMap.size()) {
				q->m_lastError = EIO;
				return -EIO;
			}
			const uint32_t sectorSize = sectorSizeIDtoSizeMap[sectorSizeID];

			// Check the track mode.
			// Data tracks are saved; audio tracks are not.
			// NOTE: This field appears to be 2 for data tracks in my test images,
			// but dcparser.py accepts anything that's non-zero.
			if (lengthFields.mode != 0) {
				// Save the track information.
				const size_t idx = blockRanges.size();
				assert(idx <= std::numeric_limits<int8_t>::max());
				if (idx > std::numeric_limits<int8_t>::max()) {
					// Too many tracks. (More than 127???)
					return -ENOMEM;
				}
				const uint32_t pregap_length = le32_to_cpu(lengthFields.pregap_length);
				blockRanges.resize(idx+1);
				BlockRange &blockRange = blockRanges[idx];
				blockRange.blockStart = le32_to_cpu(lengthFields.start_lba) + pregap_length;
				blockRange.blockEnd = blockRange.blockStart + le32_to_cpu(lengthFields.length) - 1;
				blockRange.pregapLength = pregap_length;
				blockRange.sectorSize = sectorSize;
				blockRange.trackNumber = static_cast<uint8_t>(trackNumber);
				blockRange.reserved = 0;
				blockRange.trackStart = track_offset + (static_cast<off64_t>(pregap_length) * sectorSize);

				// Save the track mapping.
				trackMappings.push_back(static_cast<int8_t>(idx));
			} else {
				// Not a data track.
				trackMappings.push_back(-1);
			}

			// Go to the next track.
			track_offset += static_cast<off64_t>(le32_to_cpu(lengthFields.total_length)) * static_cast<off64_t>(sectorSize);
			q->m_file->seek_cur(29);
			if (cdiFooter.version != CdiVersion::CDI_V2) {
				q->m_file->seek_cur(5);
				size = q->m_file->read(&temp_value, sizeof(temp_value));
				if (size != sizeof(temp_value)) {
					q->m_lastError = EIO;
					return -EIO;
				}
				if (temp_value == 0xFFFFFFFF) {
					// DiscJuggler 3.00.780+: Extra data
					q->m_file->seek_cur(78);
				}
			}
		}

		// Go to the next session.
		q->m_file->seek_cur(4);
		q->m_file->seek_cur(8);
		if (cdiFooter.version != CdiVersion::CDI_V2) {
			q->m_file->seek_cur(1);
		}
	}

	// Done parsing the CDI.
	// TODO: Sort by LBA?
	return 0;
}

/**
 * Get the starting LBA and size of the specified track number.
 * @param trackNumber	[in] Track number (1-based)
 * @param lba_start	[out] Starting LBA
 * @param lba_size	[out] Length of track, in LBAs
 * @param pregap_length	[out] Pregap length, in LBAs
 * @return 0 on success; negative POSIX error code on error.
 */
int CdiReaderPrivate::getTrackLBAInfo(int trackNumber, unsigned int &lba_start, unsigned int &lba_size, unsigned int &pregap_length)
{
	if (trackMappings.empty()) {
		// No tracks...
		return -EIO;
	} else if (trackMappings.size() < static_cast<size_t>(trackNumber)) {
		// Invalid track number.
		return -EINVAL;
	}

	int mapping = trackMappings[trackNumber-1];
	if (mapping < 0) {
		// No block range. Track either doesn't exist
		// or is an audio track.
		return -ENOENT;
	}

	// Calculate the track length.
	const BlockRange &blockRange = blockRanges[mapping];
	lba_start = blockRange.blockStart;
	lba_size = blockRange.blockEnd - lba_start + 1;
	pregap_length = blockRange.pregapLength;
	return 0;
}

/** CdiReader **/

CdiReader::CdiReader(const IRpFilePtr &file)
	: super(new CdiReaderPrivate(this), file)
{
	if (!m_file) {
		// File could not be ref()'d.
		return;
	}

	// Save the filename for later.
	RP_D(CdiReader);
	const char *const filename = m_file->filename();
	if (filename) {
		d->filename = filename;
	}

	// Parse the CDI file.
	int ret = d->parseCdiFile();
	if (ret != 0) {
		// Error parsing the GDI file.
		d->close();
		m_lastError = EIO;
		return;
	}

	// Find the last data track.
	// NOTE: Searching in reverse order.
	int lastDataTrack = 0;	// 1-based; 0 is invalid.
	std::for_each(d->trackMappings.crbegin(), d->trackMappings.crend(),
		[&lastDataTrack, d](const int mapping) noexcept -> void {
			if (mapping >= 0) {
				const CdiReaderPrivate::BlockRange &blockRange = d->blockRanges[mapping];
				if (static_cast<int>(blockRange.trackNumber) > lastDataTrack) {
					lastDataTrack = blockRange.trackNumber;
				}
			}
		}
	);

	if (lastDataTrack <= 0) {
		// No data track.
		d->close();
		m_lastError = EIO;
		return;
	}

	const int mapping_lastBlockRange = d->trackMappings[lastDataTrack-1];
	if (mapping_lastBlockRange < 0) {
		// Should not get here...
		d->close();
		m_lastError = EIO;
		return;
	}

	// Disc parameters.
	// A full Dreamcast disc has 549,150 sectors.
	d->block_size = 2048;
	d->blockCount = d->blockRanges[mapping_lastBlockRange].blockEnd + 1;
	d->disc_size = d->blockCount * 2048;

	// Reset the disc position.
	d->pos = 0;
}

/**
 * Is a disc image supported by this object?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int CdiReader::isDiscSupported(const uint8_t *pHeader, size_t szHeader) const
{
	// FIXME: This function doesn't work, since the CDI version number
	// is stored at the end of the file.
	RP_UNUSED(pHeader);
	RP_UNUSED(szHeader);
	return -1;
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
off64_t CdiReader::getPhysBlockAddr(uint32_t blockIdx) const
{
	RP_UNUSED(blockIdx);
	assert(!"CdiReader::getPhysBlockAddr() is not implemented.");
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
int CdiReader::readBlock(uint32_t blockIdx, int pos, void *ptr, size_t size)
{
	// Read 'size' bytes of block 'blockIdx', starting at 'pos'.
	// NOTE: This can only be called by SparseDiscReader,
	// so the main assertions are already checked there.
	RP_D(CdiReader);
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
	const CdiReaderPrivate::BlockRange *blockRange = nullptr;
	for (const CdiReaderPrivate::BlockRange &vbr : d->blockRanges) {
		if (blockIdx < vbr.blockStart) {
			// Not in this track.
			continue;
		}

		// Check the end block.
		if (vbr.blockEnd != 0 && blockIdx <= vbr.blockEnd) {
			// Found the track.
			blockRange = (const CdiReaderPrivate::BlockRange*)&vbr;
			break;
		}
	}

	if (!blockRange) {
		// Not found in any block range.
		return 0;
	}

	// Read the full block
	const off64_t phys_pos = blockRange->trackStart + (static_cast<off64_t>(blockIdx - blockRange->blockStart) * blockRange->sectorSize);
	if (blockRange->sectorSize == 2352) {
		// 2352-byte sectors
		// TODO: Handle audio tracks properly?
		CDROM_2352_Sector_t sector;
		size_t sz_read = m_file->seekAndRead(phys_pos, &sector, sizeof(sector));
		m_lastError = m_file->lastError();
		if (sz_read != sizeof(sector)) {
			// Read error
			return -1;
		}

		// NOTE: Sector user data area position depends on the sector mode.
		const uint8_t *const data = cdromSectorDataPtr(&sector);
		memcpy(ptr, &data[pos], size);
		return static_cast<int>(size);
	} else if (blockRange->sectorSize == 2336) {
		// Skip the first 8 bytes of the 2336-byte sector.
		array<uint8_t, 2336> sector;
		size_t sz_read = m_file->seekAndRead(phys_pos, sector.data(), sector.size());
		if (sz_read != sector.size()) {
			// Read error
			return -1;
		}

		memcpy(ptr, &sector[8+pos], size);
		return static_cast<int>(size);
	}

	// 2048-byte sectors
	size_t sz_read = m_file->seekAndRead(phys_pos, ptr, size);
	return (sz_read > 0 ? static_cast<int>(sz_read) : -1);
}

/** GDI-specific functions **/
// TODO: "CdromReader" class?

/**
 * Get the track count.
 * @return Track count.
 */
int CdiReader::trackCount(void) const
{
	RP_D(const CdiReader);
	return static_cast<int>(d->trackMappings.size());
}

/**
 * Get the starting LBA of the specified track number.
 * @param trackNumber Track number. (1-based)
 * @return Starting LBA, or -1 if the track number is invalid.
 */
int CdiReader::startingLBA(int trackNumber) const
{
	assert(trackNumber > 0);
	assert(trackNumber <= 99);

	RP_D(CdiReader);
	if (trackNumber <= 0 || trackNumber > static_cast<int>(d->trackMappings.size()))
		return -1;

	const int mapping = d->trackMappings[trackNumber-1];
	if (mapping < 0)
		return -1;

	return static_cast<int>(d->blockRanges[mapping].blockStart);
}

/**
 * Open a track using IsoPartition.
 * @param trackNumber Track number. (1-based)
 * @return IsoPartition, or nullptr on error.
 */
IsoPartitionPtr CdiReader::openIsoPartition(int trackNumber)
{
	RP_D(CdiReader);

	unsigned int lba_start, lba_size, pregap_length;
	if (d->getTrackLBAInfo(trackNumber, lba_start, lba_size, pregap_length) != 0) {
		// Unable to get track LBA info.
		return nullptr;
	}

	// Logical block size is 2048.
	// ISO starting offset is the LBA.
	return std::make_shared<IsoPartition>(this->shared_from_this(),
		static_cast<off64_t>(lba_start) * 2048, lba_start - pregap_length);
}

/**
 * Create an ISO RomData object for a given track number.
 * @param trackNumber Track number. (1-based)
 * @return ISO object, or nullptr on error.
 */
ISOPtr CdiReader::openIsoRomData(int trackNumber)
{
	RP_D(CdiReader);

	unsigned int lba_start, lba_size, pregap_length;
	if (d->getTrackLBAInfo(trackNumber, lba_start, lba_size, pregap_length) != 0) {
		// Unable to get track LBA info.
		return nullptr;
	}

	PartitionFilePtr isoFile = std::make_shared<PartitionFile>(this->shared_from_this(),
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

} // namespace LibRomData
