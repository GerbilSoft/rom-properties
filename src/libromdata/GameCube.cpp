/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCube.cpp: Nintendo GameCube and Wii disc image reader.              *
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
#include "gcn_banner.h"
#include "SystemRegion.hpp"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"
#include "img/rp_image.hpp"
#include "img/ImageDecoder.hpp"

// DiscReader
#include "disc/DiscReader.hpp"
#include "disc/WbfsReader.hpp"
#include "disc/CisoGcnReader.hpp"
#include "disc/WiiPartition.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <memory>
#include <sstream>
#include <string>
#include <vector>
using std::ostringstream;
using std::string;
using std::unique_ptr;
using std::vector;

#include "SystemRegion.hpp"

namespace LibRomData {

class GameCubePrivate
{
	public:
		explicit GameCubePrivate(GameCube *q);
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

		// NDDEMO header.
		static const uint8_t nddemo_header[64];

		enum DiscType {
			DISC_UNKNOWN = -1,	// Unknown disc type.

			// Low byte: System ID.
			DISC_SYSTEM_GCN = 0,		// GameCube disc image.
			DISC_SYSTEM_TRIFORCE = 1,	// Triforce disc/ROM image. [TODO]
			DISC_SYSTEM_WII = 2,		// Wii disc image.
			DISC_SYSTEM_UNKNOWN = 0xFF,
			DISC_SYSTEM_MASK = 0xFF,

			// High byte: Image format.
			DISC_FORMAT_RAW  = (0 << 8),	// Raw image. (ISO, GCM)
			DISC_FORMAT_WBFS = (1 << 8),	// WBFS image. (Wii only)
			DISC_FORMAT_CISO = (2 << 8),	// CISO image.
			DISC_FORMAT_TGC  = (3 << 8),	// TGC (embedded disc image) (GCN only?)
			DISC_FORMAT_UNKNOWN = (0xFF << 8),
			DISC_FORMAT_MASK = (0xFF << 8),
		};

		// Disc type and reader.
		int discType;
		IDiscReader *discReader;

		// Disc header.
		GCN_DiscHeader discHeader;
		RVL_RegionSetting regionSetting;

		// GameCube opening.bnr.
		// NOTE: Check gcn_opening_bnr->magic to determine
		// how many comment fields are present.
		banner_bnr2_t *gcn_opening_bnr;

		// Region code. (bi2.bin for GCN, RVL_RegionSetting for Wii.)
		uint32_t gcnRegion;

		enum WiiPartitionType {
			PARTITION_GAME = 0,
			PARTITION_UPDATE = 1,
			PARTITION_CHANNEL = 2,
		};

		/**
		 * Wii partition tables.
		 * Decoded from the actual on-disc tables.
		 */
		struct WiiPartEntry {
			uint64_t start;			// Starting address, in bytes.
			uint32_t type;			// Partition type. (See WiiPartitionType.)
			WiiPartition *partition;	// Partition object.
		};

		typedef std::vector<WiiPartEntry> WiiPartTable;
		WiiPartTable wiiVgTbl[4];	// Volume group table.
		bool wiiVgTblLoaded;

		// Update partition.
		// Points to entry within WiiParttable.
		WiiPartition *updatePartition;

		/**
		 * Load the Wii volume group and partition tables.
		 * Partition tables are loaded into wiiVgTbl[].
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadWiiPartitionTables(void);

		/**
		 * Format a file size.
		 * @param fileSize File size.
		 * @return Formatted file size.
		 */
		static rp_string formatFileSize(int64_t fileSize);

		/**
		 * Parse the Wii System Update version.
		 * @param version Version from the RVL-WiiSystemmenu-v*.wad filename.
		 * @return Wii System Update version, or nullptr if unknown.
		 */
		static const rp_char *parseWiiSystemMenuVersion(unsigned int version);

		/**
		 * Convert a GCN region value (from GCN_Boot_Info or RVL_RegionSetting) to a string.
		 * @param gcnRegion GCN region value.
		 * @param idRegion Game ID region.
		 * @return String, or nullptr if the region value is invalid.
		 */
		static const rp_char *gcnRegionToString(unsigned int gcnRegion, char idRegion);

		/**
		 * Convert a GCN region value (from GCN_Boot_Info or RVL_RegionSetting) to a GameTDB region code.
		 * @param gcnRegion GCN region value.
		 * @param idRegion Game ID region.
		 *
		 * NOTE: Mulitple GameTDB region codes may be returned including:
		 * - User-specified fallback region. [TODO]
		 * - General fallback region.
		 *
		 * @return GameTDB region code(s), or empty vector if the region value is invalid.
		 */
		vector<const char*> gcnRegionToGameTDB(unsigned int gcnRegion, char idRegion);

