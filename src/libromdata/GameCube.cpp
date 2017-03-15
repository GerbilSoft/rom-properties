/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCube.cpp: Nintendo GameCube and Wii disc image reader.              *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "GameCube.hpp"
#include "RomData_p.hpp"

#include "data/NintendoPublishers.hpp"
#include "data/WiiSystemMenuVersion.hpp"
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

class GameCubePrivate : public RomDataPrivate
{
	public:
		GameCubePrivate(GameCube *q, IRpFile *file);
		virtual ~GameCubePrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(GameCubePrivate)

	public:
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

		// Wii opening.bnr. (IMET section)
		wii_imet_t *wii_opening_bnr;

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

		// Pointers to specific partitions within WiiPartTable.
		WiiPartition *updatePartition;
		WiiPartition *gamePartition;

		/**
		 * Load the Wii volume group and partition tables.
		 * Partition tables are loaded into wiiVgTbl[].
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadWiiPartitionTables(void);

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
		static vector<const char*> gcnRegionToGameTDB(unsigned int gcnRegion, char idRegion);

	public:
		/**
		 * Load opening.bnr. (GameCube only)
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int gcn_loadOpeningBnr(void);

		/**
		 * Load opening.bnr. (Wii only)
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int wii_loadOpeningBnr(void);

		/**
		 * Get the banner_comment_t from opening.bnr. (GameCube version)
		 * For BNR2, this uses the comment that most
		 * closely matches the host system language.
		 * @return banner_comment_t, or nullptr if opening.bnr was not loaded.
		 */
		const banner_comment_t *gcn_getBannerComment(void) const;

		/**
		 * Get the game name from opening.bnr. (Wii version)
		 * This uses the name that most closely matches
		 * the host system language.
		 * @return Game name, or empty string if opening.bnr was not loaded.
		 */
		rp_string wii_getBannerName(void) const;
};

/** GameCubePrivate **/

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

GameCubePrivate::GameCubePrivate(GameCube *q, IRpFile *file)
	: super(q, file)
	, discType(DISC_UNKNOWN)
	, discReader(nullptr)
	, gcn_opening_bnr(nullptr)
	, wii_opening_bnr(nullptr)
	, gcnRegion(~0)
	, wiiVgTblLoaded(false)
	, updatePartition(nullptr)
	, gamePartition(nullptr)
{
	// Clear the various structs.
	memset(&discHeader, 0, sizeof(discHeader));
	memset(&regionSetting, 0, sizeof(regionSetting));
}

GameCubePrivate::~GameCubePrivate()
{
	updatePartition = nullptr;
	gamePartition = nullptr;

	// Delete partition objects in wiiVgTbl[].
	// TODO: Check wiiVgTblLoaded?
	for (int i = ARRAY_SIZE(wiiVgTbl)-1; i >= 0; i--) {
		for (auto iter = wiiVgTbl[i].begin(); iter != wiiVgTbl[i].end(); ++iter) {
			delete iter->partition;
		}
	}

	free(gcn_opening_bnr);
	free(wii_opening_bnr);
	delete discReader;
}

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
	} else if (!this->file || !this->file->isOpen()) {
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
			} else if (entry.type == PARTITION_GAME && !gamePartition) {
				// Game partition.
				gamePartition = entry.partition;
			}
		}
	}

	// Done reading the partition tables.
	return 0;
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
 * Load opening.bnr. (GameCube version)
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCubePrivate::gcn_loadOpeningBnr(void)
{
	assert(discReader != nullptr);
	if (!discReader) {
		return -EIO;
	} else if ((discType & DISC_SYSTEM_MASK) != DISC_SYSTEM_GCN &&
		   (discType & DISC_SYSTEM_MASK) != DISC_SYSTEM_TRIFORCE)
	{
		// Not supported.
		// TODO: Do Triforce games have opening.bnr?
		return -ENOTSUP;
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

	// Banner is loaded.
	gcn_opening_bnr = pBanner;
	return 0;
}

/**
 * Load opening.bnr. (Wii version)
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCubePrivate::wii_loadOpeningBnr(void)
{
	assert(discReader != nullptr);
	assert((discType & DISC_SYSTEM_MASK) == DISC_SYSTEM_WII);
	if (!discReader) {
		return -EIO;
	} else if ((discType & DISC_SYSTEM_MASK) != DISC_SYSTEM_WII) {
		// Not supported.
		return -ENOTSUP;
	}

	if (wii_opening_bnr) {
		// Banner is already loaded.
		return 0;
	}

	if (!gamePartition) {
		// No game partition...
		return -ENOENT;
	}

	unique_ptr<IRpFile> f_opening_bnr(gamePartition->open(_RP("/opening.bnr")));
	if (!f_opening_bnr) {
		// Error opening "opening.bnr".
		return -gamePartition->lastError();
	}

	// Read the IMET struct.
	wii_imet_t *pBanner = static_cast<wii_imet_t*>(malloc(sizeof(*pBanner)));
	if (!pBanner) {
		return -ENOMEM;
	}
	size_t size = f_opening_bnr->read(pBanner, sizeof(*pBanner));
	if (size != sizeof(*pBanner)) {
		// Read error.
		free(pBanner);
		int err = f_opening_bnr->lastError();
		return (err != 0 ? -err : -EIO);
	}

	// Verify the IMET magic.
	if (be32_to_cpu(pBanner->magic) != WII_IMET_MAGIC) {
		// Magic is incorrect.
		// TODO: Better error code?
		free(pBanner);
		return -EIO;
	}

	// Banner is loaded.
	wii_opening_bnr = pBanner;
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
	if (gcn_opening_bnr->magic == BANNER_MAGIC_BNR2) {
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
	} else /*if (gcn_opening_bnr->magic == BANNER_MAGIC_BNR1)*/ {
		// BNR1 only has one banner comment.
		comment = &gcn_opening_bnr->comments[0];
	}

	return comment;
}

