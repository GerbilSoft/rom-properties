/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CBMDOS.cpp: Commodore DOS floppy disk image parser.                     *
 *                                                                         *
 * Copyright (c) 2019-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://unusedino.de/ec64/technical/formats/d64.html

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

		D64 = 0,	// C1541 disk image (standard version)

		Max
	};
	DiskType diskType;

	// Track count
	// Usually 35 for a standard 1541 disk; can be up to 40.
	uint8_t track_count;

	// Directory track
	// Usually 18 for C1541 disks.
	uint8_t dir_track;

public:
	// Track offsets
	// Index is track number, minus one.
	struct track_offsets_t {
		uint8_t sector_count;	// Sectors per track
		uint16_t start_sector;	// Starting sector
	};
	// Track offsets (C1541)
	static const track_offsets_t track_offsets_C1541[40];

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
	// TODO: More?

	nullptr
};
const char *const CBMDOSPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-d64",

	nullptr
};
const RomDataInfo CBMDOSPrivate::romDataInfo = {
	"CBMDOS", exts, mimeTypes
};

// Track offsets (C1541)
const CBMDOSPrivate::track_offsets_t CBMDOSPrivate::track_offsets_C1541[40] = {
	// Tracks 1-17 (21 sectors)
	{21,   0}, {21,  21}, {21,  42}, {21,  63},
	{21,  84}, {21, 105}, {21, 126}, {21, 147},
	{21, 168}, {21, 189}, {21, 210}, {21, 231},
	{21, 252}, {21, 273}, {21, 294}, {21, 315},
	{21, 336},

	// Tracks 18-24 (19 sectors)
	{19, 357}, {19, 376}, {19, 395}, {19, 414},
	{19, 433}, {19, 452}, {19, 471},

	// Tracks 25-31 (18 sectors)
	{18, 490}, {18, 508}, {18, 526}, {18, 544},
	{18, 562}, {18, 580},
	
	// Tracks 31-40 (17 sectors)
	{17, 598}, {17, 615}, {17, 632}, {17, 649},
	{17, 666}, {17, 683}, {17, 700}, {17, 717},
	{17, 734}, {17, 751}
};

CBMDOSPrivate::CBMDOSPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, diskType(DiskType::Unknown)
	, track_count(0)
	, dir_track(0)
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
	const track_offsets_t track_offsets = track_offsets_C1541[track-1];

	assert(sector < track_offsets.sector_count);
	if (sector >= track_offsets.sector_count)
		return 0;

	// Get the absolute starting address.
	// TODO: Adjust for GCR formats.
	const off64_t start_pos = (track_offsets.start_sector + sector) * CBMDOS_SECTOR_SIZE;

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
			// 35-track image
			d->diskType = CBMDOSPrivate::DiskType::D64;
			d->track_count = 35;
			d->dir_track = 18;
			break;
		case (768 * CBMDOS_SECTOR_SIZE):
			// 40-track image
			d->diskType = CBMDOSPrivate::DiskType::D64;
			d->track_count = 40;
			d->dir_track = 18;
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
	static const char *const sysNames[1][4] = {
		{"Commodore 1541", "C1541", "C1541", nullptr},
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
	static const unsigned int codepage = CP_RP_PETSCII_Unshifted;

	d->fields.reserve(4);	// Maximum of 4 fields.

	// Get the BAM sector.
	cbmdos_C1541_BAM_t bam;
	size_t size = d->read_sector(&bam, sizeof(bam), d->dir_track, 0);
	if (size == sizeof(bam)) {
		// BAM loaded.

		// Disk name
		remove_A0_padding(bam.disk_name, sizeof(bam.disk_name));
		d->fields.addField_string(C_("CBMDOS", "Disk Name"),
			cpN_to_utf8(codepage, bam.disk_name, sizeof(bam.disk_name)));

		// Disk ID
		d->fields.addField_string(C_("CBMDOS", "Disk ID"),
			cpN_to_utf8(codepage, bam.disk_id, sizeof(bam.disk_id)));

		// DOS Type
		d->fields.addField_string(C_("CBMDOS", "DOS Type"),
			cpN_to_utf8(codepage, bam.dos_type, sizeof(bam.dos_type)));
	}

	// Read the directory.
	// NOTE: Ignoring the directory location in the BAM sector,
	// since it might be incorrect. Assuming 18/1.
	bitset<256> sectors_read(1);	// Sector 0 is not allowed here, so mark it as 'read'.
	vector<vector<string> > *const vv_dir = new vector<vector<string> >();
	const unsigned int sector_count = d->track_offsets_C1541[d->dir_track].sector_count;
	for (unsigned int i = 1; i < sector_count && !sectors_read.test(i); ) {
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
			static const char file_type_tbl[16][4] = {
				"DEL", "SEQ", "PRG", "USR",
				"REL",   "5",   "6",   "7",
				  "8",   "9",  "10",  "11",
				 "12",  "13",  "14",  "15",
			};
			s_file_type += file_type_tbl[p_dir->file_type & CBMDOS_FileType_Mask];

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
	d->fields.addField_listData(C_("CBMDOS", "Directory"), &params);

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

}