		/**
		 * Load opening.bnr. (GameCube only)
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int gcn_loadOpeningBnr(void);

		/**
		 * Get the banner_comment_t from opening.bnr.
		 * For BNR2, this uses the comment that most
		 * closely matches the host system language.
		 * @return banner_comment_t, or nullptr if opening.bnr was not loaded.
		 */
		const banner_comment_t *gcn_getBannerComment(void) const;
};

/** GameCubePrivate **/

GameCubePrivate::GameCubePrivate(GameCube *q)
	: q(q)
	, discType(DISC_UNKNOWN)
	, discReader(nullptr)
	, gcn_opening_bnr(nullptr)
	, gcnRegion(~0)
	, wiiVgTblLoaded(false)
	, updatePartition(nullptr)
{
	// Clear the Wii region settings.
	memset(&regionSetting, 0, sizeof(regionSetting));
}

GameCubePrivate::~GameCubePrivate()
{
	updatePartition = nullptr;

	// Delete partition objects in wiiVgTbl[].
	// TODO: Check wiiVgTblLoaded?
	for (int i = ARRAY_SIZE(wiiVgTbl)-1; i >= 0; i--) {
		for (auto iter = wiiVgTbl[i].begin(); iter != wiiVgTbl[i].end(); ++iter) {
			delete iter->partition;
		}
	}

	free(gcn_opening_bnr);
	delete discReader;
}

// Wii partition table.
const rp_char *const GameCubePrivate::rvl_partitions_names[] = {
	_RP("#"), _RP("Type"), _RP("Key"), _RP("Size")
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
	{_RP("Region"), RomFields::RFT_STRING, {nullptr}},

	/** GameCube-specific fields. **/
	// Game information from opening.bnr.
	{_RP("Game Info"), RomFields::RFT_STRING, {nullptr}},

	/** Wii-specific fields. **/
	// Age rating(s).
	{_RP("Age Rating"), RomFields::RFT_STRING, {nullptr}},
	// Wii System Update version.
	{_RP("Update"), RomFields::RFT_STRING, {nullptr}},

	// Wii partition table.
	// NOTE: Actually a table of tables, so we'll use
	// 0p0-style numbering, where the first digit is
	// the table number, and the second digit is the
	// partition number. (both start at 0)
	{_RP("Partitions"), RomFields::RFT_LISTDATA, {&rvl_partitions}},
};

// NDDEMO header.
const uint8_t GameCubePrivate::nddemo_header[64] = {
	0x30, 0x30, 0x00, 0x45, 0x30, 0x31, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x4E, 0x44, 0x44, 0x45, 0x4D, 0x4F, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
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
	discReader->seek(RVL_VolumeGroupTable_ADDRESS);
	size_t size = discReader->read(&vgtbl, sizeof(vgtbl));
	if (size != sizeof(vgtbl)) {
		// Could not read the volume group table.
		// TODO: Return error from fread()?
		return -EIO;
	}

	// Get the size of the disc image.
	int64_t discSize = discReader->size();
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
			entry.type = be32_to_cpu(pt[j].type);
			entry.partition = new WiiPartition(discReader, entry.start);

			if (entry.type == PARTITION_UPDATE && !updatePartition) {
				// System Update partition.
				updatePartition = entry.partition;
			}
		}
	}

	// Done reading the partition tables.
	return 0;
}

static inline int calc_frac_part(int64_t size, int64_t mask)
{
	float f = (float)(size & (mask - 1)) / (float)mask;
	int frac_part = (int)(f * 1000.0f);

	// MSVC added round() and roundf() in MSVC 2013.
	// Use our own rounding code instead.
	int round_adj = (frac_part % 10 > 5);
	frac_part /= 10;
	frac_part += round_adj;
	return frac_part;
}

/**
 * Format a file size.
 * @param size File size.
 * @return Formatted file size.
 */