/**
 * Get the game name from opening.bnr. (Wii version)
 * This uses the name that most closely matches
 * the host system language.
 * @return Game name, or empty string if opening.bnr was not loaded.
 */
rp_string GameCubePrivate::wii_getBannerName(void) const
{
	if (!wii_opening_bnr) {
		// Attempt to load opening.bnr.
		if (const_cast<GameCubePrivate*>(this)->wii_loadOpeningBnr() != 0) {
			// Error loading opening.bnr.
			return rp_string();
		}

		// Make sure it was actually loaded.
		if (!wii_opening_bnr) {
			// opening.bnr was not loaded.
			return rp_string();
		}
	}

	// Determine the system language.
	const char16_t *game_name;
	switch (SystemRegion::getLanguageCode()) {
		case 'en':
		default:
			// English. (default)
			// Used if the host system language doesn't match
			// any of the languages supported by Wii.
			game_name = wii_opening_bnr->names[WII_LANG_ENGLISH];
			break;

		case 'ja':
			game_name = wii_opening_bnr->names[WII_LANG_JAPANESE];
			break;
		case 'de':
			game_name = wii_opening_bnr->names[WII_LANG_GERMAN];
			break;
		case 'fr':
			game_name = wii_opening_bnr->names[WII_LANG_FRENCH];
			break;
		case 'es':
			game_name = wii_opening_bnr->names[WII_LANG_SPANISH];
			break;
		case 'it':
			game_name = wii_opening_bnr->names[WII_LANG_ITALIAN];
			break;
		case 'nl':
			game_name = wii_opening_bnr->names[WII_LANG_DUTCH];
			break;
		case 'ko':
			game_name = wii_opening_bnr->names[WII_LANG_KOREAN];
			break;
	}

	// If the language-specific name is empty,
	// revert to English.
	if (game_name[0] == 0) {
		// Revert to English.
		game_name = wii_opening_bnr->names[WII_LANG_ENGLISH];
	}

	// Convert from UTF-16BE.
	return utf16be_to_rp_string(game_name, ARRAY_SIZE(wii_opening_bnr->names[0]));
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
	: super(new GameCubePrivate(this, file))
{
	// This class handles disc images.
	RP_D(GameCube);
	d->fileType = FTYPE_DISC_IMAGE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the disc header.
	uint8_t header[4096+256];
	d->file->rewind();
	size_t size = d->file->read(&header, sizeof(header));
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
				d->discReader = new DiscReader(d->file);
				break;
			case GameCubePrivate::DISC_FORMAT_WBFS:
				d->discReader = new WbfsReader(d->file);
				break;
			case GameCubePrivate::DISC_FORMAT_CISO:
				d->discReader = new CisoGcnReader(d->file);
				break;
			case GameCubePrivate::DISC_FORMAT_TGC: {
				d->fileType = FTYPE_EMBEDDED_DISC_IMAGE;

				// Check the TGC header for the disc offset.
				const GCN_TGC_Header *tgcHeader = reinterpret_cast<const GCN_TGC_Header*>(header);
				uint32_t gcm_offset = be32_to_cpu(tgcHeader->header_size);
				d->discReader = new DiscReader(d->file, gcm_offset, -1);
				break;
			}
			case GameCubePrivate::DISC_FORMAT_UNKNOWN:
			default:
				d->fileType = FTYPE_UNKNOWN;
				d->discType = GameCubePrivate::DISC_UNKNOWN;
				break;
		}
	}

	d->isValid = (d->discType >= 0);
	if (!d->isValid) {
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
		d->isValid = false;
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
			d->isValid = false;
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
				d->isValid = false;
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
				d->isValid = false;
				return;
			}

			d->gcnRegion = be32_to_cpu(d->regionSetting.region_code);
			break;

		default:
			// Unknown system.
			delete d->discReader;
			d->discReader = nullptr;
			d->discType = GameCubePrivate::DISC_UNKNOWN;
			d->isValid = false;
			return;
	}
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
	RP_D(const GameCube);
	if (!d->isValid || !isSystemNameTypeValid(type))
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

		// NOTE: May cause conflicts on Windows
		// if fallback handling isn't working.
		_RP(".iso"),
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
	return IMGBF_INT_BANNER | IMGBF_EXT_MEDIA |
	       IMGBF_EXT_COVER | IMGBF_EXT_COVER_3D |
	       IMGBF_EXT_COVER_FULL;
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
 * Get a list of all available image sizes for the specified image type.
 *
 * The first item in the returned vector is the "default" size.
 * If the width/height is 0, then an image exists, but the size is unknown.
 *
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
std::vector<RomData::ImageSizeDef> GameCube::supportedImageSizes_static(ImageType imageType)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return std::vector<ImageSizeDef>();
	}

	switch (imageType) {
		case IMG_INT_BANNER: {
			static const ImageSizeDef sz_INT_BANNER[] = {
				{nullptr, 96, 32, 0},
			};
			return vector<ImageSizeDef>(sz_INT_BANNER,
				sz_INT_BANNER + ARRAY_SIZE(sz_INT_BANNER));
		}
		case IMG_EXT_MEDIA: {
			static const ImageSizeDef sz_EXT_MEDIA[] = {
				{nullptr, 160, 160, 0},
			};
			return vector<ImageSizeDef>(sz_EXT_MEDIA,
				sz_EXT_MEDIA + ARRAY_SIZE(sz_EXT_MEDIA));
		}
		case IMG_EXT_COVER: {
			static const ImageSizeDef sz_EXT_COVER[] = {
				{nullptr, 160, 224, 0},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER,
				sz_EXT_COVER + ARRAY_SIZE(sz_EXT_COVER));
		}
		case IMG_EXT_COVER_3D: {
			static const ImageSizeDef sz_EXT_COVER_3D[] = {
				{nullptr, 176, 248, 0},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER_3D,
				sz_EXT_COVER_3D + ARRAY_SIZE(sz_EXT_COVER_3D));
		}
		case IMG_EXT_COVER_FULL: {
			static const ImageSizeDef sz_EXT_COVER_FULL[] = {
				{nullptr, 512, 340, 0},
				{"HQ", 1024, 680, 1},
			};
			return vector<ImageSizeDef>(sz_EXT_COVER_FULL,
				sz_EXT_COVER_FULL + ARRAY_SIZE(sz_EXT_COVER_FULL));
		}
		default:
			break;
	}

	// Unsupported image type.
	return std::vector<ImageSizeDef>();
}

