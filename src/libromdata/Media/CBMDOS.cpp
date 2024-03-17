/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CBMDOS.cpp: Commodore DOS floppy disk image parser.                     *
 *                                                                         *
 * Copyright (c) 2019-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - http://unusedino.de/ec64/technical/formats/d64.html
// - http://unusedino.de/ec64/technical/formats/d71.html
// - http://unusedino.de/ec64/technical/formats/d80-d82.html
// - http://unusedino.de/ec64/technical/formats/d81.html
// - http://unusedino.de/ec64/technical/formats/g64.html
// - https://area51.dev/c64/cbmdos/autoboot/
// - http://unusedino.de/ec64/technical/formats/geos.html
// - https://sourceforge.net/p/vice-emu/patches/122/ (for .g71)

#include "stdafx.h"
#include "CBMDOS.hpp"

#include "cbmdos_structs.h"

// Other rom-properties libraries
#include "libi18n/i18n.h"
#include "librptexture/decoder/ImageDecoder_Linear_Gray.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// C++ STL classes
#include <bitset>
using std::array;
using std::bitset;
using std::string;
using std::vector;

// Uninitialized vector class
#include "uvector.h"

namespace LibRomData {

class CBMDOSPrivate final : public RomDataPrivate
{
public:
	CBMDOSPrivate(const IRpFilePtr &file);
	~CBMDOSPrivate();

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(CBMDOSPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// Disk type
	enum class DiskType {
		Unknown = -1,

		D64 = 0,		// C1541 disk image (single-sided, standard version)
		D71 = 1,		// C1571 disk image (double-sided, standard version)
		D80 = 2,		// C8050 disk image (single-sided, standard version)
		D82 = 3,		// C8250 disk image (double-sided, standard version)
		D81 = 4,		// C1581 disk image (double-sided, standard version)
		D67 = 5,		// C2040/C3030 disk image (single-sided, standard version)

		G64 = 6,		// C1541 disk image (single-sided, GCR format)
		G71 = 7,		// C1571 disk image (double-sided, GCR format)

		Max
	};
	DiskType diskType;

	// Directory track
	// Usually 18 for C1541/C1571 disks.
	uint8_t dir_track;

	// First directory sector
	// Usually 1, but may be 3 for C1581.
	uint8_t dir_first_sector;

	// Currently cached G64/G71 track. (0 == none)
	uint8_t GCR_track_cache_number;

	// Error bytes info (for certain D64/D71 format images)
	unsigned int err_bytes_count;
	unsigned int err_bytes_offset;

public:
	// Track offsets
	// Index is track number, minus one.
	struct track_offsets_t {
		uint8_t sector_count;		// Sectors per track
		uint8_t reserved[3];
		unsigned int start_offset;	// Starting offset (in bytes)
	};
	rp::uvector<track_offsets_t> track_offsets;

	/**
	 * Initialize track offsets for C1541. (35/40 tracks)
	 * @param isDos1 If true, use 20 sectors instead of 19 in speed zone 2. (DOS 1.x, e.g. C2040)
	 */
	void init_track_offsets_C1541(bool isDos1);

	/**
	 * Initialize track offsets for C1571. (70 tracks)
	 */
	void init_track_offsets_C1571(void);

	/**
	 * Initialize track offsets for C8050. (77 tracks)
	 * @param isC8250 If true, initialize for C8250. (154 tracks)
	 */
	void init_track_offsets_C8050(bool isC8250);

	/**
	 * Initialize track offsets for C1581. (80 tracks)
	 */
	void init_track_offsets_C1581(void);

	/**
	 * Initialize tracks for a G64/G71 (GCR-1541/GCR-1571) image. (up to 42 or 84 tracks)
	 * @param header G64/G71 header
	 */
	void init_track_offsets_G64(const cbmdos_G64_header_t *header);

public:
	// GCR track buffer (up to 21 sectors for .g64/.g71)
	struct GCR_track_buffer_t {
		uint8_t sectors[21][CBMDOS_SECTOR_SIZE];
	};
	GCR_track_buffer_t *GCR_track_buffer;

	// GCR track size (usually 7,928; we'll allow up to 8,192)
	unsigned int GCR_track_size;
#define GCR_MAX_TRACK_SIZE 8192U

public:
	// Disk header
	// Includes the disk name.
	union {
		cbmdos_C1541_BAM_t c1541;	// also used for C1571
		cbmdos_C8050_header_t c8050;
		cbmdos_C1581_header_t c1581;
	} diskHeader;

public:
	/**
	 * Decode 5 GCR bytes into 4 data bytes.
	 * @param data	[out] Output buffer for 4 data bytes
	 * @param gcr	[in] Input buffer with 5 GCR bytes
	 * @return 0 on success; non-zero on error.
	 */
	static int decode_GCR_bytes(uint8_t *data, const uint8_t *gcr);

	/**
	 * Read and decode a GCR track from the disk image.
	 * This will be cached into GCR_track_buffer.
	 * @param track	[in] Track# (starts at 1)
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int read_GCR_track(uint8_t track);

	/**
	 * Read a 256-byte sector given track/sector addresses.
	 * @param buf		[out] Output buffer
	 * @param siz		[in] Size of buf (must be CBMDOS_SECTOR_SIZE)
	 * @param track		[in] Track# (starts at 1)
	 * @param sector	[in] Sector# (starts at 0)
	 * @return Number of bytes read on success, or zero on error.
	 */
	size_t read_sector(void *buf, size_t siz, uint8_t track, uint8_t sector);

	/**
	 * Remove $A0 padding from a character buffer.
	 * @param buf	[in] Character buffer
	 * @param siz	[in] Size of buf
	 * @return String length with $A0 padding removed.
	 */
	static size_t remove_A0_padding(const char *buf, size_t siz);
};

ROMDATA_IMPL(CBMDOS)

/** CBMDOSPrivate **/

/* RomDataInfo */
const char *const CBMDOSPrivate::exts[] = {
	".d64",	".d41",	// Standard C1541 disk image
	".d71",		// Standard C1571 disk image
	".d80",		// Standard C8050 disk image
	".d82",		// Standard C8250 disk image
	".d81",		// Standard C1581 disk image
	".d67",		// Standard C2040 disk image

	".g64", ".g41",	// GCR-encoded C1541 disk imgae
	".g71",		// GCR-encoded C1571 disk image

	// TODO: More?

	nullptr
};
const char *const CBMDOSPrivate::mimeTypes[] = {
	// NOTE: Ordering matches the DiskType enum.

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-cbm-d64",
	"application/x-cbm-d71",
	"application/x-cbm-d80",
	"application/x-cbm-d82",
	"application/x-cbm-d81",
	"application/x-cbm-d67",

	"application/x-cbm-g64",
	"application/x-cbm-g71",

	// Alias types (not part of DiskType)
	"application/x-d64",
	"application/x-d71",
	"application/x-d80",
	"application/x-d82",
	"application/x-d81",
	"application/x-d67",

	"application/x-g64",
	"application/x-g71",

	"application/x-c64-datadisk",	// D64
	"application/x-c64-rawdisk",	// G64

	nullptr
};
const RomDataInfo CBMDOSPrivate::romDataInfo = {
	"CBMDOS", exts, mimeTypes
};

CBMDOSPrivate::CBMDOSPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, diskType(DiskType::Unknown)
	, dir_track(0)
	, dir_first_sector(0)
	, GCR_track_cache_number(0)
	, err_bytes_count(0)
	, err_bytes_offset(0)
	, GCR_track_buffer(nullptr)
	, GCR_track_size(0)
{
	// Clear the ROM header struct.
	memset(&diskHeader, 0, sizeof(diskHeader));
}

CBMDOSPrivate::~CBMDOSPrivate()
{
	delete GCR_track_buffer;
}

/**
 * Initialize track offsets for C1541. (35/40 tracks)
 * @param isDos1 If true, use 20 sectors instead of 19 in speed zone 2. (DOS 1.x, e.g. C2040)
 */
void CBMDOSPrivate::init_track_offsets_C1541(bool isDos1)
{
	// C1541 zones:
	// - Tracks  1-17: 21 sectors
	// - Tracks 18-24: 19 sectors (20 sectors for DOS 1.x)
	// - Tracks 25-30: 18 sectors
	// - Tracks 31-40: 17 sectors
	unsigned int offset = 0;
	track_offsets.resize(40);

	// Tracks 1-17: 21 sectors
	for (unsigned int i = 1-1; i <= 17-1; i++) {
		track_offsets[i].sector_count = 21;
		track_offsets[i].start_offset = offset;
		offset += (21 * CBMDOS_SECTOR_SIZE);
	}

	// Tracks 18-24: 19 sectors (20 sectors for DOS 1.x)
	const unsigned int zone2_sector_count = (unlikely(isDos1) ? 20 : 19);
	const unsigned int zone2_offset_adj = zone2_sector_count * CBMDOS_SECTOR_SIZE;
	for (unsigned int i = 18-1; i <= 24-1; i++) {
		track_offsets[i].sector_count = zone2_sector_count;
		track_offsets[i].start_offset = offset;
		offset += zone2_offset_adj;
	}

	// Tracks 25-30: 18 sectors
	for (unsigned int i = 25-1; i <= 30-1; i++) {
		track_offsets[i].sector_count = 18;
		track_offsets[i].start_offset = offset;
		offset += (18 * CBMDOS_SECTOR_SIZE);
	}

	// Tracks 31-40: 17 sectors
	for (unsigned int i = 31-1; i <= 40-1; i++) {
		track_offsets[i].sector_count = 17;
		track_offsets[i].start_offset = offset;
		offset += (17 * CBMDOS_SECTOR_SIZE);
	}
}

/**
 * Initialize track offsets for C1571. (70 tracks)
 */
void CBMDOSPrivate::init_track_offsets_C1571(void)
{
	// C1571 zones:

	/// Side A
	// - Tracks  1-17: 21 sectors
	// - Tracks 18-24: 19 sectors
	// - Tracks 25-30: 18 sectors
	// - Tracks 31-35: 17 sectors

	/// Side B
	// - Tracks 36-52: 21 sectors
	// - Tracks 53-59: 19 sectors
	// - Tracks 60-65: 18 sectors
	// - Tracks 66-70: 17 sectors
	unsigned int offset = 0;
	track_offsets.resize(70);

	int track_base = -1;	// track numbering starts at 1
	for (unsigned int side = 0; side < 2; side++, track_base += 35) {
		// Tracks 1-17: 21 sectors
		for (int i = 1 + track_base; i <= 17 + track_base; i++) {
			track_offsets[i].sector_count = 21;
			track_offsets[i].start_offset = offset;
			offset += (21 * CBMDOS_SECTOR_SIZE);
		}

		// Tracks 18-24: 19 sectors
		for (int i = 18 + track_base; i <= 24 + track_base; i++) {
			track_offsets[i].sector_count = 19;
			track_offsets[i].start_offset = offset;
			offset += (19 * CBMDOS_SECTOR_SIZE);
		}

		// Tracks 25-30: 18 sectors
		for (int i = 25 + track_base; i <= 30 + track_base; i++) {
			track_offsets[i].sector_count = 18;
			track_offsets[i].start_offset = offset;
			offset += (18 * CBMDOS_SECTOR_SIZE);
		}

		// Tracks 31-35: 17 sectors
		for (int i = 31 + track_base; i <= 35 + track_base; i++) {
			track_offsets[i].sector_count = 17;
			track_offsets[i].start_offset = offset;
			offset += (17 * CBMDOS_SECTOR_SIZE);
		}
	}
}

/**
 * Initialize track offsets for C8050. (77 tracks)
 * @param isC8250 If true, initialize for C8250. (154 tracks)
 */
void CBMDOSPrivate::init_track_offsets_C8050(bool isC8250)
{
	// C8050/C8250 zones:

	/// Side A
	// - Tracks  1-39: 29 sectors
	// - Tracks 40-53: 27 sectors
	// - Tracks 54-64: 25 sectors
	// - Tracks 65-77: 23 sectors

	/// Side B (C8250 only)
	// - Tracks  78-116: 29 sectors
	// - Tracks 117-130: 17 sectors
	// - Tracks 131-141: 25 sectors
	// - Tracks 142-154: 23 sectors
	unsigned int offset = 0;
	track_offsets.resize(isC8250 ? 154 : 77);

	int track_base = -1;	// track numbering starts at 1
	const unsigned int sides = (isC8250 ? 2 : 1);
	for (unsigned int side = 0; side < sides; side++, track_base += 77) {
		// Tracks 1-39: 29 sectors
		for (int i = 1 + track_base; i <= 39 + track_base; i++) {
			track_offsets[i].sector_count = 29;
			track_offsets[i].start_offset = offset;
			offset += (29 * CBMDOS_SECTOR_SIZE);
		}

		// Tracks 40-53: 27 sectors
		for (int i = 40 + track_base; i <= 53 + track_base; i++) {
			track_offsets[i].sector_count = 27;
			track_offsets[i].start_offset = offset;
			offset += (27 * CBMDOS_SECTOR_SIZE);
		}

		// Tracks 54-64: 25 sectors
		for (int i = 54 + track_base; i <= 64 + track_base; i++) {
			track_offsets[i].sector_count = 25;
			track_offsets[i].start_offset = offset;
			offset += (25 * CBMDOS_SECTOR_SIZE);
		}

		// Tracks 65-77: 23 sectors
		for (int i = 65 + track_base; i <= 77 + track_base; i++) {
			track_offsets[i].sector_count = 23;
			track_offsets[i].start_offset = offset;
			offset += (23 * CBMDOS_SECTOR_SIZE);
		}
	}
}

/**
 * Initialize track offsets for C1581. (80 tracks)
 */
void CBMDOSPrivate::init_track_offsets_C1581(void)
{
	// C1581 has 80 tracks, with 40 sectors per track.
	unsigned int offset = 0;
	track_offsets.resize(80);

	for (int i = 1-1; i <= 80-1; i++) {
		track_offsets[i].sector_count = 40;
		track_offsets[i].start_offset = offset;
		offset += (40 * CBMDOS_SECTOR_SIZE);
	}
}

/**
 * Initialize tracks for a G64/G71 (GCR-1541/GCR-1571) image. (up to 42 or 84 tracks)
 * @param header G64/G71 header
 */
void CBMDOSPrivate::init_track_offsets_G64(const cbmdos_G64_header_t *header)
{
	// G64: Up to 42 tracks. (84 half-tracks)
	// G71: Up to 84 tracks. (168 half-tracks)
	// NOTE: We'll use the value from the header if it's <= 84.
	// Odd counts will be rounded up.
	assert(header->track_count != 0);
	assert(header->track_count % 2 == 0);

	int track_count;
	if (header->magic[6] == '7') {
		// GCR-1571: Up to 84 tracks (168 half-tracks)
		assert(header->track_count <= 168U);
		track_count = std::min((uint8_t)168U, header->track_count);
	} else /*if (header->magic[6] == '4')*/ {
		// GCR-1541: Up to 42 tracks (84 half-tracks)
		assert(header->track_count <= 84U);
		track_count = std::min((uint8_t)84U, header->track_count);
	}
	track_count = (track_count / 2) + (track_count % 2);
	track_offsets.reserve(track_count);

	uint8_t sectors_this_track = 21;
	const uint32_t *src_offsets = header->track_offsets;
	bool found_any_tracks = false;
	for (int i = 1-1; i <= track_count-1; i++, src_offsets += 2) {
		if (*src_offsets == 0) {
			if (!found_any_tracks) {
				// Haven't found any tracks yet...
				// This track is missing from the disk!
				track_offsets_t this_track;
				this_track.sector_count = 0;
				this_track.start_offset = 0;
				track_offsets.push_back(this_track);
				continue;
			}

			// Finished reading tracks.
			break;
		}

		// Have we reached the next zone?
		switch (i) {
			//case 1-1:
			case 1+84-1:
				sectors_this_track = 21;
				break;
			case 18-1:
			case 18+84-1:
				sectors_this_track = 19;
				break;
			case 25-1:
			case 25+84-1:
				sectors_this_track = 18;
				break;
			case 31-1:
			case 31+84-1:
				sectors_this_track = 17;
				break;
			default:
				break;
		}

		// Save the track offset.
		track_offsets_t this_track;
		this_track.sector_count = sectors_this_track;
		this_track.start_offset = le32_to_cpu(*src_offsets);
		track_offsets.push_back(this_track);
		found_any_tracks = true;
	}
}

/**
 * Decode 5 GCR bytes into 4 data bytes.
 * @param data	[out] Output buffer for 4 data bytes
 * @param gcr	[in] Input buffer with 5 GCR bytes
 * @return 0 on success; non-zero on error.
 */
int CBMDOSPrivate::decode_GCR_bytes(uint8_t *data, const uint8_t *gcr)
{
	// Decode five bytes into four.
	// 11111222 22333334 44445555 56666677 77788888
	// GCR decode map:
	// - index: GCR 5-bit value
	// - value: Decoded 4-bit value
	// NOTE: Invalid values will be -1.
	static const int8_t gcr_decode_map[32] = {
		// GCR: 00000, 00001, 00010, 00011
		-1, -1, -1, -1,

		// GCR: 00100, 00101, 00110, 00111
		-1, -1, -1, -1,

		// GCR: 01000, 01001, 01010, 01011
		-1, 8, 0, 1,

		// GCR: 01100, 01101, 01110, 01111
		-1, 12, 4, 5,

		// GCR: 10000, 10001, 10010, 10011
		-1, -1, 2, 3,

		// GCR: 10100, 10101, 10110, 10111
		-1, 0xF, 6, 7,

		// GCR: 11000, 11001, 11010, 11011
		-1, 9, 0xA, 0xB,

		// GCR: 11100, 11101, 11110, 11111
		-1, 0xD, 0xE, -1,
	};

	// Combine five GCR bytes into a uint64_t.
	uint64_t gcr_data = ((uint64_t)gcr[0] << 32) |
	                    ((uint64_t)gcr[1] << 24) |
	                    ((uint64_t)gcr[2] << 16) |
	                    ((uint64_t)gcr[3] <<  8) |
	                     (uint64_t)gcr[4];

	// Decode GCR quintets into nybbles, backwards.
	for (int i = 3; i >= 0; i--) {
		// TODO: Check for errors.
		const uint8_t gcr_dec_2 = gcr_decode_map[gcr_data & 0x1FU] & 0x0FU;
		gcr_data >>= 5;
		const uint8_t gcr_dec_1 = gcr_decode_map[gcr_data & 0x1FU] & 0x0FU;
		gcr_data >>= 5;

		data[i] = (gcr_dec_1 << 4) | gcr_dec_2;
	}

	return 0;
}

/**
 * Read and decode a GCR track from the disk image.
 * This will be cached into GCR_track_buffer.
 * @param track	[in] Track# (starts at 1)
 * @return 0 on success; negative POSIX error code on error.
 */
int CBMDOSPrivate::read_GCR_track(uint8_t track)
{
	assert(diskType == DiskType::G64 || diskType == DiskType::G71);
	if (diskType != DiskType::G64 && diskType != DiskType::G71)
		return -EIO;

	assert(track != 0);
	assert(track <= track_offsets.size());
	if (track == 0 || track >= track_offsets.size())
		return -EINVAL;

	// Get the track offsets.
	const track_offsets_t this_track = track_offsets[track-1];

	// Read the GCR track. (usually 7,928; we'll allow up to 8,192)
	assert(GCR_track_size > 0);
	assert(GCR_track_size <= GCR_MAX_TRACK_SIZE);
	array<uint8_t, GCR_MAX_TRACK_SIZE> gcr_buf;
	size_t gcr_len = file->seekAndRead(this_track.start_offset, gcr_buf.data(), std::min((size_t)GCR_track_size, gcr_buf.size()));
	if (gcr_len == 0) {
		// Unable to read any GCR data...
		return -EIO;
	}

	// Make sure the GCR track buffer is allocated.
	if (!GCR_track_buffer) {
		GCR_track_buffer = new GCR_track_buffer_t;
	}

	// NOTE: C1541 normally writes 40 '1' bits (FF FF FF FF FF),
	// but the drive controller only requires 10 or 12 minimum.
	// We'll look for 16 '1' bits (FF FF).
	// (Monopoly.g64 has FF FF FF at 18/11.)

	// Attempt to read the total number of sectors in this track.
	// TODO: Return an error if we read less than what's expected.
	auto p = gcr_buf.begin();
	const auto p_end = gcr_buf.end();
	for (uint8_t sector = 0; sector < this_track.sector_count && p < p_end; sector++) {
		// TODO: Read bits, not bytes. Most G64s are byte-aligned, though...

		// C1541 GCR encodes four raw bytes into five encoded bytes.

		// Find the header sync. (at least 16 '1' bits, FF FF)
		uint8_t sync_count = 0;
		for (; sync_count < 2 && p < p_end; p++) {
			if (*p == 0xFF)
				sync_count++;
			else
				sync_count = 0;
		}
		if (sync_count < 2) {
			// Out of sync...
			// TODO: Return an error.
			break;
		}

		// Find the sector header. (10 GCR bytes, starts with $52 encoded)
		bool found_header = false;
		for (; p + 10 < p_end; p++) {
			if (*p != 0x52)
				continue;

			// Found the sector header.
			// TODO: Decode and verify?
			p += 10;
			found_header = true;
			break;
		}
		if (!found_header) {
			// Sector header not found...
			// TODO: Return an error.
			break;
		}

		// NOTE: There's supposed to be a header gap of 8 $55 bytes
		// (or 9 $55 bytes from early 1540/1541 ROMs), but some
		// disk images don't have it.

		// Find the data sync. (at least 16 '1' bits, FF FF)
		sync_count = 0;
		for (; sync_count < 2 && p < p_end; p++) {
			if (*p == 0xFF)
				sync_count++;
			else
				sync_count = 0;
		}
		if (sync_count < 2) {
			// Out of sync...
			// TODO: Return an error.
			break;
		}

		// Find the data block. (325 GCR bytes decodes to 260 data bytes, starts with $55 encoded)
		bool found_data = false;
		for (; p + 325 < p_end; p++) {
			if (*p != 0x55)
				continue;

			// Found the data header.
			// Decode the GCR data.
			cbmdos_GCR_data_block_t data;
			uint8_t *p_data = data.raw;
			uint8_t *const p_data_end = &data.raw[ARRAY_SIZE(data.raw)];
			for (; p_data < p_data_end; p += 5, p_data += 4) {
				// TODO: Return errors?
				decode_GCR_bytes(p_data, &(*p));
			}

			// Copy the data into the track buffer.
			memcpy(GCR_track_buffer->sectors[sector], data.data, CBMDOS_SECTOR_SIZE);

			found_data = true;
			break;
		}
		if (!found_data) {
			// Data block not found...
			// TODO: Return an error.
			break;
		}
	}

	// TODO
	return 0;
}

/**
 * Read a 256-byte sector given track/sector addresses.
 * @param buf		[out] Output buffer
 * @param siz		[in] Size of buf
 * @param track		[in] Track# (starts at 1)
 * @param sector	[in] Sector# (starts at 0)
 * @return Number of bytes read on success, or zero on error.
 */
size_t CBMDOSPrivate::read_sector(void *buf, size_t siz, uint8_t track, uint8_t sector)
{
	// Buffer must be exactly CBMDOS_SECTOR_SIZE bytes.
	assert(siz == CBMDOS_SECTOR_SIZE);
	if (siz != CBMDOS_SECTOR_SIZE)
		return 0;

	assert(track != 0);
	assert(track <= track_offsets.size());
	if (track == 0 || track >= track_offsets.size())
		return 0;

	// Get the track offsets.
	const track_offsets_t this_track = track_offsets[track-1];

	assert(sector < this_track.sector_count);
	if (sector >= this_track.sector_count)
		return 0;

	size_t ret;
	switch (diskType) {
		case DiskType::G64:
		case DiskType::G71:
			// GCR
			if (track != GCR_track_cache_number) {
				// Need to cache the track.
				// TODO: Check for errors.
				read_GCR_track(track);
			}
			if (!GCR_track_buffer) {
				// GCR track couldn't be loaded...
				return 0;
			}

			// Copy from the GCR track cache.
			memcpy(buf, &GCR_track_buffer->sectors[sector], siz);
			ret = siz;
			break;

		default:
			// Standard disk image
			// Get the absolute starting address.
			const off64_t start_pos = this_track.start_offset + (sector * CBMDOS_SECTOR_SIZE);

			// Read the sector.
			ret = file->seekAndRead(start_pos, buf, siz);
			break;
	}
	return ret;
}

/**
 * Remove $A0 padding from a character buffer.
 * @param buf	[in] Character buffer
 * @param siz	[in] Size of buf
 * @return String length with $A0 padding removed.
 */
size_t CBMDOSPrivate::remove_A0_padding(const char *buf, size_t siz)
{
	assert(siz != 0);
	if (siz == 0)
		return 0;

	buf += (siz - 1);
	for (; siz > 0; buf--, siz--) {
		if ((uint8_t)*buf != 0xA0)
			break;
	}

	return siz;
}

/** CBMDOS **/

/**
 * Read a Commodore DOS floppy disk image.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
CBMDOS::CBMDOS(const IRpFilePtr &file)
	: super(new CBMDOSPrivate(file))
{
	// This class handles disk images.
	RP_D(CBMDOS);
	d->fileType = FileType::DiskImage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Seek to the beginning of the file.
	d->file->rewind();

	// Read the disk header for G64/G71 detection.
	cbmdos_G64_header_t g64_header;
	size_t size = d->file->read(&g64_header, sizeof(g64_header));
	if (size < sizeof(g64_header)) {
		d->file.reset();
		return;
	}

	// Check if this disk image is supported.
	const off64_t filesize = d->file->size();
	const DetectInfo info = {
		{0, static_cast<uint32_t>(size), reinterpret_cast<const uint8_t*>(&g64_header)},
		nullptr,	// ext (TODO: May be needed?)
		filesize	// szFile
	};
	d->diskType = static_cast<CBMDOSPrivate::DiskType>(isRomSupported_static(&info));
	if (d->diskType <= CBMDOSPrivate::DiskType::Unknown) {
		d->file.reset();
		return;
	}

	// TODO: Other format images, and maybe validate the directory track?
	switch (d->diskType) {
		default:
			// Not supported...
			d->file.reset();
			return;

		case CBMDOSPrivate::DiskType::D64:
			// C1541 image (35 or 40 tracks, single-sided)
			d->dir_track = 18;
			d->dir_first_sector = 1;
			d->init_track_offsets_C1541(false);

			// Check for error bytes.
			if (filesize == (683 * CBMDOS_SECTOR_SIZE) + 683) {
				// 35-track C1541 image, with error bytes
				d->err_bytes_count = 683;
				d->err_bytes_offset = (683 * CBMDOS_SECTOR_SIZE);
			} else if (filesize == (768 * CBMDOS_SECTOR_SIZE) + 768) {
				// 40-track C1541 image, with error bytes
				d->err_bytes_count = 768;
				d->err_bytes_offset = (768 * CBMDOS_SECTOR_SIZE);
			}
			break;

		case CBMDOSPrivate::DiskType::D71:
			// C1571 image (35 tracks, double-sided; 70 tracks total)
			d->dir_track = 18;
			d->dir_first_sector = 1;
			d->init_track_offsets_C1571();

			// Check for error bytes.
			if (filesize == (1366 * CBMDOS_SECTOR_SIZE) + 1366) {
				// 70-track C1571 image, with error bytes
				d->err_bytes_count = 1366;
				d->err_bytes_offset = (1366 * CBMDOS_SECTOR_SIZE);
			}
			break;

		case CBMDOSPrivate::DiskType::D80:
			// C8050 image (77 tracks, single-sided)
			d->dir_track = 39;
			d->dir_first_sector = 1;
			d->init_track_offsets_C8050(false);
			break;
		case CBMDOSPrivate::DiskType::D82:
			// C8250 image (77 tracks, double-sided; 154 tracks total)
			d->dir_track = 39;
			d->dir_first_sector = 1;
			d->init_track_offsets_C8050(true);
			break;

		case CBMDOSPrivate::DiskType::D81:
			// C1581 image (80 tracks, double-sided)
			d->dir_track = 40;
			d->dir_first_sector = 3;
			d->init_track_offsets_C1581();

			// Check for error bytes.
			if (filesize == (3200 * CBMDOS_SECTOR_SIZE) + 3200) {
				// 80-track C1581 image, with error bytes
				d->err_bytes_count = 3200;
				d->err_bytes_offset = (3200 * CBMDOS_SECTOR_SIZE);
			}
			break;

		case CBMDOSPrivate::DiskType::D67:
			// C2040 image (35 or 40 tracks, single-sided)
			// NOTE: DOS 1.x; similar to C1541, except speed zone 2 has 20 sectors instead of 19.
			d->dir_track = 18;
			d->dir_first_sector = 1;
			d->init_track_offsets_C1541(true);

			// Check for error bytes.
			if (filesize == (690 * CBMDOS_SECTOR_SIZE) + 690) {
				// 35-track C2040 image, with error bytes
				d->err_bytes_count = 690;
				d->err_bytes_offset = (690 * CBMDOS_SECTOR_SIZE);
			} else if (filesize == (775 * CBMDOS_SECTOR_SIZE) + 775) {
				// 40-track C2040 image, with error bytes
				d->err_bytes_count = 775;
				d->err_bytes_offset = (775 * CBMDOS_SECTOR_SIZE);
			}
			break;

		case CBMDOSPrivate::DiskType::G64:
		case CBMDOSPrivate::DiskType::G71:
			// C1541/C1571 image, GCR-encoded.
			// TODO: Save g64_header?

			d->GCR_track_size = le16_to_cpu(g64_header.track_size);
			if (d->GCR_track_size == 0 || d->GCR_track_size > GCR_MAX_TRACK_SIZE) {
				// Track size is out of range.
				d->file.reset();
				return;
			}

			d->dir_track = 18;
			d->dir_first_sector = 1;
			d->init_track_offsets_G64(&g64_header);
			break;
	}

	// Get the BAM/header sector. (sector 0 of the directory track)
	size = d->read_sector(&d->diskHeader, sizeof(d->diskHeader), d->dir_track, 0);
	if (size != sizeof(d->diskHeader)) {
		// Read error.
		d->file.reset();
		return;
	}

	// This is avalid CBM DOS disk image.
	d->mimeType = d->mimeTypes[(int)d->diskType];
	d->isValid = true;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int CBMDOS::isRomSupported_static(const DetectInfo *info)
{
	// NOTE: Most of the Dxx images have no magic number.
	// Assuming this image is valid if it has the correct filesize
	// for one of the supported disk image formats.
	switch (info->szFile) {
		case (683 * CBMDOS_SECTOR_SIZE) + 683:
		case (683 * CBMDOS_SECTOR_SIZE):
		case (768 * CBMDOS_SECTOR_SIZE) + 768:
		case (768 * CBMDOS_SECTOR_SIZE):
			// C1541 disk image
			return static_cast<int>(CBMDOSPrivate::DiskType::D64);

		case (1366 * CBMDOS_SECTOR_SIZE) + 1366:
		case (1366 * CBMDOS_SECTOR_SIZE):
			// C1571 disk image (double-sided)
			return static_cast<int>(CBMDOSPrivate::DiskType::D71);

		case (2083 * CBMDOS_SECTOR_SIZE):
			// C8050 disk image (single-sided)
			return static_cast<int>(CBMDOSPrivate::DiskType::D80);
		case (4166 * CBMDOS_SECTOR_SIZE):
			// C8250 disk image (double-sided)
			return static_cast<int>(CBMDOSPrivate::DiskType::D82);

		case (3200 * CBMDOS_SECTOR_SIZE) + 3200:
		case (3200 * CBMDOS_SECTOR_SIZE):
			// C1581 disk image
			return static_cast<int>(CBMDOSPrivate::DiskType::D81);

		case (690 * CBMDOS_SECTOR_SIZE) + 690:
		case (690 * CBMDOS_SECTOR_SIZE):
		case (775 * CBMDOS_SECTOR_SIZE) + 775:
		case (775 * CBMDOS_SECTOR_SIZE):
			// C2040 disk image
			return static_cast<int>(CBMDOSPrivate::DiskType::D67);

		default:
			break;
	}

	// Check for G64/G71.
	if (info->header.addr == 0 && info->header.size >= sizeof(cbmdos_G64_header_t)) {
		const cbmdos_G64_header_t *pHeader = reinterpret_cast<const cbmdos_G64_header_t*>(info->header.pData);
		if (!memcmp(pHeader->magic, CBMDOS_G64_MAGIC, sizeof(pHeader->magic))) {
			// This is a G64 image.
			return static_cast<int>(CBMDOSPrivate::DiskType::G64);
		} else if (!memcmp(pHeader->magic, CBMDOS_G71_MAGIC, sizeof(pHeader->magic))) {
			// This is a G71 image.
			return static_cast<int>(CBMDOSPrivate::DiskType::G71);
		}
	}

	// No match.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *CBMDOS::systemName(unsigned int type) const
{
	RP_D(const CBMDOS);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// CBMDOS has the same name worldwide, so we can
	// ignore the region selection.
	// TODO: Identify the OS, or list that in the fields instead?
	static_assert(SYSNAME_TYPE_MASK == 3,
		"CBMDOS::systemName() array index optimization needs to be updated.");

	// TODO: More types.
	static const char *const sysNames[8][4] = {
		{"Commodore 1541", "C1541", "C1541", nullptr},
		{"Commodore 1571", "C1571", "C1571", nullptr},
		{"Commodore 8050", "C8050", "C8050", nullptr},
		{"Commodore 8250", "C8250", "C8250", nullptr},
		{"Commodore 1581", "C1581", "C1581", nullptr},
		{"Commodore 2040", "C2040", "C2040", nullptr},

		{"Commodore 1541 (GCR)", "C1541 (GCR)", "C1541 (GCR)", nullptr},
		{"Commodore 1571 (GCR)", "C1571 (GCR)", "C1571 (GCR)", nullptr},
	};

	unsigned int sysID = 0;
	if ((int)d->diskType >= 0 && d->diskType < CBMDOSPrivate::DiskType::Max) {
		sysID = (int)d->diskType;
	}
	return sysNames[sysID][type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int CBMDOS::loadFieldData(void)
{
	RP_D(CBMDOS);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unsupported file.
		return -EIO;
	}

	// TODO: Selectable unshifted vs. shifted PETSCII conversion. Using unshifted for now.
	// TODO: Reverse video?
	static const char uFFFD[] = "\xEF\xBF\xBD";
	unsigned int codepage = CP_RP_PETSCII_Unshifted;

	// Disk BAM/header is read in the constructor.
	const auto *diskHeader = &d->diskHeader;
	d->fields.reserve(4);	// Maximum of 4 fields.

	// Get the string addresses from the BAM/header sector.
	// TODO: Separate function for this?
	const char *disk_name, *disk_id, *dos_type;
	size_t disk_name_len;

	switch (d->diskType) {
		case CBMDOSPrivate::DiskType::D64:
		case CBMDOSPrivate::DiskType::D71:
		case CBMDOSPrivate::DiskType::D67:
		case CBMDOSPrivate::DiskType::G64:
		case CBMDOSPrivate::DiskType::G71:
			// C1541, C1571, C2040
			disk_name = diskHeader->c1541.disk_name;
			disk_name_len = sizeof(diskHeader->c1541.disk_name);
			disk_id = diskHeader->c1541.disk_id;
			dos_type = diskHeader->c1541.dos_type;
			break;

		case CBMDOSPrivate::DiskType::D80:
		case CBMDOSPrivate::DiskType::D82:
			// C8050/C8250
			disk_name = diskHeader->c8050.disk_name;
			disk_name_len = sizeof(diskHeader->c8050.disk_name);
			disk_id = diskHeader->c8050.disk_id;
			dos_type = diskHeader->c8050.dos_type;
			break;

		case CBMDOSPrivate::DiskType::D81:
			// C1581
			disk_name = diskHeader->c1581.disk_name;
			disk_name_len = sizeof(diskHeader->c1581.disk_name);
			disk_id = diskHeader->c1581.disk_id;
			dos_type = diskHeader->c1581.dos_type;
			break;

		default:
			assert(!"Unsupported CBM disk type?");
			return 0;
	}

	// Disk name
	const char *const s_disk_name_title = C_("CBMDOS", "Disk Name");
	disk_name_len = d->remove_A0_padding(disk_name, disk_name_len);
	if (unlikely(!memcmp(diskHeader->c1541.geos.geos_id_string, "GEOS", 4))) {
		// GEOS ID is present. Parse the disk name as ASCII. (well, Latin-1)
		d->fields.addField_string(s_disk_name_title,
			latin1_to_utf8(disk_name, disk_name_len));
	} else {
		string s_disk_name = cpN_to_utf8(codepage, disk_name, disk_name_len);
		if (s_disk_name.find(uFFFD) != string::npos) {
			// Disk name has invalid characters when using Unshifted.
			// Try again with Shifted.
			codepage = CP_RP_PETSCII_Shifted;
			s_disk_name = cpN_to_utf8(codepage, disk_name, disk_name_len);
		}
		d->fields.addField_string(s_disk_name_title, s_disk_name);
	}

	// Disk ID
	d->fields.addField_string(C_("CBMDOS", "Disk ID"),
		cpN_to_utf8(codepage, disk_id, sizeof(diskHeader->c1541.disk_id)));

	// DOS Type (NOTE: Always unshifted)
	d->fields.addField_string(C_("CBMDOS", "DOS Type"),
		cpN_to_utf8(CP_RP_PETSCII_Unshifted, dos_type, sizeof(diskHeader->c1541.dos_type)));

	// C1581 has an additional file type, "CBM".
	const uint8_t max_file_type = (d->diskType == CBMDOSPrivate::DiskType::D81) ? 6 : 5;

	// Make sure the directory track number is valid.
	assert((size_t)d->dir_track-1 < d->track_offsets.size());
	if ((size_t)d->dir_track-1 >= d->track_offsets.size()) {
		// Unable to read the directory track...
		// TODO: Show an error?
		return static_cast<int>(d->fields.count());
	}

	// Read the directory.
	// NOTE: Ignoring the directory location in the BAM sector,
	// since it might be incorrect. Assuming dir_track/dir_first_sector.
	bitset<64> sectors_read(1);	// Sector 0 is not allowed here, so mark it as 'read'.

	vector<vector<string> > *const vv_dir = new vector<vector<string> >();
	auto vv_icons = new RomFields::ListDataIcons_t;	// for GEOS files only
	bool has_icons = false;

	const unsigned int sector_count = d->track_offsets[d->dir_track-1].sector_count;
	for (unsigned int i = d->dir_first_sector; i < sector_count && !sectors_read.test(i); ) {
		cbmdos_dir_sector_t entries;
		size_t size = d->read_sector(&entries, sizeof(entries), d->dir_track, i);
		if (size != sizeof(entries))
			break;

		// Update the next sector entry before processing any entries.
		sectors_read.set(i);
		if (entries.entry[0].next.track == d->dir_track) {
			i = entries.entry[0].next.sector;
		} else {
			// No more directory sectors after this one.
			i = 0;
		}

		const cbmdos_dir_entry_t *const p_end = &entries.entry[ARRAY_SIZE(entries.entry)];
		for (cbmdos_dir_entry_t *p_dir = entries.entry; p_dir < p_end; p_dir++) {
			// File type 0 ("*DEL") indicates an empty directory entry.
			// TODO: Also check filename to see if it's a "scratched" file?
			if (p_dir->file_type == 0)
				continue;

			vv_dir->resize(vv_dir->size() + 1);
			auto &p_list = vv_dir->at(vv_dir->size() - 1);

			// Directory listing as seen on a C64:
			// - # of blocks
			// - Filename
			// - File type

			// Determine if this is a GEOS file.
			bool is_geos_file = false;
			const uint8_t file_type = (p_dir->file_type & CBMDOS_FileType_Mask);
			// GEOS files can only be DEL, SEQ, PRG, or USR.
			if (file_type < CBMDOS_FileType_REL) {
				// GEOS file type and file structure cannot both be 0.
				if (p_dir->geos.file_type != 0 || p_dir->geos.file_structure != 0) {
					if (p_dir->geos.file_structure <= GEOS_FILE_STRUCTURE_VLIR) {
						// This is a GEOS file.
						is_geos_file = true;
					}
				}
			}

			// # of blocks (filesize)
			char filesize[16];
			snprintf(filesize, sizeof(filesize), "%u", le16_to_cpu(p_dir->sector_count));
			p_list.emplace_back(filesize);

			// Filename
			const size_t filename_len = d->remove_A0_padding(p_dir->filename, sizeof(p_dir->filename));
			if (unlikely(is_geos_file)) {
				// GEOS file: The filename is encoded as ASCII.
				// NOTE: Using Latin-1...
				p_list.emplace_back(latin1_to_utf8(p_dir->filename, filename_len));
			} else {
				string s_filename = cpN_to_utf8(codepage, p_dir->filename, filename_len);
				if (codepage == CP_RP_PETSCII_Unshifted && s_filename.find(uFFFD) != string::npos) {
					// File name has invalid characters when using Unshifted.
					// Try again with Shifted.
					codepage = CP_RP_PETSCII_Shifted;
					s_filename = cpN_to_utf8(codepage, p_dir->filename, filename_len);
				}
				p_list.emplace_back(std::move(s_filename));
			}

			// File type
			string s_file_type;

			// Splat files are indicated with a *preceding* '*'.
			if (!(p_dir->file_type & CBMDOS_FileType_Closed)) {
				s_file_type = "*";
			}

			// Actual file type
			static const char file_type_tbl[6][4] = {
				"DEL", "SEQ", "PRG", "USR",
				"REL", "CBM",
			};
			if (file_type < max_file_type) {
				s_file_type += file_type_tbl[file_type];
			} else {
				// Print the numeric value instead.
				char buf[16];
				snprintf(buf, sizeof(buf), "%u", file_type);
				s_file_type += buf;
			}

			// Append the other flags, if set.
			if (p_dir->file_type & CBMDOS_FileType_SaveReplace) {
				s_file_type += '@';
			}
			if (p_dir->file_type & CBMDOS_FileType_Locked) {
				s_file_type += '>';
			}
			p_list.emplace_back(std::move(s_file_type));

			// If this is a GEOS file, get the icon.
			rp_image_ptr icon;
			if (unlikely(is_geos_file) &&
			    likely(p_dir->geos.file_type != GEOS_FILE_TYPE_NON_GEOS) &&
			    likely(p_dir->geos.info_addr.track != 0))
			{
				// Read the information sector.
				cbmdos_GEOS_info_block_t geos_info;
				size = d->read_sector(&geos_info, sizeof(geos_info), p_dir->geos.info_addr.track ,p_dir->geos.info_addr.sector);
				if (size == sizeof(geos_info)) {
					icon = ImageDecoder::fromLinearMono(24, 21, geos_info.icon, sizeof(geos_info.icon));
				}
			}

			if (icon) {
				has_icons = true;
			}
			vv_icons->emplace_back(std::move(icon));
		}
	}

	static const char *const dir_headers[] = {
		NOP_C_("CBMDOS|Directory", "Blocks"),
		NOP_C_("CBMDOS|Directory", "Filename"),
		NOP_C_("CBMDOS|Directory", "Type"),
	};
	vector<string> *const v_dir_headers = RomFields::strArrayToVector_i18n(
		"CBMDOS|Directory", dir_headers, ARRAY_SIZE(dir_headers));

	RomFields::AFLD_PARAMS params(has_icons ? (unsigned int)RomFields::RFT_LISTDATA_ICONS : 0, 8);
	params.headers = v_dir_headers;
	params.data.single = vv_dir;
	params.col_attrs.align_headers	= AFLD_ALIGN3(TXA_D, TXA_D, TXA_D);
	params.col_attrs.align_data	= AFLD_ALIGN3(TXA_R, TXA_D, TXA_D);
	params.col_attrs.sizing		= AFLD_ALIGN3(COLSZ_R, COLSZ_S, COLSZ_R);
	params.col_attrs.sorting	= AFLD_ALIGN3(COLSORT_NUM, COLSORT_STD, COLSORT_STD);
	params.col_attrs.sort_col	= -1;	// no sorting by default; show files as listed on disk
	params.col_attrs.sort_dir	= RomFields::COLSORTORDER_ASCENDING;

	if (has_icons) {
		params.mxd.icons = vv_icons;
	} else {
		// Not using the icons vector.
		delete vv_icons;
	}

	d->fields.addField_listData(C_("CBMDOS", "Directory"), &params);

	// Check for a C128 autoboot sector.
	if (d->diskType == CBMDOSPrivate::DiskType::D64 ||
	    d->diskType == CBMDOSPrivate::DiskType::D71)
	{
		cbmdos_C128_autoboot_sector_t autoboot;
		size_t size = d->read_sector(&autoboot, sizeof(autoboot), 1, 0);
		if (size == sizeof(autoboot) && !memcmp(autoboot.signature, "CBM", 3)) {
			// We have an autoboot sector.
			// TODO: Show other fields?

			// Messages
			autoboot.messages[ARRAY_SIZE(autoboot.messages)-1] = 0;	// ensure NULL termination
			const char *const p_msg_start = autoboot.messages;
			const char *const p_msg_end = &autoboot.messages[ARRAY_SIZE(autoboot.messages)];

			// Find the message offsets.
			const char *p_boot_msg = p_msg_start;
			const char *p_boot_prg;
			if (*p_boot_msg == '\0') {
				// No custom boot message. Use the default.
				p_boot_prg = p_boot_msg + 1;
				p_boot_msg = nullptr;
			} else {
				// Find the first NULL after the boot message.
				// This will be right before the start of the boot program name.
				p_boot_prg = static_cast<const char*>(memchr(p_boot_msg, '\0', p_msg_end - p_boot_msg));
				if (p_boot_prg && (p_boot_prg + 1 < p_msg_end)) {
					p_boot_prg++;
				}
			}

			// Track/sector to load from
			if (autoboot.addl_sectors.track != 0 && autoboot.addl_sectors.sector != 0) {
				d->fields.addField_string(C_("CBMDOS", "C128 boot T/S"),
					rp_sprintf("%u/%u", autoboot.addl_sectors.track, autoboot.addl_sectors.sector));
				// Bank
				d->fields.addField_string_numeric(C_("CBMDOS", "C128 boot bank"), autoboot.bank);
				// Load count
				d->fields.addField_string_numeric(C_("CBMDOS", "C128 boot load count"), autoboot.load_count);
			}

			// Boot message
			// NOTE: Assuming unshifted, since the system starts unshifted.
			const char *const s_title_boot_msg = C_("CBMDOS", "C128 boot message");
			if (p_boot_msg) {
				d->fields.addField_string(s_title_boot_msg,
					cpN_to_utf8(CP_RP_PETSCII_Unshifted, p_boot_msg, -1));
			} else {
				d->fields.addField_string(s_title_boot_msg, "BOOTING...");
			}

			// Boot program
			if (p_boot_prg && *p_boot_prg != '\0') {
				d->fields.addField_string(C_("CBMDOS", "C128 boot program"),
					cpN_to_utf8(codepage, p_boot_prg, -1));
			}
		}
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int CBMDOS::loadMetaData(void)
{
	RP_D(CBMDOS);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unsupported file.
		return -EIO;
	}

	// TODO: Selectable unshifted vs. shifted PETSCII conversion. Using unshifted for now.
	// TODO: Reverse video?
	static const char uFFFD[] = "\xEF\xBF\xBD";
	unsigned int codepage = CP_RP_PETSCII_Unshifted;

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// ROM header is read in the constructor.
	const auto *diskHeader = &d->diskHeader;

	// Get the string addresses from the BAM/header sector.
	// TODO: Separate function for this?
	const char *disk_name;
	size_t disk_name_len;

	switch (d->diskType) {
		case CBMDOSPrivate::DiskType::D64:
		case CBMDOSPrivate::DiskType::D71:
		case CBMDOSPrivate::DiskType::D67:
		case CBMDOSPrivate::DiskType::G64:
		case CBMDOSPrivate::DiskType::G71:
			// C1541, C1571, C2040
			disk_name = diskHeader->c1541.disk_name;
			disk_name_len = sizeof(diskHeader->c1541.disk_name);
			break;

		case CBMDOSPrivate::DiskType::D80:
		case CBMDOSPrivate::DiskType::D82:
			// C8050/C8250
			disk_name = diskHeader->c8050.disk_name;
			disk_name_len = sizeof(diskHeader->c8050.disk_name);
			break;

		case CBMDOSPrivate::DiskType::D81:
			// C1581
			disk_name = diskHeader->c1581.disk_name;
			disk_name_len = sizeof(diskHeader->c1581.disk_name);
			break;

		default:
			assert(!"Unsupported CBM disk type?");
			return 0;
	}

	// Title (disk name)
	disk_name_len = d->remove_A0_padding(disk_name, disk_name_len);
	if (unlikely(!memcmp(diskHeader->c1541.geos.geos_id_string, "GEOS", 4))) {
		// GEOS ID is present. Parse the disk name as ASCII. (well, Latin-1)
		d->metaData->addMetaData_string(Property::Title,
			latin1_to_utf8(disk_name, disk_name_len));
	} else {
		string s_disk_name = cpN_to_utf8(codepage, disk_name, disk_name_len);
		if (s_disk_name.find(uFFFD) != string::npos) {
			// Disk name has invalid characters when using Unshifted.
			// Try again with Shifted.
			codepage = CP_RP_PETSCII_Shifted;
			s_disk_name = cpN_to_utf8(codepage, disk_name, disk_name_len);
		}
		d->metaData->addMetaData_string(Property::Title, s_disk_name);
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

}
