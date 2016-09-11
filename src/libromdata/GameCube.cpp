/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCube.hpp: Nintendo GameCube and Wii disc image reader.              *
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

#include "GameCube.hpp"
#include "NintendoPublishers.hpp"
#include "gcn_structs.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

// DiscReader
#include "DiscReader.hpp"
#include "WbfsReader.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstring>

// C++ includes.
#include <vector>
using std::vector;

namespace LibRomData {

class GameCubePrivate
{
	public:
		GameCubePrivate(GameCube *q);
		~GameCubePrivate();

	private:
		GameCubePrivate(const GameCubePrivate &other);
		GameCubePrivate &operator=(const GameCubePrivate &other);

	private:
		GameCube *const q;

	public:
		// RomFields data.
		static const rp_char *const rvl_partitions_names[];
		static const RomFields::ListDataDesc rvl_partitions;
		static const struct RomFields::Desc gcn_fields[];

		enum DiscType {
			DISC_UNKNOWN = -1,	// Unknown disc type.

			// Low byte: System ID.
			DISC_SYSTEM_GCN = 0,		// GameCube disc image.
			DISC_SYSTEM_WII = 1,		// Wii disc image.
			DISC_SYSTEM_TRIFORCE = 2,	// Triforce disc/ROM image. [TODO]
			DISC_SYSTEM_UNKNOWN = 0xFF,
			DISC_SYSTEM_MASK = 0xFF,

			// High byte: Image format.
			DISC_FORMAT_RAW  = (0 << 8),	// Raw image. (ISO, GCM)
			DISC_FORMAT_WBFS = (1 << 8),	// WBFS image. (Wii only)
			DISC_FORMAT_UNKNOWN = (0xFF << 8),
			DISC_FORMAT_MASK = (0xFF << 8),
		};

		// Disc type and reader.
		int discType;
		IDiscReader *discReader;

		// Disc header.
		GCN_DiscHeader discHeader;

		/**
		 * Wii partition tables.
		 * Decoded from the actual on-disc tables.
		 */
		struct WiiPartEntry {
			uint64_t start;		// Starting address, in bytes.
			//uint64_t length;	// Length, in bytes. [TODO: Calculate this]
			uint32_t type;		// Partition type. (0 == Game, 1 == Update, 2 == Channel)
		};

		typedef std::vector<WiiPartEntry> WiiPartTable;
		WiiPartTable wiiVgTbl[4];	// Volume group table.
		bool wiiVgTblLoaded;