/**
 * Get a list of all available image sizes for the specified image type.
 *
 * The first item in the returned vector is the "default" size.
 * If the width/height is 0, then an image exists, but the size is unknown.
 *
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
std::vector<RomData::ImageSizeDef> GameCube::supportedImageSizes(ImageType imageType) const
{
	return supportedImageSizes_static(imageType);
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GameCube::loadFieldData(void)
{
	RP_D(GameCube);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->discType < 0) {
		// Unknown disc type.
		return -EIO;
	}

	// Disc header is read in the constructor.
	// TODO: Reserve fewer fields for GCN?
	d->fields->reserve(10);	// Maximum of 10 fields.

	// TODO: Trim the titles. (nulls, spaces)
	// NOTE: The titles are dup()'d as C strings, so maybe not nulls.

	// Game title.
	// TODO: Is Shift-JIS actually permissible here?
	switch (d->gcnRegion) {
		case GCN_REGION_USA:
		case GCN_REGION_PAL:
		default:
			// USA/PAL uses cp1252.
			d->fields->addField_string(_RP("Title"),
				cp1252_to_rp_string(
					d->discHeader.game_title, sizeof(d->discHeader.game_title)));
			break;

		case GCN_REGION_JAPAN:
		case GCN_REGION_SOUTH_KOREA:
			// Japan uses Shift-JIS.
			d->fields->addField_string(_RP("Title"),
				cp1252_sjis_to_rp_string(
					d->discHeader.game_title, sizeof(d->discHeader.game_title)));
			break;
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
	d->fields->addField_string(_RP("Game ID"), latin1_to_rp_string(id6, 6));

	// Look up the publisher.
	const rp_char *publisher = NintendoPublishers::lookup(d->discHeader.company);
	d->fields->addField_string(_RP("Publisher"),
		publisher ? publisher : _RP("Unknown"));

	// Other fields.
	d->fields->addField_string_numeric(_RP("Disc #"),
		d->discHeader.disc_number+1, RomFields::FB_DEC);
	d->fields->addField_string_numeric(_RP("Revision"),
		d->discHeader.revision, RomFields::FB_DEC, 2);

	// Region code.
	// bi2.bin and/or RVL_RegionSetting is loaded in the constructor,
	// and the region code is stored in d->gcnRegion.
	const rp_char *region = d->gcnRegionToString(d->gcnRegion, d->discHeader.id4[3]);
	if (region) {
		d->fields->addField_string(_RP("Region"), region);
	} else {
		// Invalid region code.
		char buf[32];
		int len = snprintf(buf, sizeof(buf), "Unknown (0x%08X)", d->gcnRegion);
		if (len > (int)sizeof(buf))
			len = sizeof(buf);
		d->fields->addField_string(_RP("Region"),
			len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
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
						d->fields->addField_string(_RP("Game Info"),
							cp1252_to_rp_string(comment_data.data(), (int)comment_data.size()));
						break;

					case GCN_REGION_JAPAN:
					case GCN_REGION_SOUTH_KOREA:
						// Japan uses Shift-JIS.
						d->fields->addField_string(_RP("Game Info"),
							cp1252_sjis_to_rp_string(comment_data.data(), (int)comment_data.size()));
						break;
				}
			}
		}

		// Finished reading the field data.
		return (int)d->fields->count();
	}
	
	/** Wii-specific fields. **/

	// Load the Wii partition tables.
	int wiiPtLoaded = d->loadWiiPartitionTables();

	// Get the game name from opening.bnr.
	if (wiiPtLoaded == 0) {
		rp_string game_name = d->wii_getBannerName();
		if (!game_name.empty()) {
			d->fields->addField_string(_RP("Game Info"), game_name);
		}
	}

	// Get age rating(s).
	// RVL_RegionSetting is loaded in the constructor.
	// Note that not all 16 fields are present on GCN,
	// though the fields do match exactly, so no
	// mapping is necessary.
	RomFields::age_ratings_t age_ratings;
	// Valid ratings: 0-1, 3-9
	static const uint16_t valid_ratings = 0x3FB;

	for (int i = (int)age_ratings.size()-1; i >= 0; i--) {
		if (!(valid_ratings & (1 << i))) {
			// Rating is not applicable for GameCube.
			age_ratings[i] = 0;
			continue;
		}

		// GCN ratings field:
		// - 0x1F: Age rating.
		// - 0x20: Has online play if set.
		// - 0x80: Unused if set.
		const uint8_t rvl_rating = d->regionSetting.ratings[i];
		if (rvl_rating & 0x80) {
			// Rating is unused.
			age_ratings[i] = 0;
			continue;
		}
		// Set active | age value.
		age_ratings[i] = RomFields::AGEBF_ACTIVE | (rvl_rating & 0x1F);

		// Is "rating may change during online play" set?
		if (rvl_rating & 0x20) {
			age_ratings[i] |= RomFields::AGEBF_ONLINE_PLAY;
		}
	}
	d->fields->addField_ageRatings(_RP("Age Rating"), age_ratings);

	// Display the Wii partition tables.
	if (wiiPtLoaded == 0) {
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
							sysMenu = WiiSystemMenuVersion::lookup(version);
							break;
						}
					}
				}
				d->updatePartition->closedir(dirp);
			}
		}

		if (!sysMenu) {
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
		}
		d->fields->addField_string(_RP("Update"), sysMenu);

		// Partition table.
		auto partitions = new std::vector<std::vector<rp_string> >();
		int partition_count = 0;
		for (int i = 0; i < 4; i++) {
			partition_count += (int)d->wiiVgTbl[i].size();
		}
		partitions->resize(partition_count);

		partition_count = 0;
		for (int i = 0; i < 4; i++) {
			const int count = (int)d->wiiVgTbl[i].size();
			for (int j = 0; j < count; j++, partition_count++) {
				vector<rp_string> &data_row = partitions->at(partition_count);
				data_row.reserve(5);	// 5 fields per row.

				// Partition entry.
				const GameCubePrivate::WiiPartEntry &entry = d->wiiVgTbl[i].at(j);

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
					case WiiPartition::ENCKEY_DEBUG:
						key_name = _RP("Debug");
						break;
				}
				data_row.push_back(key_name);

				// Used size.
				data_row.push_back(d->formatFileSize(entry.partition->partition_size_used()));

				// Partition size.
				data_row.push_back(d->formatFileSize(entry.partition->partition_size()));
			}
		}

		// Fields.
		static const rp_char *const partitions_names[] = {
			_RP("#"), _RP("Type"), _RP("Key"),
			_RP("Used Size"), _RP("Total Size")
		};
		vector<rp_string> *v_partitions_names = RomFields::strArrayToVector(
			partitions_names, ARRAY_SIZE(partitions_names));

		// Add the partitions list data.
		d->fields->addField_listData(_RP("Partitions"), v_partitions_names, partitions);
	} else {
		// Could not load partition tables.
		// FIXME: Show an error?
	}

	// Finished reading the field data.
	return (int)d->fields->count();
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

	RP_D(GameCube);
	if (d->images[imageType]) {
		// Icon *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Save file isn't valid.
		return -EIO;
	}

	// Check for supported image types.
	if (imageType != IMG_INT_BANNER) {
		// Only IMG_INT_BANNER is supported by GameCube.
		return -ENOENT;
	}

	// Load opening.bnr. (GCN/Triforce only)
	// FIXME: Does Triforce have opening.bnr?
	if (d->gcn_loadOpeningBnr() != 0) {
		// Could not load opening.bnr.
		return -ENOENT;
	}

	// Use nearest-neighbor scaling when resizing.
	d->imgpf[imageType] = IMGPF_RESCALE_NEAREST;

	// Convert the banner from GameCube RGB5A3 to ARGB32.
	rp_image *banner = ImageDecoder::fromGcnRGB5A3(
		BANNER_IMAGE_W, BANNER_IMAGE_H,
		d->gcn_opening_bnr->banner, sizeof(d->gcn_opening_bnr->banner));
	if (!banner) {
		// Error converting the banner.
		return -EIO;
	}

	// Finished decoding the banner.
	d->images[imageType] = banner;
	return 0;
}

