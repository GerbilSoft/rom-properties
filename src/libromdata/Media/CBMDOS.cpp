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
// - https://area51.dev/c64/cbmdos/autoboot/

#include "stdafx.h"
#include "CBMDOS.hpp"

#include "cbmdos_structs.h"

// Other rom-properties libraries
#include "libi18n/i18n.h"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
#include <bitset>
using std::bitset;
using std::string;
using std::vector;

namespace LibRomData {

class CBMDOSPrivate final : public RomDataPrivate
{
public:
	CBMDOSPrivate(const IRpFilePtr &file);

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

		Max
	};
	DiskType diskType;

	// Track count
	// Usually 35 for a standard C1541 disk; can be up to 40.
	// Usually 70 for a double-sided C1571 disk.
	uint8_t track_count;

	// Directory track
	// Usually 18 for C1541/C1571 disks.
	uint8_t dir_track;

	// First directory sector
	// Usually 1, but may be 3 for C1581.
	uint8_t dir_first_sector;

	// Error bytes info (for certain D64/D71 format images)
	unsigned int err_bytes_count;
	unsigned int err_bytes_offset;

public:
	// Track offsets
	// Index is track number, minus one.
	struct track_offsets_t {
		uint8_t sector_count;	// Sectors per track
		uint16_t start_sector;	// Starting sector
	};
	track_offsets_t track_offsets[154];

	/**
	 * Initialize track offsets for C1541. (35/40 tracks)
	 */
	void init_track_offsets_C1541(void);

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
	 * Initialize track offsets for C8050. (80 tracks)
	 */
	void init_track_offsets_C1581(void);

public:
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
	 * @param buf	[in/out] Character buffer
	 * @param siz	[in] Size of buf
	 */
	void remove_A0_padding(char *buf, size_t siz);
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
	// TODO: More?

	nullptr
};
const char *const CBMDOSPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-d64",
	"application/x-d71",
	"application/x-d80",
	"application/x-d82",
	"application/x-d81",

	nullptr
};
const RomDataInfo CBMDOSPrivate::romDataInfo = {
	"CBMDOS", exts, mimeTypes
};

/**
 * Initialize track offsets for C1541. (35/40 tracks)
 */