rp_string GameCubePrivate::formatFileSize(int64_t size)
{
	// TODO: Move to somewhere common?
	// TODO: Thousands formatting?
	char buf[64];

	const char *suffix;
	// frac_part is always 0 to 100.
	// If whole_part >= 10, frac_part is divided by 10.
	int whole_part, frac_part;

	// TODO: Optimize this?
	// TODO: Localize this?
	if (size < 0) {
		// Invalid size. Print the value as-is.
		suffix = "";
		whole_part = (int)size;
		frac_part = 0;
	} else if (size < (2LL << 10)) {
		suffix = (size == 1 ? "byte" : " bytes");
		whole_part = (int)size;
		frac_part = 0;
	} else if (size < (2LL << 20)) {
		suffix = " KB";
		whole_part = (int)(size >> 10);
		frac_part = calc_frac_part(size, (1LL << 10));
	} else if (size < (2LL << 30)) {
		suffix = " MB";
		whole_part = (int)(size >> 20);
		frac_part = calc_frac_part(size, (1LL << 20));
	} else if (size < (2LL << 40)) {
		suffix = " GB";
		whole_part = (int)(size >> 30);
		frac_part = calc_frac_part(size, (1LL << 30));
	} else if (size < (2LL << 50)) {
		suffix = " TB";
		whole_part = (int)(size >> 40);
		frac_part = calc_frac_part(size, (1LL << 40));
	} else if (size < (2LL << 60)) {
		suffix = " PB";
		whole_part = (int)(size >> 50);
		frac_part = calc_frac_part(size, (1LL << 50));
	} else /*if (size < (2ULL << 70))*/ {
		suffix = " EB";
		whole_part = (int)(size >> 60);
		frac_part = calc_frac_part(size, (1LL << 60));
	}

	int len;
	if (size < (2LL << 10)) {
		// Bytes or negative value. No fractional part.
		len = snprintf(buf, sizeof(buf), "%d%s", whole_part, suffix);
	} else {
		// TODO: Localized decimal point?
		int frac_digits = 2;
		if (whole_part >= 10) {
			int round_adj = (frac_part % 10 > 5);
			frac_part /= 10;
			frac_part += round_adj;
			frac_digits = 1;
		}
		len = snprintf(buf, sizeof(buf), "%d.%0*d%s",
			whole_part, frac_digits, frac_part, suffix);
	}

	if (len > (int)sizeof(buf))
		len = (int)sizeof(buf);
	return (len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
}

/**
 * Parse the Wii System Update version.
 * @param version Version from the RVL-WiiSystemmenu-v*.wad filename.
 * @return Wii System Update version, or nullptr if unknown.
 */
const rp_char *GameCubePrivate::parseWiiSystemMenuVersion(unsigned int version)
{
	switch (version) {
		// Wii
		// Reference: http://wiiubrew.org/wiki/Title_database
		case 33:	return _RP("1.0");
		case 97:	return _RP("2.0U");
		case 128:	return _RP("2.0J");
		case 130:	return _RP("2.0E");
		case 162:	return _RP("2.1E");
		case 192:	return _RP("2.2J");
		case 193:	return _RP("2.2U");
		case 194:	return _RP("2.2E");
		case 224:	return _RP("3.0J");
		case 225:	return _RP("3.0U");
		case 226:	return _RP("3.0E");
		case 256:	return _RP("3.1J");
		case 257:	return _RP("3.1U");
		case 258:	return _RP("3.1E");
		case 288:	return _RP("3.2J");
		case 289:	return _RP("3.2U");
		case 290:	return _RP("3.2E");
		case 326:	return _RP("3.3K");
		case 352:	return _RP("3.3J");
		case 353:	return _RP("3.3U");
		case 354:	return _RP("3.3E");
		case 384:	return _RP("3.4J");
		case 385:	return _RP("3.4U");
		case 386:	return _RP("3.4E");
		case 390:	return _RP("3.5K");
		case 416:	return _RP("4.0J");
		case 417:	return _RP("4.0U");
		case 418:	return _RP("4.0E");
		case 448:	return _RP("4.1J");
		case 449:	return _RP("4.1U");
		case 450:	return _RP("4.1E");
		case 454:	return _RP("4.1K");
		case 480:	return _RP("4.2J");
		case 481:	return _RP("4.2U");
		case 482:	return _RP("4.2E");
		case 483:	return _RP("4.2K");
		case 512:	return _RP("4.3J");
		case 513:	return _RP("4.3U");
		case 514:	return _RP("4.3E");
		case 518:	return _RP("4.3K");

		// vWii
		// References:
		// - http://wiiubrew.org/wiki/Title_database
		// - https://yls8.mtheall.com/ninupdates/reports.php
		// NOTE: These are all listed as 4.3.
		// NOTE 2: vWii also has 512, 513, and 514.
		case 544:	return _RP("4.3J");
		case 545:	return _RP("4.3U");
		case 546:	return _RP("4.3E");
		case 608:	return _RP("4.3J");
		case 609:	return _RP("4.3U");
		case 610:	return _RP("4.3E");

		// Unknown version.
		default:	return nullptr;
	}
}

/**
 * Convert a GCN region value (from GCN_Boot_Info or RVL_RegionSetting) to a string.
 * @param gcnRegion GCN region value.
 * @param idRegion Game ID region.
 * @return String, or nullptr if the region value is invalid.
 */
const rp_char *GameCubePrivate::gcnRegionToString(unsigned int gcnRegion, char idRegion)
{
	/**
	 * There are two region codes for GCN/Wii games:
	 * - BI2.bin (GCN) or Age Rating (Wii)
	 * - Game ID
	 *
	 * The BI2.bin code is what's actually enforced.
	 * The Game ID may provide additional information.
	 *
	 * For games where the BI2.bin code matches the
	 * game ID region, only the BI2.bin region will
	 * be displayed. For others, if the game ID region
	 * is known, it will be printed as text, and the
	 * BI2.bin region will be abbreviated.
	 *
	 * Game ID reference:
	 * - https://github.com/dolphin-emu/dolphin/blob/4c9c4568460df91a38d40ac3071d7646230a8d0f/Source/Core/DiscIO/Enums.cpp
	 */
	switch (gcnRegion) {
		case GCN_REGION_JAPAN:
			switch (idRegion) {
				case 'J':	// Japan
				default:
					return _RP("Japan");

				case 'W':	// Taiwan
					return _RP("Taiwan (JPN)");
				case 'K':	// South Korea
				case 'T':	// South Korea with Japanese language
				case 'Q':	// South Korea with English language
					// FIXME: Is this combination possible?
					return _RP("South Korea (JPN)");
				case 'C':	// China (unofficial?)
					return _RP("China (JPN)");
			}

		case GCN_REGION_PAL:
			switch (idRegion) {
				case 'P':	// PAL
				case 'X':	// Multi-language release
				case 'Y':	// Multi-language release
				case 'L':	// Japanese import to PAL regions
				case 'M':	// Japanese import to PAL regions
				default:
					return _RP("Europe / Australia (PAL)");

				case 'D':	// Germany
					return _RP("Germany (PAL)");
				case 'F':	// France
					return _RP("France (PAL)");
				case 'H':	// Netherlands
					return _RP("Netherlands (PAL)");
				case 'I':	// Italy
					return _RP("Italy (PAL)");
				case 'R':	// Russia
					return _RP("Russia (PAL)");
				case 'S':	// Spain
					return _RP("Spain (PAL)");
				case 'U':	// Australia
					return _RP("Australia (PAL)");
			}

		// USA and South Korea regions don't have separate
		// subregions for other countries.
		case GCN_REGION_USA:
			// Possible game ID regions:
			// - E: USA
			// - N: Japanese import to USA and other NTSC regions.
			// - Z: Prince of Persia - The Forgotten Sands (Wii)
			// - B: Ufouria: The Saga (Virtual Console)
			return _RP("USA");

		case GCN_REGION_SOUTH_KOREA:
			// Possible game ID regions:
			// - K: South Korea
			// - Q: South Korea with Japanese language
			// - T: South Korea with English language
			return _RP("South Korea");

		default:
			break;
	}

	return nullptr;
}

/**
 * Convert a GCN region value (from GCN_Boot_Info or RVL_RegionSetting) to a GameTDB region code.
 * @param gcnRegion GCN region value.
 * @param idRegion Game ID region.
 *
 * NOTE: Mulitple GameTDB region codes may be returned including:
 * - User-specified fallback region. [TODO]
 * - General fallback region.
 *
 * @return GameTDB region code(s), or empty vector if the region value is invalid.
 */
vector<const char*> GameCubePrivate::gcnRegionToGameTDB(unsigned int gcnRegion, char idRegion)
{
	/**
	 * There are two region codes for GCN/Wii games:
	 * - BI2.bin (GCN) or Age Rating (Wii)
	 * - Game ID
	 *
	 * The BI2.bin code is what's actually enforced.
	 * The Game ID may provide additional information.
	 *
	 * For games where the BI2.bin code matches the
	 * game ID region, only the BI2.bin region will
	 * be displayed. For others, if the game ID region
	 * is known, it will be printed as text, and the
	 * BI2.bin region will be abbreviated.
	 *
	 * Game ID reference:
	 * - https://github.com/dolphin-emu/dolphin/blob/4c9c4568460df91a38d40ac3071d7646230a8d0f/Source/Core/DiscIO/Enums.cpp
	 */
	vector<const char*> ret;

	switch (gcnRegion) {
		case GCN_REGION_JAPAN:
			switch (idRegion) {
				case 'J':
					break;

				case 'W':	// Taiwan
					ret.push_back("ZHTW");
					break;
				case 'K':
				case 'T':	// South Korea with Japanese language
				case 'Q':	// South Korea with English language
					ret.push_back("KO");
					break;
				case 'C':	// China (unofficial?)
					ret.push_back("ZHCN");
					break;

				// Wrong region, but handle it anyway.
				case 'E':	// USA
					ret.push_back("US");
					break;
				case 'P':	// Europe (PAL)
				default:	// All others
					ret.push_back("EN");
					break;
			}
			ret.push_back("JA");
			break;

		case GCN_REGION_PAL:
			switch (idRegion) {
				case 'P':	// PAL
				case 'X':	// Multi-language release
				case 'Y':	// Multi-language release
				case 'L':	// Japanese import to PAL regions
				case 'M':	// Japanese import to PAL regions
				default:
					break;

				// NOTE: No GameID code for PT.
				// TODO: Implement user-specified fallbacks.

				case 'D':	// Germany
					ret.push_back("DE");
					break;
				case 'F':	// France
					ret.push_back("FR");
					break;
				case 'H':	// Netherlands
					ret.push_back("NL");
					break;
				case 'I':	// Italy
					ret.push_back("NL");
					break;
				case 'R':	// Russia
					ret.push_back("RU");
					break;
				case 'S':	// Spain
					ret.push_back("ES");
					break;
				case 'U':	// Australia
					ret.push_back("AU");
					break;

				// Wrong region, but handle it anyway.
				case 'E':	// USA
					ret.push_back("US");
					break;
				case 'J':	// Japan
					ret.push_back("JA");
					break;
			}
			ret.push_back("EN");
			break;

		// USA and South Korea regions don't have separate
		// subregions for other countries.
		case GCN_REGION_USA:
			// Possible game ID regions:
			// - E: USA
			// - N: Japanese import to USA and other NTSC regions.
			// - Z: Prince of Persia - The Forgotten Sands (Wii)
			// - B: Ufouria: The Saga (Virtual Console)
			switch (idRegion) {
				case 'E':
				default:
					break;

				// Wrong region, but handle it anyway.
				case 'P':	// Europe (PAL)
					ret.push_back("EN");
					break;
				case 'J':	// Japan
					ret.push_back("JA");
					break;
			}
			ret.push_back("US");
			break;

		case GCN_REGION_SOUTH_KOREA:
			// Possible game ID regions:
			// - K: South Korea
			// - Q: South Korea with Japanese language
			// - T: South Korea with English language
			ret.push_back("KO");
			break;

		default:
			break;
	}

	return ret;
}

/**
 * Load opening.bnr. (GameCube only)
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCubePrivate::gcn_loadOpeningBnr(void)
{
	assert(discReader != nullptr);

	// TODO: Do Triforce games have opening.bnr?
	if ((discType & DISC_SYSTEM_MASK) != DISC_SYSTEM_GCN &&
	    (discType & DISC_SYSTEM_MASK) != DISC_SYSTEM_TRIFORCE)
	{
		// No opening.bnr.
		return -ENOENT;
	}

	if (gcn_opening_bnr) {
		// Banner is already loaded.
		return 0;
	}

	// NOTE: We usually don't keep a GcnPartition open,
	// since we don't need to access more than one file.
	unique_ptr<GcnPartition> gcnPartition(new GcnPartition(discReader, 0));
	if (!gcnPartition || !gcnPartition->isOpen()) {
		// Could not open the partition.
		return -EIO;
	}

	unique_ptr<IRpFile> f_opening_bnr(gcnPartition->open(_RP("/opening.bnr")));
	if (!f_opening_bnr) {
		// Error opening "opening.bnr".
		return -gcnPartition->lastError();
	}

	// Static assertions for the banner structs.
	static_assert(sizeof(banner_comment_t) == GCN_BANNER_COMMENT_SIZE,
		"banner_comment_t is the wrong size. (Should be 320 bytes.)");
	static_assert(sizeof(banner_bnr1_t) == GCN_BANNER_BNR1_SIZE,
		"banner_bnr1_t is the wrong size. (Should be 6,496 bytes.)");
	static_assert(sizeof(banner_bnr2_t) == GCN_BANNER_BNR2_SIZE,
		"banner_bnr2_t is the wrong size. (Should be 8,096 bytes.)");

	// Always use a BNR2 pointer.
	// BNR1 and BNR2 have identical layouts, except
	// BNR2 has more comment fields.
	banner_bnr2_t *pBanner = nullptr;
	unsigned int banner_size = 0;

	// Read the magic number to determine what type of
	// opening.bnr file this is.
	uint32_t bnr_magic;
	size_t size = f_opening_bnr->read(&bnr_magic, sizeof(bnr_magic));
	if (size != sizeof(bnr_magic)) {
		// Read error.
		int err = f_opening_bnr->lastError();
		return (err != 0 ? -err : -EIO);
	}

	bnr_magic = be32_to_cpu(bnr_magic);
	switch (bnr_magic) {
		case BANNER_MAGIC_BNR1:
			banner_size = GCN_BANNER_BNR1_SIZE;
			break;
		case BANNER_MAGIC_BNR2:
			banner_size = GCN_BANNER_BNR2_SIZE;
			break;
		default:
			// Unknown magic.
			// TODO: Better error code?
			return -EIO;
	}

	// Load the full banner.
	// NOTE: Magic number is loaded as host-endian.
	pBanner = (banner_bnr2_t*)malloc(banner_size);
	if (!pBanner) {
		// ENOMEM
		return -ENOMEM;
	}
	pBanner->magic = bnr_magic;
	size = f_opening_bnr->read(&pBanner->reserved, banner_size-4);
	if (size != banner_size-4) {
		// Read error.
		// TODO: Allow smaller than "full" for BNR2?
		free(pBanner);
		int err = f_opening_bnr->lastError();
		return (err != 0 ? -err : -EIO);
	}

	// Banner loaded.
	gcn_opening_bnr = pBanner;
	return 0;
}

/**
* Get the banner_comment_t from opening.bnr.
 * For BNR2, this uses the comment that most
 * closely matches the host system language.
 * @return banner_comment_t, or nullptr if opening.bnr was not loaded.
 */
const banner_comment_t *GameCubePrivate::gcn_getBannerComment(void) const
{
	if (!gcn_opening_bnr) {
		// Attempt to load opening.bnr.
		if (const_cast<GameCubePrivate*>(this)->gcn_loadOpeningBnr() != 0) {
			// Error loading opening.bnr.
			return nullptr;
		}

		// Make sure it was actually loaded.
		if (!gcn_opening_bnr) {
			// opening.bnr was not loaded.
			return nullptr;
		}
	}

	// Check if this is BNR1 or BNR2.
	// BNR2 has language-specific fields.
	const banner_comment_t *comment = nullptr;
	if (gcn_opening_bnr->magic == 0x424E5232) {
		// Determine the system language.
		switch (SystemRegion::getLanguageCode()) {
			case 'en':
			default:
				// English. (default)
				// Used if the host system language
				// doesn't match any of the languages
				// supported by PAL GameCubes.
				comment = &gcn_opening_bnr->comments[GCN_PAL_LANG_ENGLISH];
				break;

			case 'de':
				comment = &gcn_opening_bnr->comments[GCN_PAL_LANG_GERMAN];
				break;
			case 'fr':
				comment = &gcn_opening_bnr->comments[GCN_PAL_LANG_FRENCH];
				break;
			case 'es':
				comment = &gcn_opening_bnr->comments[GCN_PAL_LANG_SPANISH];
				break;
			case 'it':
				comment = &gcn_opening_bnr->comments[GCN_PAL_LANG_ITALIAN];
				break;
			case 'nl':
				comment = &gcn_opening_bnr->comments[GCN_PAL_LANG_DUTCH];
				break;
		}

		// If all of the language-specific fields are empty,
		// revert to English.
		if (comment->gamename[0] == 0 &&
		    comment->company[0] == 0 &&
		    comment->gamename_full[0] == 0 &&
		    comment->company_full[0] == 0 &&
		    comment->gamedesc[0] == 0)
		{
			// Revert to English.
			comment = &gcn_opening_bnr->comments[GCN_PAL_LANG_ENGLISH];
		}
	} else {
		// BNR1 only has one banner comment.
		comment = &gcn_opening_bnr->comments[0];
	}

	return comment;
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
	: super(file, GameCubePrivate::gcn_fields, ARRAY_SIZE(GameCubePrivate::gcn_fields))
	, d(new GameCubePrivate(this))
{
	// This class handles disc images.
	m_fileType = FTYPE_DISC_IMAGE;

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
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for GCN.
	info.szFile = 0;	// Not needed for GCN.
	d->discType = isRomSupported_static(&info);

	// TODO: DiscReaderFactory?
	if (d->discType >= 0) {
		switch (d->discType & GameCubePrivate::DISC_FORMAT_MASK) {
			case GameCubePrivate::DISC_FORMAT_RAW:
				d->discReader = new DiscReader(m_file);
				break;
			case GameCubePrivate::DISC_FORMAT_WBFS:
				d->discReader = new WbfsReader(m_file);
				break;
			case GameCubePrivate::DISC_FORMAT_CISO:
				d->discReader = new CisoGcnReader(m_file);
				break;
			case GameCubePrivate::DISC_FORMAT_TGC: {
				m_fileType = FTYPE_EMBEDDED_DISC_IMAGE;

				// Check the TGC header for the disc offset.
				const GCN_TGC_Header *tgcHeader = reinterpret_cast<const GCN_TGC_Header*>(header);
				uint32_t gcm_offset = be32_to_cpu(tgcHeader->header_size);
				d->discReader = new DiscReader(m_file, gcm_offset, -1);
				break;
			}
			case GameCubePrivate::DISC_FORMAT_UNKNOWN:
			default:
				m_fileType = FTYPE_UNKNOWN;
				d->discType = GameCubePrivate::DISC_UNKNOWN;
				break;
		}
	}

	m_isValid = (d->discType >= 0);
	if (!m_isValid) {
		// Nothing else to do here.
		return;
	}

	// Save the disc header for later.
	d->discReader->rewind();
	size = d->discReader->read(&d->discHeader, sizeof(d->discHeader));
	if (size != sizeof(d->discHeader)) {
		// Error reading the disc header.
		delete d->discReader;
		d->discReader = nullptr;
		d->discType = GameCubePrivate::DISC_UNKNOWN;
		m_isValid = false;
		return;
	}

	if (d->discType != GameCubePrivate::DISC_UNKNOWN &&
	    ((d->discType & GameCubePrivate::DISC_SYSTEM_MASK) == GameCubePrivate::DISC_SYSTEM_UNKNOWN))
	{
		// isRomSupported() was unable to determine the
		// system type, possibly due to format limitations.
		// Examples:
		// - CISO doesn't store a copy of the disc header
		//   in range of the data we read.
		// - TGC has a 32 KB header before the embedded GCM.
		if (be32_to_cpu(d->discHeader.magic_wii) == WII_MAGIC) {
			// Wii disc image.
			d->discType &= ~GameCubePrivate::DISC_SYSTEM_MASK;
			d->discType |=  GameCubePrivate::DISC_SYSTEM_WII;
		} else if (be32_to_cpu(d->discHeader.magic_gcn) == GCN_MAGIC) {
			// GameCube disc image.
			// TODO: Check for Triforce?
			d->discType &= ~GameCubePrivate::DISC_SYSTEM_MASK;
			d->discType |=  GameCubePrivate::DISC_SYSTEM_GCN;
		} else if (!memcmp(&d->discHeader, GameCubePrivate::nddemo_header,
			    sizeof(GameCubePrivate::nddemo_header)))
		{
			// NDDEMO disc.
			d->discType &= ~GameCubePrivate::DISC_SYSTEM_MASK;
			d->discType |=  GameCubePrivate::DISC_SYSTEM_GCN;
		} else {
			// Unknown system type.
			delete d->discReader;
			d->discReader = nullptr;
			d->discType = GameCubePrivate::DISC_UNKNOWN;
			m_isValid = false;
			return;
		}
	}

	// Get the GCN region code. (bi2.bin or RVL_RegionSetting)
	switch (d->discType & GameCubePrivate::DISC_SYSTEM_MASK) {
		case GameCubePrivate::DISC_SYSTEM_GCN:
		case GameCubePrivate::DISC_SYSTEM_TRIFORCE: {	// TODO?
			GCN_Boot_Info bootInfo;	// TODO: Save in GameCubePrivate?
			d->discReader->seek(GCN_Boot_Info_ADDRESS);
			size = d->discReader->read(&bootInfo, sizeof(bootInfo));
			if (size != sizeof(bootInfo)) {
				// Cannot read bi2.bin.
				delete d->discReader;
				d->discReader = nullptr;
				d->discType = GameCubePrivate::DISC_UNKNOWN;
				m_isValid = false;
				return;
			}

			d->gcnRegion = be32_to_cpu(bootInfo.region_code);
			break;
		}

		case GameCubePrivate::DISC_SYSTEM_WII:
			d->discReader->seek(RVL_RegionSetting_ADDRESS);
			size = d->discReader->read(&d->regionSetting, sizeof(d->regionSetting));
			if (size != sizeof(d->regionSetting)) {
				// Cannot read RVL_RegionSetting.
				delete d->discReader;
				d->discReader = nullptr;
				d->discType = GameCubePrivate::DISC_UNKNOWN;
				m_isValid = false;
				return;
			}

			d->gcnRegion = be32_to_cpu(d->regionSetting.region_code);
			break;

		default:
			// Unknown system.
			delete d->discReader;
			d->discReader = nullptr;
			d->discType = GameCubePrivate::DISC_UNKNOWN;
			m_isValid = false;
			return;
	}
}

GameCube::~GameCube()
{
	delete d;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameCube::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(GCN_DiscHeader))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return GameCubePrivate::DISC_UNKNOWN;
	}

	// Check for the magic numbers.
	const GCN_DiscHeader *gcn_header = reinterpret_cast<const GCN_DiscHeader*>(info->header.pData);
	if (be32_to_cpu(gcn_header->magic_wii) == WII_MAGIC) {
		// Wii disc image.
		return (GameCubePrivate::DISC_SYSTEM_WII | GameCubePrivate::DISC_FORMAT_RAW);
	} else if (be32_to_cpu(gcn_header->magic_gcn) == GCN_MAGIC) {
		// GameCube disc image.
		// TODO: Check for Triforce?
		return (GameCubePrivate::DISC_SYSTEM_GCN | GameCubePrivate::DISC_FORMAT_RAW);
	}

	// Check for NDDEMO. (Early GameCube demo discs.)
	if (!memcmp(gcn_header, GameCubePrivate::nddemo_header, sizeof(GameCubePrivate::nddemo_header))) {
		// NDDEMO disc.
		return (GameCubePrivate::DISC_SYSTEM_GCN | GameCubePrivate::DISC_FORMAT_RAW);
	}

	// Check for sparse/compressed disc formats.
	// These are checked after the magic numbers in case some joker
	// decides to make a GCN or Wii disc image with the game ID "WBFS".

	// Check for WBFS.
	if (WbfsReader::isDiscSupported_static(info->header.pData, info->header.size) >= 0) {
		// Disc image is stored in "HDD" sector 1.
		unsigned int hdd_sector_size = (1 << info->header.pData[8]);
		if (info->header.size >= hdd_sector_size + 0x200) {
			// Check for Wii magic.
			// FIXME: GCN magic too?
			gcn_header = reinterpret_cast<const GCN_DiscHeader*>(&info->header.pData[hdd_sector_size]);
			if (be32_to_cpu(gcn_header->magic_wii) == WII_MAGIC) {
				// Wii disc image. (WBFS format)
				return (GameCubePrivate::DISC_SYSTEM_WII | GameCubePrivate::DISC_FORMAT_WBFS);
			}
		}
	}

	// Check for CISO.
	if (CisoGcnReader::isDiscSupported_static(info->header.pData, info->header.size) >= 0) {
		// CISO format doesn't store a copy of the disc header
		// at the beginning of the disc, so we can't check the
		// system format here.
		return GameCubePrivate::DISC_SYSTEM_UNKNOWN | GameCubePrivate::DISC_FORMAT_CISO;
	}

	// Check for TGC.
	const GCN_TGC_Header *const tgcHeader = reinterpret_cast<const GCN_TGC_Header*>(info->header.pData);
	if (be32_to_cpu(tgcHeader->tgc_magic) == TGC_MAGIC) {
		// TGC images have their own 32 KB header, so we can't
		// check the actual GCN/Wii header here.
		return GameCubePrivate::DISC_SYSTEM_UNKNOWN | GameCubePrivate::DISC_FORMAT_TGC;
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
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *GameCube::systemName(uint32_t type) const
{
	if (!m_isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GCN, Wii, and Triforce have the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"GameCube::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (short, long, abbreviation)
	// Bits 2-3: DISC_SYSTEM_MASK (GCN, Wii, Triforce)

	static const rp_char *const sysNames[16] = {
		// FIXME: "NGC" in Japan?
		_RP("Nintendo GameCube"), _RP("GameCube"), _RP("GCN"), nullptr,
		_RP("Nintendo/Sega/Namco Triforce"), _RP("Triforce"), _RP("TF"), nullptr,
		_RP("Nintendo Wii"), _RP("Wii"), _RP("Wii"), nullptr,
		nullptr, nullptr, nullptr, nullptr
	};

	const uint32_t idx = (type & SYSNAME_TYPE_MASK) | ((d->discType & 3) << 2);
	return sysNames[idx];
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
	static const rp_char *const exts[] = {
		_RP(".gcm"), _RP(".rvm"), _RP(".wbfs"),
		_RP(".ciso"), _RP(".cso"), _RP(".tgc"),
		//_RP(".iso"),	// TODO: Enable this?
	};
	return vector<const rp_char*>(exts, exts + ARRAY_SIZE(exts));
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
       return IMGBF_INT_BANNER | IMGBF_EXT_MEDIA;
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
	switch (d->gcnRegion) {
		case GCN_REGION_USA:
		case GCN_REGION_PAL:
		default:
			// USA/PAL uses cp1252.
			m_fields->addData_string(cp1252_to_rp_string(
				d->discHeader.game_title, sizeof(d->discHeader.game_title)));
			break;

		case GCN_REGION_JAPAN:
		case GCN_REGION_SOUTH_KOREA:
			// Japan uses Shift-JIS.
			m_fields->addData_string(cp1252_sjis_to_rp_string(
				d->discHeader.game_title, sizeof(d->discHeader.game_title)));
	}

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

	// Region code.
	// bi2.bin and/or RVL_RegionSetting is loaded in the constructor,
	// and the region code is stored in d->gcnRegion.
	const rp_char *region = d->gcnRegionToString(d->gcnRegion, d->discHeader.id4[3]);
	if (region) {
		m_fields->addData_string(region);
	} else {
		// Invalid region code.
		char buf[32];
		int len = snprintf(buf, sizeof(buf), "Unknown (0x%08X)", d->gcnRegion);
		if (len > (int)sizeof(buf))
			len = sizeof(buf);
		m_fields->addData_string(len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
	}

	if ((d->discType & GameCubePrivate::DISC_SYSTEM_MASK) !=
	    GameCubePrivate::DISC_SYSTEM_WII)
	{
		// GameCube-specific fields.

		// Game information from opening.bnr.
		const banner_comment_t *comment = d->gcn_getBannerComment();
		if (comment) {
			// cp1252/sjis comment data.
			// TODO: BNR2 is only cp1252.
			string comment_data;
			comment_data.reserve(sizeof(*comment));

			// Fields are not necessarily null-terminated.
			// NOTE: We're converting from cp1252 or Shift-JIS
			// *after* concatenating all the strings, which is
			// why we're using strnlen() here.
			int field_len;

			// Game name.
			if (comment->gamename_full[0] != 0) {
				field_len = (int)strnlen(comment->gamename_full, sizeof(comment->gamename_full));
				comment_data += string(comment->gamename_full, field_len);
				comment_data += '\n';
			} else if (comment->gamename[0] != 0) {
				field_len = (int)strnlen(comment->gamename, sizeof(comment->gamename));
				comment_data += string(comment->gamename, field_len);
				comment_data += '\n';
			}

			// Company.
			if (comment->company_full[0] != 0) {
				field_len = (int)strnlen(comment->company_full, sizeof(comment->company_full));
				comment_data += string(comment->company_full, field_len);
				comment_data += '\n';
			} else if (comment->company[0] != 0) {
				field_len = (int)strnlen(comment->company, sizeof(comment->company));
				comment_data += string(comment->company, field_len);
				comment_data += '\n';
			}

			// Game description.
			if (comment->gamedesc[0] != 0) {
				// Add a second newline if necessary.
				if (!comment_data.empty()) {
					comment_data += '\n';
				}

				field_len = (int)strnlen(comment->gamedesc, sizeof(comment->gamedesc));
				comment_data += string(comment->gamedesc, field_len);
			}

			// Remove trailing newlines.
			while (!comment_data.empty() && comment_data[comment_data.size()-1] == '\n') {
				comment_data.resize(comment_data.size()-1);
			}

			if (!comment_data.empty()) {
				// Show the comment data.
				switch (d->gcnRegion) {
					case GCN_REGION_USA:
					case GCN_REGION_PAL:
					default:
						// USA/PAL uses cp1252.
						m_fields->addData_string(
							cp1252_to_rp_string(comment_data.data(), (int)comment_data.size()));
						break;

					case GCN_REGION_JAPAN:
					case GCN_REGION_SOUTH_KOREA:
						// Japan uses Shift-JIS.
						m_fields->addData_string(
							cp1252_sjis_to_rp_string(comment_data.data(), (int)comment_data.size()));
						break;
				}
			} else {
				// No comment data.
				m_fields->addData_invalid();
			}
		} else {
			// opening.bnr was not loaded.
			m_fields->addData_invalid();
		}

		// Add dummy entries for Wii-specific fields.
		m_fields->addData_invalid();	// Age rating(s).
		m_fields->addData_invalid();	// System Update
		m_fields->addData_invalid();	// Partitions

		// Finished reading the field data.
		return (int)m_fields->count();
	}
	
	/** Wii-specific fields. **/

	// Add a dummy entry for the "Game Info" field.
	m_fields->addData_invalid();	// Game Info

	// Get age rating(s).
	// RVL_RegionSetting is loaded in the constructor.
	// TODO: Optimize this?
	ostringstream oss;
	static const char rating_organizations[16][8] = {
		"CERO", "ESRB", "", "USK",
		"PEGI", "MEKU", "PEGI-PT", "BBFC",
		"ACB", "GRB", "", "",
		"", "", "", ""
	};

	for (int i = 0; i < 16; i++) {
		// "Ratings" value is an age.
		// TODO: Decode to e.g. ESRB and CERO identifiers.
		// TODO: Handle PEGI before USK?
		if (rating_organizations[i][0] == 0)
			continue;

		// NOTE: This field also contains some flags:
		// - 0x80: Rating is unused.
		// - 0x20: Has online play. (TODO: Check PAL and JPN)
		if (d->regionSetting.ratings[i] < 0x80) {
			oss << rating_organizations[i] << "="
			    << (int)(d->regionSetting.ratings[i] & 0x1F);
			if (d->regionSetting.ratings[i] & 0x20) {
				// Indicate online play.
				// TODO: Explain this flag.
				// NOTE: Unicode U+00B0, encoded as UTF-8.
				oss << "\xC2\xB0";
			}
			oss << ", ";
		}
	}

	string str = oss.str();
	if (str.size() > 2) {
		// Remove the trailing ", ".
		str.resize(str.size() - 2);
	}

	if (!str.empty()) {
		m_fields->addData_string(utf8_to_rp_string(str.data(), (int)str.size()));
	} else {
		m_fields->addData_string(_RP("Unknown"));
	}

	// Load the Wii partition tables.
	int ret = d->loadWiiPartitionTables();
	if (ret == 0) {
		// Wii partition tables loaded.
		// Convert them to RFT_LISTDATA for display purposes.

		// Update version.
		const rp_char *sysMenu = nullptr;
		if (d->updatePartition) {
			// Find the RVL-WiiSystemmenu-v*.wad file.
			IFst::Dir *dirp = d->updatePartition->opendir(_RP("/_sys/"));
			if (dirp) {
				IFst::DirEnt *dirent;
				while ((dirent = d->updatePartition->readdir(dirp)) != nullptr) {
					if (dirent->name && dirent->type == DT_REG) {
						unsigned int version;
						// TODO: Optimize this?
						string u8str = rp_string_to_utf8(dirent->name);
						int ret = sscanf(u8str.c_str(), "RVL-WiiSystemmenu-v%u.wad", &version);
						if (ret == 1) {
							sysMenu = d->parseWiiSystemMenuVersion(version);
							break;
						}
					}
				}
				d->updatePartition->closedir(dirp);
			}
		}

		if (sysMenu) {
			m_fields->addData_string(sysMenu);
		} else {
			// TODO: Show specific errors if the system menu WAD wasn't found.
			// Missing encryption key, etc.
			if (!d->updatePartition) {
				sysMenu = _RP("None");
			} else {
				switch (d->updatePartition->encInitStatus()) {
					case WiiPartition::ENCINIT_DISABLED:
						sysMenu = _RP("ERROR: Decryption is disabled.");
						break;
					case WiiPartition::ENCINIT_INVALID_KEY_IDX:
						sysMenu = _RP("ERROR: Invalid common key index.");
						break;
					case WiiPartition::ENCINIT_NO_KEYFILE:
						sysMenu = _RP("ERROR: keys.conf was not found.");
						break;
					case WiiPartition::ENCINIT_MISSING_KEY:
						// TODO: Which key?
						sysMenu = _RP("ERROR: Required key was not found in keys.conf.");
						break;
					case WiiPartition::ENCINIT_CIPHER_ERROR:
						sysMenu = _RP("ERROR: Decryption library failed.");
						break;
					case WiiPartition::ENCINIT_INCORRECT_KEY:
						sysMenu = _RP("ERROR: Key is incorrect.");
						break;
					default:
						sysMenu = _RP("Unknown");
						break;
				}
			}

			m_fields->addData_string(sysMenu);
		}

		// Partition table.
		RomFields::ListData *partitions = new RomFields::ListData();

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
					case GameCubePrivate::PARTITION_GAME:
						str = _RP("Game");
						break;
					case GameCubePrivate::PARTITION_UPDATE:
						str = _RP("Update");
						break;
					case GameCubePrivate::PARTITION_CHANNEL:
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

				// Encryption key.
				const rp_char *key_name;
				switch (entry.partition->encKey()) {
					case WiiPartition::ENCKEY_UNKNOWN:
					default:
						key_name = _RP("Unknown");
						break;
					case WiiPartition::ENCKEY_COMMON:
						key_name = _RP("Retail");
						break;
					case WiiPartition::ENCKEY_KOREAN:
						key_name = _RP("Korean");
						break;
				}
				data_row.push_back(key_name);

				// Partition size.
				data_row.push_back(d->formatFileSize(entry.partition->partition_size()));

				// Add the partition information to the ListData.
				partitions->data.push_back(data_row);
			}
		}

		// Add the partitions list data.
		m_fields->addData_listData(partitions);
	} else {
		// Could not load partition tables.
		// FIXME: Show an error? For now, add dummy fields.
		m_fields->addData_invalid();		// Update
		m_fields->addData_invalid();		// Partitions
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
 * Load an internal image.
 * Called by RomData::image() if the image data hasn't been loaded yet.
 * @param imageType Image type to load.
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCube::loadInternalImage(ImageType imageType)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}

	if (m_images[imageType]) {
		// Icon *has* been loaded...
		return 0;
	} else if (!m_file) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid) {
		// Save file isn't valid.
		return -EIO;
	}

	// Check for supported image types.
	if (imageType != IMG_INT_BANNER) {
		// Only IMG_INT_BANNER is supported by GameCube.
		return -ENOENT;
	}

	// Load opening.bnr.
	// FIXME: Does Triforce have opening.bnr?
	if (d->gcn_loadOpeningBnr() != 0) {
		// Could not load opening.bnr.
		return -ENOENT;
	}

	// Use nearest-neighbor scaling when resizing.
	m_imgpf[imageType] = IMGPF_RESCALE_NEAREST;

	// Convert the banner from GameCube RGB5A3 to ARGB32.
	rp_image *banner = ImageDecoder::fromGcnRGB5A3(
		BANNER_IMAGE_W, BANNER_IMAGE_H,
		d->gcn_opening_bnr->banner, sizeof(d->gcn_opening_bnr->banner));
	if (!banner) {
		// Error converting the banner.
		return -EIO;
	}

	// Finished decoding the banner.
	m_images[imageType] = banner;
	return 0;
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

	if ((d->discType & GameCubePrivate::DISC_FORMAT_MASK) == GameCubePrivate::DISC_FORMAT_TGC) {
		// TGC game IDs aren't unique, so we can't get
		// an image URL that makes any sense.
		return 0;
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

	// Determine the GameTDB region code(s).
	vector<const char*> tdb_regions = d->gcnRegionToGameTDB(d->gcnRegion, d->discHeader.id4[3]);

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

	// Replace any non-printable characters with underscores.
	// (NDDEMO has ID6 "00\0E01".)
	for (int i = 0; i < 6; i++) {
		if (!isprint(id6[i])) {
			id6[i] = '_';
		}
	}

	// NOTE: GameTDB's Wii and GameCube disc and 3D cover scans have
	// alpha transparency. Hence, no image processing is required.
	m_imgpf[imageType] = 0;

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
			for (auto iter = tdb_regions.cbegin(); iter != tdb_regions.cend(); ++iter) {
				extURL.url = getURL_GameTDB("wii", s_discNum, *iter, id6);
				extURL.cache_key = getCacheKey("wii", s_discNum, *iter, id6);
				extURLs.push_back(extURL);
			}
		}
	}

	// First disc, or not a disc scan.
	for (auto iter = tdb_regions.cbegin(); iter != tdb_regions.cend(); ++iter)
	{
		extURL.url = getURL_GameTDB("wii", imageTypeName, *iter, id6);
		extURL.cache_key = getCacheKey("wii", imageTypeName, *iter, id6);
		extURLs.push_back(extURL);
	}

	// All URLs added.
	return (extURLs.empty() ? -ENOENT : 0);
}

}