/**
 * Get the imgpf value for external media types.
 * @param imageType Image type to load.
 * @return imgpf value.
 */
uint32_t GameCube::imgpf_extURL(ImageType imageType) const
{
	assert(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_EXT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}

	// NOTE: GameTDB's Wii and GameCube disc and 3D cover scans have
	// alpha transparency. Hence, no image processing is required.
	return 0;
}

/**
 * Get a list of URLs for an external image type.
 *
 * A thumbnail size may be requested from the shell.
 * If the subclass supports multiple sizes, it should
 * try to get the size that most closely matches the
 * requested size.
 *
 * @param imageType	[in]     Image type.
 * @param pExtURLs	[out]    Output vector.
 * @param size		[in,opt] Requested image size. This may be a requested
 *                               thumbnail size in pixels, or an ImageSizeType
 *                               enum value.
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCube::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	assert(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_EXT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return -ERANGE;
	}
	assert(pExtURLs != nullptr);
	if (!pExtURLs) {
		// No vector.
		return -EINVAL;
	}
	pExtURLs->clear();

	RP_D(const GameCube);
	if ((d->discType & GameCubePrivate::DISC_FORMAT_MASK) == GameCubePrivate::DISC_FORMAT_TGC) {
		// TGC game IDs aren't unique, so we can't get
		// an image URL that makes any sense.
		return -ENOENT;
	}

	// Check for known unusable game IDs.
	// - RELSAB: Generic ID used for prototypes.
	if (!memcmp(d->discHeader.id6, "RELSAB", 6)) {
		// Cannot download images for this game ID.
		return -ENOENT;
	}

	if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (d->discType < 0) {
		// Unknown disc type.
		return -EIO;
	}

	// Get the image sizes and sort them based on the
	// requested image size.
	vector<ImageSizeDef> sizeDefs = supportedImageSizes(imageType);
	if (sizeDefs.empty()) {
		// No image sizes.
		return -ENOENT;
	}

	// Select the best size.
	const ImageSizeDef *sizeDef = d->selectBestSize(sizeDefs, size);
	if (!sizeDef) {
		// No size available...
		return -ENOENT;
	}

	// NOTE: Only downloading the first size as per the
	// sort order, since GameTDB basically guarantees that
	// all supported sizes for an image type are available.
	// TODO: Add cache keys for other sizes in case they're
	// downloaded and none of these are available?

	// Determine the image type name.
	const char *imageTypeName_base;
	switch (imageType) {
		case IMG_EXT_MEDIA:
			imageTypeName_base = "disc";
			break;
		case IMG_EXT_COVER:
			imageTypeName_base = "cover";
			break;
		case IMG_EXT_COVER_3D:
			imageTypeName_base = "cover3D";
			break;
		case IMG_EXT_COVER_FULL:
			imageTypeName_base = "coverfull";
			break;
		default:
			// Unsupported image type.
			return -ENOENT;
	}
	// Current image type.
	char imageTypeName[16];
	snprintf(imageTypeName, sizeof(imageTypeName), "%s%s",
		 imageTypeName_base, (sizeDef->name ? sizeDef->name : ""));

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

	// ExtURLs.
	// TODO: If multiple image sizes are added, add the
	// "default" size to the end of ExtURLs in case the
	// user has high-resolution downloads disabled.
	pExtURLs->reserve(4);

	// Disc scan: Is this not the first disc?
	if (imageType == IMG_EXT_MEDIA &&
	    d->discHeader.disc_number > 0)
	{
		// Disc 2 (or 3, or 4...)
		// Request the disc 2 image first.
		char discName[16];
		snprintf(discName, sizeof(discName), "%s%u",
			 imageTypeName, d->discHeader.disc_number+1);

		for (auto iter = tdb_regions.cbegin(); iter != tdb_regions.cend(); ++iter) {
			int idx = (int)pExtURLs->size();
			pExtURLs->resize(idx+1);
			auto &extURL = pExtURLs->at(idx);

			extURL.url = d->getURL_GameTDB("wii", discName, *iter, id6, ".png");
			extURL.cache_key = d->getCacheKey_GameTDB("wii", discName, *iter, id6, ".png");
			extURL.width = sizeDef->width;
			extURL.height = sizeDef->height;
		}
	}

	// First disc, or not a disc scan.
	for (auto iter = tdb_regions.cbegin(); iter != tdb_regions.cend(); ++iter) {
		int idx = (int)pExtURLs->size();
		pExtURLs->resize(idx+1);
		auto &extURL = pExtURLs->at(idx);

		extURL.url = d->getURL_GameTDB("wii", imageTypeName, *iter, id6, ".png");
		extURL.cache_key = d->getCacheKey_GameTDB("wii", imageTypeName, *iter, id6, ".png");
		extURL.width = sizeDef->width;
		extURL.height = sizeDef->height;
		extURL.high_res = false;	// Only one size is available.
	}

	// All URLs added.
	return 0;
}

}