void CBMDOSPrivate::init_track_offsets_C1541(void)
{
	// C1541 zones:
	// - Tracks  1-17: 21 sectors
	// - Tracks 18-24: 19 sectors
	// - Tracks 25-30: 18 sectors
	// - Tracks 31-40: 17 sectors
	unsigned int sector = 0;

	// Tracks 1-17: 21 sectors
	for (unsigned int i = 1-1; i <= 17-1; i++) {
		track_offsets[i].sector_count = 21;
		track_offsets[i].start_sector = sector;
		sector += 21;
	}

	// Tracks 18-24: 19 sectors
	for (unsigned int i = 18-1; i <= 24-1; i++) {
		track_offsets[i].sector_count = 19;
		track_offsets[i].start_sector = sector;
		sector += 19;
	}

	// Tracks 25-30: 18 sectors
	for (unsigned int i = 25-1; i <= 30-1; i++) {
		track_offsets[i].sector_count = 18;
		track_offsets[i].start_sector = sector;
		sector += 18;
	}

	// Tracks 31-40: 17 sectors
	for (unsigned int i = 31-1; i <= 40-1; i++) {
		track_offsets[i].sector_count = 17;
		track_offsets[i].start_sector = sector;
		sector += 17;
	}

	// Zero out the rest of the tracks.
	for (unsigned int i = 41-1; i < ARRAY_SIZE(track_offsets); i++) {
		track_offsets[i].sector_count = 0;
		track_offsets[i].start_sector = 0;
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
	unsigned int sector = 0;

	int track_base = -1;	// track numbering starts at 1
	for (unsigned int side = 0; side < 2; side++, track_base += 35) {
		// Tracks 1-17: 21 sectors
		for (int i = 1 + track_base; i <= 17 + track_base; i++) {
			track_offsets[i].sector_count = 21;
			track_offsets[i].start_sector = sector;
			sector += 21;
		}

		// Tracks 18-24: 19 sectors
		for (int i = 18 + track_base; i <= 24 + track_base; i++) {
			track_offsets[i].sector_count = 19;
			track_offsets[i].start_sector = sector;
			sector += 19;
		}

		// Tracks 25-30: 18 sectors
		for (int i = 25 + track_base; i <= 30 + track_base; i++) {
			track_offsets[i].sector_count = 18;
			track_offsets[i].start_sector = sector;
			sector += 18;
		}

		// Tracks 31-35: 17 sectors
		for (int i = 31 + track_base; i <= 35 + track_base; i++) {
			track_offsets[i].sector_count = 17;
			track_offsets[i].start_sector = sector;
			sector += 17;
		}
	}

	// Zero out the rest of the tracks.
	for (int i = track_base + 1; i < ARRAY_SIZE_I(track_offsets); i++) {
		track_offsets[i].sector_count = 0;
		track_offsets[i].start_sector = 0;
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
	unsigned int sector = 0;

	int track_base = -1;	// track numbering starts at 1
	const unsigned int sides = (isC8250 ? 2 : 1);
	for (unsigned int side = 0; side < sides; side++, track_base += 77) {
		// Tracks 1-39: 29 sectors
		for (int i = 1 + track_base; i <= 39 + track_base; i++) {
			track_offsets[i].sector_count = 29;
			track_offsets[i].start_sector = sector;
			sector += 29;
		}

		// Tracks 40-53: 27 sectors
		for (int i = 40 + track_base; i <= 53 + track_base; i++) {
			track_offsets[i].sector_count = 27;
			track_offsets[i].start_sector = sector;
			sector += 27;
		}

		// Tracks 54-64: 25 sectors
		for (int i = 54 + track_base; i <= 64 + track_base; i++) {
			track_offsets[i].sector_count = 25;
			track_offsets[i].start_sector = sector;
			sector += 25;
		}

		// Tracks 65-77: 23 sectors
		for (int i = 65 + track_base; i <= 77 + track_base; i++) {
			track_offsets[i].sector_count = 23;
			track_offsets[i].start_sector = sector;
			sector += 23;
		}
	}

	// Zero out the rest of the tracks.
	for (int i = track_base + 1; i < ARRAY_SIZE_I(track_offsets); i++) {
		track_offsets[i].sector_count = 0;
		track_offsets[i].start_sector = 0;
	}
}

/**
 * Initialize track offsets for C8050. (80 tracks)
 */
void CBMDOSPrivate::init_track_offsets_C1581(void)
{
	// C1581 has 80 tracks, with 40 sectors per track.
	unsigned int sector = 0;
	for (int i = 1-1; i <= 80-1; i++) {
		track_offsets[i].sector_count = 40;
		track_offsets[i].start_sector = sector;
		sector += 40;
	}

	// Zero out the rest of the tracks.
	for (int i = 81-1; i < ARRAY_SIZE_I(track_offsets); i++) {
		track_offsets[i].sector_count = 0;
		track_offsets[i].start_sector = 0;
	}
}

CBMDOSPrivate::CBMDOSPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, diskType(DiskType::Unknown)
	, track_count(0)
	, dir_track(0)
	, dir_first_sector(0)
	, err_bytes_count(0)
	, err_bytes_offset(0)
{}

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
	assert(track <= track_count);
	if (track == 0 || track >= track_count)
		return 0;

	// Get the track offsets.
	const track_offsets_t this_track = track_offsets[track-1];

	assert(sector < this_track.sector_count);
	if (sector >= this_track.sector_count)
		return 0;

	// Get the absolute starting address.
	// TODO: Adjust for GCR formats.
	const off64_t start_pos = (this_track.start_sector + sector) * CBMDOS_SECTOR_SIZE;

	// Read the sector.
	return file->seekAndRead(start_pos, buf, siz);
}

/**
 * Remove $A0 padding from a character buffer.
 * @param buf	[in/out] Character buffer
 * @param siz	[in] Size of buf
 */
void remove_A0_padding(char *buf, size_t siz)
{
	assert(siz != 0);
	if (siz == 0)
		return;

	buf += (siz - 1);
	for (; siz > 0; buf--, siz--) {
		if ((uint8_t)*buf == 0xA0) {
			*buf = 0;
		} else {
			break;
		}
	}
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
	d->mimeType = "application/x-d64";	// unofficial [TODO: Others?]
	d->fileType = FileType::DiskImage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// NOTE: There's no magic number here.
	// Assuming this image is valid if it has the correct filesize
	// for either a 35-track or 40-track C1541 disk image.
	// TODO: Other format images, and maybe validate the directory track?
	// TODO: Use isRomSupported_static() for the diskType.
	const off64_t filesize = d->file->size();
	switch (filesize) {
		case (683 * CBMDOS_SECTOR_SIZE):
			// 35-track C1541 image
			d->diskType = CBMDOSPrivate::DiskType::D64;
			d->track_count = 35;
			d->dir_track = 18;
			d->dir_first_sector = 1;
			d->init_track_offsets_C1541();
			break;
		case (683 * CBMDOS_SECTOR_SIZE) + 683:
			// 35-track C1541 image, with error bytes
			d->diskType = CBMDOSPrivate::DiskType::D64;
			d->track_count = 35;
			d->dir_track = 18;
			d->dir_first_sector = 1;
			d->init_track_offsets_C1541();
			d->err_bytes_count = 683;
			d->err_bytes_offset = (683 * CBMDOS_SECTOR_SIZE);
			break;

		case (768 * CBMDOS_SECTOR_SIZE):
			// 40-track C1541 image
			d->diskType = CBMDOSPrivate::DiskType::D64;
			d->track_count = 40;
			d->dir_track = 18;
			d->dir_first_sector = 1;
			d->init_track_offsets_C1541();
			break;
		case (768 * CBMDOS_SECTOR_SIZE) + 768:
			// 40-track C1541 image, with error bytes
			d->diskType = CBMDOSPrivate::DiskType::D64;
			d->track_count = 40;
			d->dir_track = 18;
			d->dir_first_sector = 1;
			d->init_track_offsets_C1541();
			d->err_bytes_count = 768;
			d->err_bytes_offset = (768 * CBMDOS_SECTOR_SIZE);
			break;

		case (1366 * CBMDOS_SECTOR_SIZE):
			// 70-track C1571 image
			d->diskType = CBMDOSPrivate::DiskType::D71;
			d->track_count = 70;
			d->dir_track = 18;
			d->dir_first_sector = 1;
			d->init_track_offsets_C1571();
			break;
		case (1366 * CBMDOS_SECTOR_SIZE) + 1366:
			// 70-track C1571 image, with error bytes
			d->diskType = CBMDOSPrivate::DiskType::D71;
			d->track_count = 70;
			d->dir_track = 18;
			d->dir_first_sector = 1;
			d->init_track_offsets_C1571();
			d->err_bytes_count = 1366;
			d->err_bytes_offset = (1366 * CBMDOS_SECTOR_SIZE);
			break;

		case (2083 * CBMDOS_SECTOR_SIZE):
			// 77-track C8050 image
			d->diskType = CBMDOSPrivate::DiskType::D80;
			d->track_count = 77;
			d->dir_track = 39;
			d->dir_first_sector = 1;
			d->init_track_offsets_C8050(false);
			break;
		case (4166 * CBMDOS_SECTOR_SIZE):
			// 154-track C8250 image
			d->diskType = CBMDOSPrivate::DiskType::D82;
			d->track_count = 154;
			d->dir_track = 39;
			d->dir_first_sector = 1;
			d->init_track_offsets_C8050(true);
			break;

		case (3200 * CBMDOS_SECTOR_SIZE):
			// 80-track C1581 image
			d->diskType = CBMDOSPrivate::DiskType::D81;
			d->track_count = 80;
			d->dir_track = 40;
			d->dir_first_sector = 3;
			d->init_track_offsets_C1581();
			break;
		case (3200 * CBMDOS_SECTOR_SIZE) + 3200:
			// 80-track C1581 image, with error bytes
			d->diskType = CBMDOSPrivate::DiskType::D81;
			d->track_count = 80;
			d->dir_track = 40;
			d->dir_first_sector = 3;
			d->init_track_offsets_C1581();
			d->err_bytes_count = 3200;
			d->err_bytes_offset = (3200 * CBMDOS_SECTOR_SIZE);
			break;

		default:
			// Not supported.
			d->file.reset();
			return;
	}

	// This is a valid CBM DOS disk image.
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
	// NOTE: Only checking for supported file extensions.
	assert(info->ext != nullptr);
	if (!info->ext) {
		// No file extension specified...
		return -1;
	}

	// TODO: Also check file sizes?
	for (const char *const *ext = CBMDOSPrivate::exts;
	     *ext != nullptr; ext++)
	{
		if (!strcasecmp(info->ext, *ext)) {
			// Found a match.
			return 0;
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
	static const char *const sysNames[5][4] = {
		{"Commodore 1541", "C1541", "C1541", nullptr},
		{"Commodore 1571", "C1571", "C1571", nullptr},
		{"Commodore 8050", "C8050", "C8050", nullptr},
		{"Commodore 8250", "C8250", "C8250", nullptr},
		{"Commodore 1581", "C1581", "C1581", nullptr},
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
	// TODO: If there's too many U+FFFD characters, try shifted instead.
		// ...if it's like that in Disk Name, use shifted for filenames.
	// TODO: Default to shifted if this is a GEOS disk.
	// TODO: Reverse video?
	static const char uFFFD[] = "\xEF\xBF\xBD";
	unsigned int codepage = CP_RP_PETSCII_Unshifted;

	d->fields.reserve(4);	// Maximum of 4 fields.

	union {
		cbmdos_C1541_BAM_t c1541_bam;
		cbmdos_C8050_header_t c8050_header;
		cbmdos_C1581_header_t c1581_header;
		cbmdos_C128_autoboot_sector_t autoboot;
	};

	// Get the BAM/header sector. (sector 0 of the directory track)
	size_t size = d->read_sector(&c1541_bam, sizeof(c1541_bam), d->dir_track, 0);
	if (size == sizeof(c1541_bam)) {
		// BAM loaded. Get the string addresses.
		char *disk_name, *disk_id, *dos_type;
		int disk_name_len;

		switch (d->diskType) {
			case CBMDOSPrivate::DiskType::D64:
			case CBMDOSPrivate::DiskType::D71:
				// C1541/C1571
				disk_name = c1541_bam.disk_name;
				disk_name_len = static_cast<int>(sizeof(c1541_bam.disk_name));
				disk_id = c1541_bam.disk_id;
				dos_type = c1541_bam.dos_type;
				break;

			case CBMDOSPrivate::DiskType::D80:
			case CBMDOSPrivate::DiskType::D82:
				// C8050/C8250
				disk_name = c8050_header.disk_name;
				disk_name_len = static_cast<int>(sizeof(c8050_header.disk_name));
				disk_id = c8050_header.disk_id;
				dos_type = c8050_header.dos_type;
				break;

			case CBMDOSPrivate::DiskType::D81:
				// C1581
				disk_name = c1581_header.disk_name;
				disk_name_len = static_cast<int>(sizeof(c1581_header.disk_name));
				disk_id = c1581_header.disk_id;
				dos_type = c1581_header.dos_type;
				break;

			default:
				assert(!"Unsupported CBM disk type?");
				return 0;
		}

		// Disk name
		remove_A0_padding(disk_name, disk_name_len);
		string s_disk_name = cpN_to_utf8(codepage, disk_name, disk_name_len);
		if (s_disk_name.find(uFFFD) != string::npos) {
			// Disk name has invalid characters when using Unshifted.
			// Try again with Shifted.
			codepage = CP_RP_PETSCII_Shifted;
			s_disk_name = cpN_to_utf8(codepage, disk_name, disk_name_len);
		}
		d->fields.addField_string(C_("CBMDOS", "Disk Name"), s_disk_name);

		// Disk ID
		d->fields.addField_string(C_("CBMDOS", "Disk ID"),
			cpN_to_utf8(codepage, disk_id, sizeof(c1541_bam.disk_id)));

		// DOS Type (NOTE: Always unshifted)
		d->fields.addField_string(C_("CBMDOS", "DOS Type"),
			cpN_to_utf8(CP_RP_PETSCII_Unshifted, dos_type, sizeof(c1541_bam.dos_type)));
	}

	// C1581 has an additional file type, "CBM".
	const uint8_t max_file_type = (d->diskType == CBMDOSPrivate::DiskType::D81) ? 6 : 5;

	// Read the directory.
	// NOTE: Ignoring the directory location in the BAM sector,
	// since it might be incorrect. Assuming dir_track/dir_first_sector.
	bitset<64> sectors_read(1);	// Sector 0 is not allowed here, so mark it as 'read'.
	vector<vector<string> > *const vv_dir = new vector<vector<string> >();
	const unsigned int sector_count = d->track_offsets[d->dir_track].sector_count;
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

			// # of blocks (filesize)
			char filesize[16];
			snprintf(filesize, sizeof(filesize), "%u", le16_to_cpu(p_dir->sector_count));
			p_list.emplace_back(filesize);

			// Filename
			remove_A0_padding(p_dir->filename, sizeof(p_dir->filename));
			p_list.emplace_back(cpN_to_utf8(codepage, p_dir->filename, sizeof(p_dir->filename)));

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
			const uint8_t file_type = (p_dir->file_type & CBMDOS_FileType_Mask);
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
		}
	}

	static const char *const dir_headers[] = {
		NOP_C_("CBMDOS|Directory", "Blocks"),
		NOP_C_("CBMDOS|Directory", "Filename"),
		NOP_C_("CBMDOS|Directory", "Type"),
	};
	vector<string> *const v_dir_headers = RomFields::strArrayToVector_i18n(
		"CBMDOS|Directory", dir_headers, ARRAY_SIZE(dir_headers));

	RomFields::AFLD_PARAMS params(0, 8);
	params.headers = v_dir_headers;
	params.data.single = vv_dir;
	params.col_attrs.align_headers	= AFLD_ALIGN3(TXA_D, TXA_D, TXA_D);
	params.col_attrs.align_data	= AFLD_ALIGN3(TXA_R, TXA_D, TXA_D);
	params.col_attrs.sizing		= AFLD_ALIGN3(COLSZ_R, COLSZ_S, COLSZ_R);
	params.col_attrs.sorting	= AFLD_ALIGN3(COLSORT_NUM, COLSORT_STD, COLSORT_STD);
	params.col_attrs.sort_col	= -1;	// no sorting by default; show files as listed on disk
	params.col_attrs.sort_dir	= RomFields::COLSORTORDER_ASCENDING;
	d->fields.addField_listData(C_("CBMDOS", "Directory"), &params);

	// Check for a C128 autoboot sector.
	if (d->diskType == CBMDOSPrivate::DiskType::D64 ||
	    d->diskType == CBMDOSPrivate::DiskType::D71)
	{
		size = d->read_sector(&autoboot, sizeof(autoboot), 1, 0);
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

}