		/**
		 * Load the Wii volume group and partition tables.
		 * Partition tables are loaded into wiiVgTbl[].
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadWiiPartitionTables(void);
};

/** GameCubePrivate **/

GameCubePrivate::GameCubePrivate(GameCube *q)
	: q(q)
	, discType(DISC_UNKNOWN)
	, discReader(nullptr)
	, wiiVgTblLoaded(false)
{ }

GameCubePrivate::~GameCubePrivate()
{
	delete discReader;
}

// Wii partition table.
const rp_char *const GameCubePrivate::rvl_partitions_names[] = {
	_RP("#"), _RP("Type"),
	// TODO: Start/End addresses?
};

const struct RomFields::ListDataDesc GameCubePrivate::rvl_partitions = {
	ARRAY_SIZE(rvl_partitions_names), rvl_partitions_names
};

// ROM fields.
const struct RomFields::Desc GameCubePrivate::gcn_fields[] = {
	// TODO: Banner?
	{_RP("Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Game ID"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Publisher"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Disc #"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Revision"), RomFields::RFT_STRING, {nullptr}},

	// Wii partition table.
	// NOTE: Actually a table of tables, so we'll use
	// 0p0-style numbering, where the first digit is
	// the table number, and the second digit is the
	// partition number. (both start at 0)
	{_RP("Partitions"), RomFields::RFT_LISTDATA, {&rvl_partitions}},

	// TODO:
	// - System update version.
	// - Region and age ratings.
};

/**
 * Load the Wii volume group and partition tables.
 * Partition tables are loaded into wiiVgTbl[].
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCubePrivate::loadWiiPartitionTables(void)
{
	if (wiiVgTblLoaded) {
		// Partition tables have already been loaded.
		return 0;
	} else if (!q->m_file || !q->m_file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if ((discType & DISC_SYSTEM_MASK) != DISC_SYSTEM_WII) {
		// Unsupported disc type.
		return -EIO;
	}

	// Clear the existing partition tables.
	for (int i = ARRAY_SIZE(wiiVgTbl)-1; i >= 0; i--) {
		wiiVgTbl[i].clear();
	}

	// Assuming a maximum of 128 partitions per table.
	// (This is a rather high estimate.)
	RVL_VolumeGroupTable vgtbl;
	RVL_PartitionTableEntry pt[1024];

	// Read the volume group table.
	// References:
	// - http://wiibrew.org/wiki/Wii_Disc#Partitions_information
	// - http://blog.delroth.net/2011/06/reading-wii-discs-with-python/
	discReader->seek(0x40000);
	size_t size = discReader->read(&vgtbl, sizeof(vgtbl));
	if (size != sizeof(vgtbl)) {
		// Could not read the volume group table.
		// TODO: Return error from fread()?
		return -EIO;
	}

	// Get the size of the disc image.
	// TODO: Large File Support for 32-bit Linux and Windows.
	int64_t discSize = discReader->fileSize();
	if (discSize < 0) {
		// Error getting the size of the disc image.
		return -errno;
	}

	// Process each volume group.
	for (int i = 0; i < 4; i++) {
		uint32_t count = be32_to_cpu(vgtbl.vg[i].count);
		if (count == 0) {
			continue;
		} else if (count > ARRAY_SIZE(pt)) {
			count = ARRAY_SIZE(pt);
		}

		// Read the partition table entries.
		uint64_t pt_addr = (uint64_t)(be32_to_cpu(vgtbl.vg[i].addr)) << 2;
		const size_t ptSize = sizeof(RVL_PartitionTableEntry) * count;
		discReader->seek((int64_t)pt_addr);
		size = discReader->read(pt, ptSize);
		if (size != ptSize) {
			// Error reading the partition table entries.
			return -EIO;
		}

		// Process each partition table entry.
		wiiVgTbl[i].resize(count);
		for (int j = 0; j < (int)count; j++) {
			WiiPartEntry &entry = wiiVgTbl[i].at(j);
			entry.start = (uint64_t)(be32_to_cpu(pt[j].addr)) << 2;
			// TODO: Figure out how to calculate length?
			entry.type = be32_to_cpu(pt[j].type);
		}
	}

	// Done reading the partition tables.
	return 0;
}

/** GameCube **/

/**
 * Read a Nintendo GameCube or Wii disc image.
 *
 * A disc image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open disc image.
 */
GameCube::GameCube(IRpFile *file)
	: RomData(file, GameCubePrivate::gcn_fields, ARRAY_SIZE(GameCubePrivate::gcn_fields))
	, d(new GameCubePrivate(this))
{
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the disc header.
	uint8_t header[4096+256];
	m_file->rewind();
	size_t size = m_file->read(&header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this disc image is supported.
	DetectInfo info;
	info.pHeader = header;
	info.szHeader = sizeof(header);
	info.ext = nullptr;	// Not needed for GCN.
	info.szFile = 0;	// Not needed for GCN.
	d->discType = isRomSupported(&info);

	// TODO: DiscReaderFactory?
	if (d->discType >= 0) {
		switch (d->discType & GameCubePrivate::DISC_FORMAT_MASK) {
			case GameCubePrivate::DISC_FORMAT_RAW:
				d->discReader = new DiscReader(m_file);
				break;
			case GameCubePrivate::DISC_FORMAT_WBFS:
				d->discReader = new WbfsReader(m_file);
				break;
			case GameCubePrivate::DISC_FORMAT_UNKNOWN:
			default:
				d->discType = GameCubePrivate::DISC_UNKNOWN;
				break;
		}
	}

	m_isValid = (d->discType >= 0);
	if (m_isValid) {
		// Save the disc header for later.
		d->discReader->rewind();
		size = d->discReader->read(&d->discHeader, sizeof(d->discHeader));
		if (size != sizeof(d->discHeader)) {
			// Error reading the disc header.
			delete d->discReader;
			d->discReader = nullptr;
			d->discType = GameCubePrivate::DISC_UNKNOWN;
			m_isValid = false;
		}
	}
}

GameCube::~GameCube()
{
	delete d;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Object-specific system ID (>= 0) if supported; -1 if not.
 */
int GameCube::isRomSupported_static(const DetectInfo *info)
{
	if (!info || info->szHeader < sizeof(GCN_DiscHeader)) {
		// Either no detection information was specified,
		// or the header is too small.
		return GameCubePrivate::DISC_UNKNOWN;
	}

	static const uint32_t magic_wii = 0x5D1C9EA3;
	static const uint32_t magic_gcn = 0xC2339F3D;

	// Check for the magic numbers.
	const GCN_DiscHeader *gcn_header = reinterpret_cast<const GCN_DiscHeader*>(info->pHeader);
	if (gcn_header->magic_wii == cpu_to_be32(magic_wii)) {
		// Wii disc image.
		return (GameCubePrivate::DISC_SYSTEM_WII | GameCubePrivate::DISC_FORMAT_RAW);
	} else if (gcn_header->magic_gcn == cpu_to_be32(magic_gcn)) {
		// GameCube disc image.
		// TODO: Check for Triforce?
		return (GameCubePrivate::DISC_SYSTEM_GCN | GameCubePrivate::DISC_FORMAT_RAW);
	}

	// Check for NDDEMO. (Early GameCube demo discs.)
	static const uint8_t nddemo_header[64] = {
		0x30, 0x30, 0x00, 0x45, 0x30, 0x31, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x4E, 0x44, 0x44, 0x45, 0x4D, 0x4F, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};
	if (!memcmp(gcn_header, nddemo_header, sizeof(nddemo_header))) {
		// NDDEMO disc.
		return (GameCubePrivate::DISC_SYSTEM_GCN | GameCubePrivate::DISC_FORMAT_RAW);
	}

	// Check for WBFS.
	// This is checked after the magic numbers in case some joker
	// decides to make a GCN or Wii disc image with the game ID "WBFS".
	static const uint8_t wbfs_magic[4] = {'W', 'B', 'F', 'S'};
	if (!memcmp(info->pHeader, wbfs_magic, sizeof(wbfs_magic))) {
		// Disc image is stored in "HDD" sector 1.
		unsigned int hdd_sector_size = (1 << info->pHeader[8]);
		if (info->szHeader >= hdd_sector_size + 0x200) {
			// Check for Wii magic.
			// FIXME: GCN magic too?
			gcn_header = reinterpret_cast<const GCN_DiscHeader*>(&info->pHeader[hdd_sector_size]);
			if (gcn_header->magic_wii == cpu_to_be32(magic_wii)) {
				// Wii disc image. (WBFS format)
				return (GameCubePrivate::DISC_SYSTEM_WII | GameCubePrivate::DISC_FORMAT_WBFS);
			}
		}
	}

	// Not supported.
	return GameCubePrivate::DISC_UNKNOWN;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameCube::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @return System name, or nullptr if not supported.
 */
const rp_char *GameCube::systemName(void) const
{
	switch (d->discType & GameCubePrivate::DISC_SYSTEM_MASK) {
		case GameCubePrivate::DISC_SYSTEM_GCN:
			return _RP("GameCube");
		case GameCubePrivate::DISC_SYSTEM_WII:
			return _RP("Wii");
		case GameCubePrivate::DISC_SYSTEM_TRIFORCE:
			return _RP("Triforce");
		default:
			break;
	}

	return nullptr;
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> GameCube::supportedFileExtensions_static(void)
{
	// TODO: Add ".iso" later? (Too generic, though...)
	vector<const rp_char*> ret;
	ret.reserve(3);
	ret.push_back(_RP(".gcm"));
	ret.push_back(_RP(".rvm"));
	ret.push_back(_RP(".wbfs"));
	return ret;
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> GameCube::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t GameCube::supportedImageTypes_static(void)
{
       return IMGBF_EXT_MEDIA;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t GameCube::supportedImageTypes(void) const
{
       return supportedImageTypes_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GameCube::loadFieldData(void)
{
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!m_file || !m_file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid || d->discType < 0) {
		// Unknown disc type.
		return -EIO;
	}

	// Disc header is read in the constructor.

	// TODO: Trim the titles. (nulls, spaces)
	// NOTE: The titles are dup()'d as C strings, so maybe not nulls.

	// Game title.
	// TODO: Is Shift-JIS actually permissible here?
	m_fields->addData_string(cp1252_sjis_to_rp_string(
		d->discHeader.game_title, sizeof(d->discHeader.game_title)));

	// Game ID.
	// Replace any non-printable characters with underscores.
	// (NDDEMO has ID6 "00\0E01".)
	char id6[7];
	for (int i = 0; i < 6; i++) {
		id6[i] = (isprint(d->discHeader.id6[i])
			? d->discHeader.id6[i]
			: '_');
	}
	id6[6] = 0;
	m_fields->addData_string(latin1_to_rp_string(id6, 6));

	// Look up the publisher.
	const rp_char *publisher = NintendoPublishers::lookup(d->discHeader.company);
	m_fields->addData_string(publisher ? publisher : _RP("Unknown"));

	// Other fields.
	m_fields->addData_string_numeric(d->discHeader.disc_number+1, RomFields::FB_DEC);
	m_fields->addData_string_numeric(d->discHeader.revision, RomFields::FB_DEC, 2);

	// Partition table. (Wii only)
	if ((d->discType & GameCubePrivate::DISC_SYSTEM_MASK) == GameCubePrivate::DISC_SYSTEM_WII) {
		RomFields::ListData *partitions = new RomFields::ListData();

		// Load the Wii partition tables.
		int ret = d->loadWiiPartitionTables();
		if (ret == 0) {
			// Wii partition tables loaded.
			// Convert them to RFT_LISTDATA for display purposes.
			vector<rp_string> data_row;     // Temporary storage for each partition entry.
			for (int i = 0; i < 4; i++) {
				const int count = (int)d->wiiVgTbl[i].size();
				for (int j = 0; j < count; j++) {
					const GameCubePrivate::WiiPartEntry &entry = d->wiiVgTbl[i].at(j);
					data_row.clear();

					// Partition number.
					char buf[16];
					int len = snprintf(buf, sizeof(buf), "%dp%d", i, j);
					if (len > (int)sizeof(buf))
						len = sizeof(buf);
					data_row.push_back(len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));

					// Partition type.
					rp_string str;
					switch (entry.type) {
						case 0:
							str = _RP("Game");
							break;
						case 1:
							str = _RP("Update");
							break;
						case 2:
							str = _RP("Channel");
							break;
						default: {
							// If all four bytes are ASCII,
							// print it as-is. (SSBB demo channel)
							// Otherwise, print the number.
							// NOTE: Must be BE32 for proper display.
							uint32_t be32_type = cpu_to_be32(entry.type);
							memcpy(buf, &be32_type, 4);
							if (isalnum(buf[0]) && isalnum(buf[1]) &&
							    isalnum(buf[2]) && isalnum(buf[3]))
							{
								// All four bytes are ASCII.
								str = latin1_to_rp_string(buf, 4);
							} else {
								// Non-ASCII data. Print the hex values instead.
								len = snprintf(buf, sizeof(buf), "%08X", entry.type);
								if (len > (int)sizeof(buf))
									len = sizeof(buf);
								str = (len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
							}
 						}
					}
					data_row.push_back(str);

					// Add the partition information to the ListData.
					partitions->data.push_back(data_row);
				}
			}
		}

		// Add the partitions list data.
		m_fields->addData_listData(partitions);
	} else {
		// Add a dummy entry.
		m_fields->addData_invalid();
	}

	// Finished reading the field data.
	return (int)m_fields->count();
}

/**
 * Get the GameTDB URL for a given game.
 * @param system System name.
 * @param type Image type.
 * @param region Region name.
 * @param gameID Game ID.
 * @return GameTDB URL.
 */
static LibRomData::rp_string getURL_GameTDB(const char *system, const char *type, const char *region, const char *gameID)
{
	char buf[128];
	int len = snprintf(buf, sizeof(buf), "http://art.gametdb.com/%s/%s/%s/%s.png", system, type, region, gameID);
	if (len > (int)sizeof(buf))
		len = sizeof(buf);	// TODO: Handle truncation better.

	// TODO: UTF-8, not Latin-1?
	return (len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
}

/**
 * Get the GameTDB URL for a given game.
 * @param system System name.
 * @param type Image type.
 * @param region Region name.
 * @param gameID Game ID.
 * TODO: PAL multi-region selection?
 * @return GameTDB URL.
 */
static LibRomData::rp_string getCacheKey(const char *system, const char *type, const char *region, const char *gameID)
{
	char buf[128];
	int len = snprintf(buf, sizeof(buf), "%s/%s/%s/%s.png", system, type, region, gameID);
	if (len > (int)sizeof(buf))
		len = sizeof(buf);	// TODO: Handle truncation better.

	// TODO: UTF-8, not Latin-1?
	return (len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
}

/**
 * Load URLs for an external media type.
 * Called by RomData::extURL() if the URLs haven't been loaded yet.
 * @param imageType Image type to load.
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCube::loadURLs(ImageType imageType)
{
	assert(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_EXT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}

	const int idx = imageType - IMG_EXT_MIN;
	std::vector<ExtURL> &extURLs = m_extURLs[idx];
	if (!extURLs.empty()) {
		// URLs *have* been loaded...
		return 0;
	} else if (!m_file || !m_file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (d->discType < 0) {
		// Unknown disc type.
		return -EIO;
	}

	// Check for the requested media type.
	// NOTE: GameTDB has coverHQ for Wii/GCN, but not discHQ.
	// GameTDB does not have *M for Wii/GCN.
	// TODO: Configurable quality settings?
	// TODO: Option to pick the *B version?
	// TODO: Handle discs that have weird region codes?
	const char *imageTypeName;
	switch (imageType) {
		case IMG_EXT_MEDIA:
			imageTypeName = "disc";
			break;
		case IMG_EXT_BOX:
			imageTypeName = "cover";
			break;
		case IMG_EXT_BOX_FULL:
			imageTypeName = "coverfull";
			break;
		case IMG_EXT_BOX_3D:
			imageTypeName = "cover3D";
			break;
		default:
			// Unsupported image type.
			return -ENOENT;
	}

	// If PAL, fall back to EN if a language-specific
	// image isn't available.
	bool isPal = false;

	// Determine the GameTDB region code.
	const char *region;
	switch (d->discHeader.id4[3]) {
		// PAL regions. (Europe)
		case 'P':
		default:
			// TODO: Region selection for PAL?
			// Assuming "EN" for now.
			region = "EN";
			isPal = false;	// No fallback needed.
			break;
		case 'R':	// Russia
			region = "RU";
			isPal = true;
			break;
		case 'I':	// Italy
			region = "IT";
			isPal = true;
			break;
		case 'F':	// France
			region = "FR";
			isPal = true;
			break;
		case 'S':	// Spain
			region = "ES";
			isPal = true;
			break;
		case 'D':	// Germany
			region = "DE";
			isPal = true;
			break;

		// NTSC regions.
		case 'E':	// USA
			region = "US";
			break;
		case 'J':	// Japan
			region = "JA";
			break;
		case 'K':	// South Korea
			region = "KO";
			break;

		// Ambiguous...
		case 'W':
			// FIXME: This is used for Taiwan as well as
			// various "alternate" language packs in
			// PAL regions.
			region = "ZH";
			isPal = true;
			break;
		case 'X':
		case 'Y':
		case 'Z':
			// FIXME: This is used for "alternate" language packs
			// in PAL regions, plus some "special" versions.
			region = "EN";
			isPal = false;	// TODO: Fallback to US?
			break;
	}

	// NULL-terminate the id6.
	char id6[7];
	memcpy(id6, d->discHeader.id6, sizeof(id6));
	id6[6] = 0;

	// Replace any non-printable characters with underscores.
	// (NDDEMO has ID6 "00\0E01".)
	for (int i = 0; i < 6; i++) {
		if (!isprint(id6[i])) {
			id6[i] = '_';
		}
	}

	// Determine the image processing field.
	if (imageType == IMG_EXT_MEDIA) {
		switch (d->discType & GameCubePrivate::DISC_SYSTEM_MASK) {
			case GameCubePrivate::DISC_SYSTEM_WII:
			case GameCubePrivate::DISC_SYSTEM_TRIFORCE:
			default:
				// 120mm disc.
				m_imgpf[imageType] = IMGPF_CDROM_120MM;
				break;
			case GameCubePrivate::DISC_SYSTEM_GCN:
				// 80mm disc.
				m_imgpf[imageType] = IMGPF_CDROM_80MM;
				break;
		}
	} else {
		// No image processing for other types.
		// TODO: Should IMG_EXT_BOX_3D have a transparency mask?
		m_imgpf[imageType] = 0;
	}

	// Current extURL.
	ExtURL extURL;

	// Disc scan: Is this not the first disc?
	if (imageType == IMG_EXT_MEDIA &&
	    d->discHeader.disc_number > 0)
	{
		// Disc 2 (or 3, or 4...)
		// Request the disc 2 image first.
		char s_discNum[16];
		int len = snprintf(s_discNum, sizeof(s_discNum), "disc%u", d->discHeader.disc_number+1);
		if (len > 0 && len < (int)(sizeof(s_discNum))) {
			extURL.url = getURL_GameTDB("wii", s_discNum, region, id6);
			extURL.cache_key = getCacheKey("wii", s_discNum, region, id6);
			extURLs.push_back(extURL);

			if (isPal) {
				// Fall back to "EN" if the region-specific image wasn't found.
				extURL.url = getURL_GameTDB("wii", s_discNum, "EN", id6);
				extURL.cache_key = getCacheKey("wii", s_discNum, "EN", id6);
				extURLs.push_back(extURL);
			}
		}
	}

	// First disc, or not a disc scan.
	extURL.url = getURL_GameTDB("wii", imageTypeName, region, id6);
	extURL.cache_key = getCacheKey("wii", imageTypeName, region, id6);
	extURLs.push_back(extURL);
	if (isPal) {
		// Fall back to "EN" if the region-specific image wasn't found.
		extURL.url = getURL_GameTDB("wii", imageTypeName, "EN", id6);
		extURL.cache_key = getCacheKey("wii", imageTypeName, "EN", id6);
		extURLs.push_back(extURL);
	}

	// All URLs added.
	return (extURLs.empty() ? -ENOENT : 0);
}

}
