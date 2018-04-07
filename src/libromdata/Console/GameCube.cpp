/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCube.cpp: Nintendo GameCube and Wii disc image reader.              *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#include "GameCube.hpp"
#include "librpbase/RomData_p.hpp"

#include "gcn_structs.h"
#include "gcn_banner.h"
#include "data/NintendoPublishers.hpp"
#include "data/WiiSystemMenuVersion.hpp"
#include "data/NintendoLanguage.hpp"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/SystemRegion.hpp"
#include "librpbase/file/IRpFile.hpp"

#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/ImageDecoder.hpp"

#include "libi18n/i18n.h"
using namespace LibRpBase;

// DiscReader
#include "librpbase/disc/DiscReader.hpp"
#include "disc/WbfsReader.hpp"
#include "disc/CisoGcnReader.hpp"
#include "disc/WiiPartition.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(GameCube)
ROMDATA_IMPL_IMG(GameCube)

class GameCubePrivate : public RomDataPrivate
{
	public:
		GameCubePrivate(GameCube *q, IRpFile *file);
		virtual ~GameCubePrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(GameCubePrivate)

	public:
		// Internal images.
		rp_image *img_banner;

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
			DISC_FORMAT_TGC  = (1 << 8),	// TGC (embedded disc image) (GCN only?)
			DISC_FORMAT_WBFS = (2 << 8),	// WBFS image. (Wii only)
			DISC_FORMAT_CISO = (3 << 8),	// CISO image.
			DISC_FORMAT_WIA  = (4 << 8),	// WIA image. (Header only!)
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
			uint8_t vg;		// Volume group number.
			uint8_t pt;		// Partition number.

			int64_t start;		// Starting address, in bytes.
			int64_t size;		// Estimated partition size, in bytes.

			uint32_t type;		// Partition type. (See WiiPartitionType.)
			WiiPartition *partition;	// Partition object.
		};

		std::vector<WiiPartEntry> wiiPtbl;
		bool wiiPtblLoaded;

		// Pointers to specific partitions within wiiPtbl.
		WiiPartition *updatePartition;
		WiiPartition *gamePartition;

		/**
		 * Load the Wii volume group and partition tables.
		 * Partition tables are loaded into wiiPtbl.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadWiiPartitionTables(void);

		/**
		 * Convert a GCN region value (from GCN_Boot_Info or RVL_RegionSetting) to a string.
		 * @param gcnRegion GCN region value.
		 * @param idRegion Game ID region.
		 * @return String, or nullptr if the region value is invalid.
		 */
		static const char *gcnRegionToString(unsigned int gcnRegion, char idRegion);

		/**
		 * Convert a GCN region value (from GCN_Boot_Info or RVL_RegionSetting) to a GameTDB region code.
		 * @param gcnRegion GCN region value.
		 * @param idRegion Game ID region.
		 *
		 * NOTE: Mulitple GameTDB region codes may be returned, including:
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
		string wii_getBannerName(void) const;

		/**
		 * Get the encryption status of a partition.
		 *
		 * This is used to check if the encryption keys are available
		 * for a partition, or if not, why not.
		 *
		 * @param partition Partition to check.
		 * @return nullptr if partition is readable; error message if not.
		 */
		const char *wii_getCryptoStatus(const WiiPartition *partition);
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
	, img_banner(nullptr)
	, discType(DISC_UNKNOWN)
	, discReader(nullptr)
	, gcn_opening_bnr(nullptr)
	, wii_opening_bnr(nullptr)
	, gcnRegion(~0)
	, wiiPtblLoaded(false)
	, updatePartition(nullptr)
	, gamePartition(nullptr)
{
	// Clear the various structs.
	memset(&discHeader, 0, sizeof(discHeader));
	memset(&regionSetting, 0, sizeof(regionSetting));
}

GameCubePrivate::~GameCubePrivate()
{
	// Internal images.
	delete img_banner;

	updatePartition = nullptr;
	gamePartition = nullptr;

	// Clear the existing partition table vector.
	for (auto iter = wiiPtbl.cbegin(); iter != wiiPtbl.cend(); ++iter) {
		delete iter->partition;
	}
	wiiPtbl.clear();

	delete gcn_opening_bnr;
	delete wii_opening_bnr;
	delete discReader;
}

/**
 * Load the Wii volume group and partition tables.
 * Partition tables are loaded into wiiPtbl.
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCubePrivate::loadWiiPartitionTables(void)
{
	if (wiiPtblLoaded) {
		// Partition tables have already been loaded.
		return 0;
	} else if (!this->file || !this->file->isOpen() || !this->discReader) {
		// File isn't open.
		return -EBADF;
	} else if ((discType & DISC_SYSTEM_MASK) != DISC_SYSTEM_WII) {
		// Unsupported disc type.
		return -EIO;
	}

	// Clear the existing partition table vector.
	for (auto iter = wiiPtbl.cbegin(); iter != wiiPtbl.cend(); ++iter) {
		delete iter->partition;
	}
	wiiPtbl.clear();

	// Assuming a maximum of 128 partitions per table.
	// (This is a rather high estimate.)
	RVL_VolumeGroupTable vgtbl;
	RVL_PartitionTableEntry pt[1024];

	// Read the volume group table.
	// References:
	// - http://wiibrew.org/wiki/Wii_Disc#Partitions_information
	// - http://blog.delroth.net/2011/06/reading-wii-discs-with-python/
	size_t size = discReader->seekAndRead(RVL_VolumeGroupTable_ADDRESS, &vgtbl, sizeof(vgtbl));
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

	// Check for NoCrypt.
	// TODO: Check both hash_verify and disc_noCrypt.
	// Dolphin only checks hash_verify.
	const bool noCrypt = (discHeader.hash_verify != 0);

	// Process each volume group.
	for (unsigned int i = 0; i < 4; i++) {
		unsigned int count = be32_to_cpu(vgtbl.vg[i].count);
		if (count == 0) {
			continue;
		} else if (count > ARRAY_SIZE(pt)) {
			count = ARRAY_SIZE(pt);
		}

		// Read the partition table entries.
		int64_t pt_addr = (int64_t)(be32_to_cpu(vgtbl.vg[i].addr)) << 2;
		const size_t ptSize = sizeof(RVL_PartitionTableEntry) * count;
		size = discReader->seekAndRead(pt_addr, pt, ptSize);
		if (size != ptSize) {
			// Error reading the partition table entries.
			return -EIO;
		}

		// Process each partition table entry.
		size_t idx = wiiPtbl.size();
		wiiPtbl.resize(wiiPtbl.size() + count);
		for (unsigned int j = 0; j < count; j++, idx++) {
			WiiPartEntry &entry = wiiPtbl.at(idx);

			entry.vg = (uint8_t)i;
			entry.pt = (uint8_t)j;
			entry.start = (int64_t)(be32_to_cpu(pt[j].addr)) << 2;
			entry.type = be32_to_cpu(pt[j].type);
		}
	}

	// Sort partitions by starting address in order to calculate the sizes.
	std::sort(wiiPtbl.begin(), wiiPtbl.end(),
		[](const WiiPartEntry &a, const WiiPartEntry &b) {
			return (a.start < b.start);
		}
	);

	// Calculate the size values.
	// Technically not needed for retail and RVT-R images, but unencrypted
	// images on RVT-H systems don't have the partition size set in the
	// partition header.
	size_t pt_idx;
	for (pt_idx = 0; pt_idx < wiiPtbl.size()-1; pt_idx++) {
		wiiPtbl[pt_idx].size = wiiPtbl[pt_idx+1].start - wiiPtbl[pt_idx].size;
	}
	// Last partition.
	wiiPtbl[pt_idx].size = discSize - wiiPtbl[pt_idx].start;

	// Restore the original sorting order. (VG#, then PT#)
	std::sort(wiiPtbl.begin(), wiiPtbl.end(),
		[](const WiiPartEntry &a, const WiiPartEntry &b) {
			return (a.vg < b.vg || a.pt < b.pt);
		}
	);

	// Create the WiiPartition objects.
	for (auto iter = wiiPtbl.begin(); iter != wiiPtbl.end(); ++iter) {
		iter->partition = new WiiPartition(discReader, iter->start, iter->size, noCrypt);

		if (iter->type == PARTITION_UPDATE && !updatePartition) {
			// System Update partition.
			updatePartition = iter->partition;
		} else if (iter->type == PARTITION_GAME && !gamePartition) {
			// Game partition.
			gamePartition = iter->partition;
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
const char *GameCubePrivate::gcnRegionToString(unsigned int gcnRegion, char idRegion)
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
					return C_("Region", "Japan");

				case 'W':	// Taiwan
					return C_("Region|GameCube", "Taiwan (JPN)");
				case 'K':	// South Korea
				case 'T':	// South Korea with Japanese language
				case 'Q':	// South Korea with English language
					// FIXME: Is this combination possible?
					return C_("Region|GameCube", "South Korea (JPN)");
				case 'C':	// China (unofficial?)
					return C_("Region|GameCube", "China (JPN)");
			}

		case GCN_REGION_PAL:
			switch (idRegion) {
				case 'P':	// PAL
				case 'X':	// Multi-language release
				case 'Y':	// Multi-language release
				case 'L':	// Japanese import to PAL regions
				case 'M':	// Japanese import to PAL regions
				default:
					return C_("Region|GameCube", "Europe / Australia (PAL)");

				case 'D':	// Germany
					return C_("Region|GameCube", "Germany (PAL)");
				case 'F':	// France
					return C_("Region|GameCube", "France (PAL)");
				case 'H':	// Netherlands
					return C_("Region|GameCube", "Netherlands (PAL)");
				case 'I':	// Italy
					return C_("Region|GameCube", "Italy (PAL)");
				case 'R':	// Russia
					return C_("Region|GameCube", "Russia (PAL)");
				case 'S':	// Spain
					return C_("Region|GameCube", "Spain (PAL)");
				case 'U':	// Australia
					return C_("Region|GameCube", "Australia (PAL)");
			}

		// USA and South Korea regions don't have separate
		// subregions for other countries.
		case GCN_REGION_USA:
			// Possible game ID regions:
			// - E: USA
			// - N: Japanese import to USA and other NTSC regions.
			// - Z: Prince of Persia - The Forgotten Sands (Wii)
			// - B: Ufouria: The Saga (Virtual Console)
			return C_("Region", "USA");

		case GCN_REGION_SOUTH_KOREA:
			// Possible game ID regions:
			// - K: South Korea
			// - Q: South Korea with Japanese language
			// - T: South Korea with English language
			return C_("Region", "South Korea");

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
 * NOTE: Mulitple GameTDB region codes may be returned, including:
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
			// Invalid gcnRegion. Use the game ID by itself.
			// (Usually happens if we can't read BI2.bin,
			// e.g. in WIA images.)
			bool addEN = false;
			switch (idRegion) {
				case 'E':	// USA
					ret.push_back("US");
					break;
				case 'P':	// Europe (PAL)
					addEN = true;
					break;
				case 'J':	// Japan
					ret.push_back("JA");
					break;
				case 'W':	// Taiwan
					ret.push_back("ZHTW");
					break;
				case 'K':	// South Korea
				case 'T':	// South Korea with Japanese language
				case 'Q':	// South Korea with English language
					ret.push_back("KO");
					break;
				case 'C':	// China (unofficial?)
					ret.push_back("ZHCN");
					break;

				/** PAL regions **/
				case 'D':	// Germany
					ret.push_back("DE");
					addEN = true;
					break;
				case 'F':	// France
					ret.push_back("FR");
					addEN = true;
					break;
				case 'H':	// Netherlands
					ret.push_back("NL");
					addEN = true;
					break;
				case 'I':	// Italy
					ret.push_back("NL");
					addEN = true;
					break;
				case 'R':	// Russia
					ret.push_back("RU");
					addEN = true;
					break;
				case 'S':	// Spain
					ret.push_back("ES");
					addEN = true;
					break;
				case 'U':	// Australia
					ret.push_back("AU");
					addEN = true;
					break;
			}
			if (addEN) {
				ret.push_back("EN");
			}
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

	unique_ptr<IRpFile> f_opening_bnr(gcnPartition->open("/opening.bnr"));
	if (!f_opening_bnr) {
		// Error opening "opening.bnr".
		return -gcnPartition->lastError();
	}

	// Determine the banner size.
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
	// NOTE: Always use a BNR2 pointer.
	//       BNR1 and BNR2 have identical layouts, except
	//       BNR2 has more comment fields.
	// NOTE 2: Magic number is loaded as host-endian.
	unique_ptr<banner_bnr2_t> pBanner(new banner_bnr2_t);
	pBanner->magic = bnr_magic;
	size = f_opening_bnr->read(&pBanner->reserved, banner_size-4);
	if (size != banner_size-4) {
		// Read error.
		int err = f_opening_bnr->lastError();
		return (err != 0 ? -err : -EIO);
	}

	// Banner is loaded.
	gcn_opening_bnr = pBanner.release();
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

	unique_ptr<IRpFile> f_opening_bnr(gamePartition->open("/opening.bnr"));
	if (!f_opening_bnr) {
		// Error opening "opening.bnr".
		return -gamePartition->lastError();
	}

	// Read the IMET struct.
	unique_ptr<wii_imet_t> pBanner(new wii_imet_t);
	if (!pBanner) {
		return -ENOMEM;
	}
	size_t size = f_opening_bnr->read(pBanner.get(), sizeof(*pBanner));
	if (size != sizeof(*pBanner)) {
		// Read error.
		int err = f_opening_bnr->lastError();
		return (err != 0 ? -err : -EIO);
	}

	// Verify the IMET magic.
	if (pBanner->magic != cpu_to_be32(WII_IMET_MAGIC)) {
		// Magic is incorrect.
		// TODO: Better error code?
		return -EIO;
	}

	// Banner is loaded.
	wii_opening_bnr = pBanner.release();
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
		// Get the system language.
		const int lang = NintendoLanguage::getGcnPalLanguage();
		comment = &gcn_opening_bnr->comments[lang];

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
string GameCubePrivate::wii_getBannerName(void) const
{
	if (!wii_opening_bnr) {
		// Attempt to load opening.bnr.
		if (const_cast<GameCubePrivate*>(this)->wii_loadOpeningBnr() != 0) {
			// Error loading opening.bnr.
			return string();
		}

		// Make sure it was actually loaded.
		if (!wii_opening_bnr) {
			// opening.bnr was not loaded.
			return string();
		}
	}

	// Get the system language.
	// TODO: Verify against the region code somehow?
	int lang = NintendoLanguage::getWiiLanguage();

	// If the language-specific name is empty,
	// revert to English.
	if (wii_opening_bnr->names[lang][0][0] == 0) {
		// Revert to English.
		lang = WII_LANG_ENGLISH;
	}

	// NOTE: The banner may have two lines.
	// Each line is a maximum of 21 characters.
	// Convert from UTF-16 BE and split into two lines at the same time.
	string info = utf16be_to_utf8(wii_opening_bnr->names[lang][0], 21);
	if (wii_opening_bnr->names[lang][1][0] != 0) {
		info += '\n';
		info += utf16be_to_utf8(wii_opening_bnr->names[lang][1], 21);
	}
	return info;
}

/**
 * Get the encryption status of a partition.
 *
 * This is used to check if the encryption keys are available
 * for a partition, or if not, why not.
 *
 * @param partition Partition to check.
 * @return nullptr if partition is readable; error message if not.
 */
const char *GameCubePrivate::wii_getCryptoStatus(const WiiPartition *partition)
{
	const KeyManager::VerifyResult res = partition->verifyResult();
	if (res == KeyManager::VERIFY_KEY_NOT_FOUND) {
		// This may be an invalid key index.
		if (partition->encKey() == WiiPartition::ENCKEY_UNKNOWN) {
			// Invalid key index.
			return C_("GameCube", "ERROR: Invalid common key index.");
		}
	}

	const char *err = KeyManager::verifyResultToString(res);
	if (!err) {
		err = C_("GameCube", "ERROR: Unknown error. (THIS IS A BUG!)");
	}
	return err;
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
	d->className = "GameCube";
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
			case GameCubePrivate::DISC_FORMAT_TGC: {
				d->fileType = FTYPE_EMBEDDED_DISC_IMAGE;

				// Check the TGC header for the disc offset.
				const GCN_TGC_Header *tgcHeader = reinterpret_cast<const GCN_TGC_Header*>(header);
				uint32_t gcm_offset = be32_to_cpu(tgcHeader->header_size);
				d->discReader = new DiscReader(d->file, gcm_offset, -1);
				break;
			}
			case GameCubePrivate::DISC_FORMAT_WBFS:
				d->discReader = new WbfsReader(d->file);
				break;
			case GameCubePrivate::DISC_FORMAT_CISO:
				d->discReader = new CisoGcnReader(d->file);
				break;
			case GameCubePrivate::DISC_FORMAT_WIA:
				// TODO: Implement WiaReader.
				// For now, only the header will be readable.
				d->discReader = nullptr;
				break;
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

	if (!d->discReader) {
		// No WiaReader yet. If this is WIA,
		// retrieve the header from header[].
		if ((d->discType & GameCubePrivate::DISC_FORMAT_MASK) == GameCubePrivate::DISC_FORMAT_WIA) {
			// GCN/Wii header starts at 0x58.
			memcpy(&d->discHeader, &header[0x58], sizeof(d->discHeader));
			return;
		} else {
			// Non-WIA formats must have a valid DiscReader.
			d->discType = GameCubePrivate::DISC_UNKNOWN;
			d->isValid = false;
			return;
		}
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
		if (d->discHeader.magic_wii == cpu_to_be32(WII_MAGIC)) {
			// Wii disc image.
			d->discType &= ~GameCubePrivate::DISC_SYSTEM_MASK;
			d->discType |=  GameCubePrivate::DISC_SYSTEM_WII;
		} else if (d->discHeader.magic_gcn == cpu_to_be32(GCN_MAGIC)) {
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
			size = d->discReader->seekAndRead(GCN_Boot_Info_ADDRESS, &bootInfo, sizeof(bootInfo));
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
			size = d->discReader->seekAndRead(RVL_RegionSetting_ADDRESS, &d->regionSetting, sizeof(d->regionSetting));
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
	if (gcn_header->magic_wii == cpu_to_be32(WII_MAGIC)) {
		// Wii disc image.
		return (GameCubePrivate::DISC_SYSTEM_WII | GameCubePrivate::DISC_FORMAT_RAW);
	} else if (gcn_header->magic_gcn == cpu_to_be32(GCN_MAGIC)) {
		// GameCube disc image.
		// TODO: Check for Triforce?
		return (GameCubePrivate::DISC_SYSTEM_GCN | GameCubePrivate::DISC_FORMAT_RAW);
	}

	// Check for NDDEMO. (Early GameCube demo discs.)
	if (!memcmp(gcn_header, GameCubePrivate::nddemo_header, sizeof(GameCubePrivate::nddemo_header))) {
		// NDDEMO disc.
		return (GameCubePrivate::DISC_SYSTEM_GCN | GameCubePrivate::DISC_FORMAT_RAW);
	}

	// Check for TGC.
	const GCN_TGC_Header *const tgcHeader = reinterpret_cast<const GCN_TGC_Header*>(info->header.pData);
	if (tgcHeader->tgc_magic == cpu_to_be32(TGC_MAGIC)) {
		// TGC images have their own 32 KB header, so we can't
		// check the actual GCN/Wii header here.
		return (GameCubePrivate::DISC_SYSTEM_UNKNOWN | GameCubePrivate::DISC_FORMAT_TGC);
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
			if (gcn_header->magic_wii == cpu_to_be32(WII_MAGIC)) {
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
		return (GameCubePrivate::DISC_SYSTEM_UNKNOWN | GameCubePrivate::DISC_FORMAT_CISO);
	}

	// Check for WIA.
	static const uint8_t wia_magic[4] = {'W','I','A',1};
	if (!memcmp(info->header.pData, wia_magic, sizeof(wia_magic))) {
		// This is a WIA image.
		// NOTE: We're using the WIA system ID if it's valid.
		// Otherwise, fall back to GCN/Wii magic.
		switch (info->header.pData[0x48]) {
			case 1:
				// GameCube disc image. (WIA format)
				return (GameCubePrivate::DISC_SYSTEM_GCN | GameCubePrivate::DISC_FORMAT_WIA);
			case 2:
				// Wii disc image. (WIA format)
				return (GameCubePrivate::DISC_SYSTEM_WII | GameCubePrivate::DISC_FORMAT_WIA);
			default:
				break;
		}

		// Check the GameCube/Wii magic.
		// TODO: WIA struct when full WIA support is added.
		uint32_t magic_tmp;
		memcpy(&magic_tmp, &info->header.pData[0x70], sizeof(magic_tmp));
		if (magic_tmp == cpu_to_be32(WII_MAGIC)) {
			// Wii disc image. (WIA format)
			return (GameCubePrivate::DISC_SYSTEM_WII | GameCubePrivate::DISC_FORMAT_WIA);
		}
		memcpy(&magic_tmp, &info->header.pData[0x74], sizeof(magic_tmp));
		if (magic_tmp == cpu_to_be32(GCN_MAGIC)) {
			// GameCube disc image. (WIA format)
			return (GameCubePrivate::DISC_SYSTEM_GCN | GameCubePrivate::DISC_FORMAT_WIA);
		}

		// Unrecognized WIA image...
		return (GameCubePrivate::DISC_SYSTEM_UNKNOWN | GameCubePrivate::DISC_FORMAT_WIA);
	}

	// Not supported.
	return GameCubePrivate::DISC_UNKNOWN;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *GameCube::systemName(unsigned int type) const
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

	static const char *const sysNames[16] = {
		// FIXME: "NGC" in Japan?
		"Nintendo GameCube", "GameCube", "GCN", nullptr,
		"Nintendo/Sega/Namco Triforce", "Triforce", "TF", nullptr,
		"Nintendo Wii", "Wii", "Wii", nullptr,
		nullptr, nullptr, nullptr, nullptr
	};

	const unsigned int idx = (type & SYSNAME_TYPE_MASK) | ((d->discType & 3) << 2);
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
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *GameCube::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".gcm", ".rvm", ".wbfs",
		".ciso", ".cso", ".tgc",

		// Partially supported. (Header only!)
		".wia",

		// NOTE: May cause conflicts on Windows
		// if fallback handling isn't working.
		".iso",

		nullptr
	};
	return exts;
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
 * Get image processing flags.
 *
 * These specify post-processing operations for images,
 * e.g. applying transparency masks.
 *
 * @param imageType Image type.
 * @return Bitfield of ImageProcessingBF operations to perform.
 */
uint32_t GameCube::imgpf(ImageType imageType) const
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_EXT_MAX);
	if (imageType < IMG_INT_MIN || imageType > IMG_EXT_MAX) {
		// ImageType is out of range.
		return 0;
	}

	switch (imageType) {
		case IMG_INT_BANNER:
			// Use nearest-neighbor scaling.
			return IMGPF_RESCALE_NEAREST;
		default:
			// GameTDB's GameCube and Wii disc and 3D cover scans
			// have alpha transparency. Hence, no image processing
			// is required.
			break;
	}
	return 0;
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
	const GCN_DiscHeader *const discHeader = &d->discHeader;
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
			d->fields->addField_string(C_("GameCube", "Title"),
				cp1252_to_utf8(
					discHeader->game_title, sizeof(discHeader->game_title)));
			break;

		case GCN_REGION_JAPAN:
		case GCN_REGION_SOUTH_KOREA:
			// Japan uses Shift-JIS.
			d->fields->addField_string(C_("GameCube", "Title"),
				cp1252_sjis_to_utf8(
					discHeader->game_title, sizeof(discHeader->game_title)));
			break;
	}

	// Game ID.
	// The ID6 cannot have non-printable characters.
	// (NDDEMO has ID6 "00\0E01".)
	for (int i = ARRAY_SIZE(discHeader->id6)-1; i >= 0; i--) {
		if (!isprint(discHeader->id6[i])) {
			// Non-printable character found.
			return -ENOENT;
		}
	}
	d->fields->addField_string(C_("GameCube", "Game ID"),
		latin1_to_utf8(discHeader->id6, ARRAY_SIZE(discHeader->id6)));

	// Look up the publisher.
	const char *const publisher = NintendoPublishers::lookup(discHeader->company);
	string s_publisher;
	if (publisher) {
		s_publisher = publisher;
	} else {
		if (isalnum(discHeader->company[0]) &&
		    isalnum(discHeader->company[1]))
		{
			s_publisher = rp_sprintf(C_("GameCube", "Unknown (%.2s)"),
				discHeader->company);
		} else {
			s_publisher = rp_sprintf(C_("GameCube", "Unknown (%02X %02X)"),
				(uint8_t)discHeader->company[0],
				(uint8_t)discHeader->company[1]);
		}
	}
	d->fields->addField_string(C_("GameCube", "Publisher"), s_publisher);

	// Other fields.
	d->fields->addField_string_numeric(C_("GameCube", "Disc #"),
		discHeader->disc_number+1, RomFields::FB_DEC);
	d->fields->addField_string_numeric(C_("GameCube", "Revision"),
		discHeader->revision, RomFields::FB_DEC, 2);

	// The remaining fields are not located in the disc header.
	// If we can't read the disc contents for some reason, e.g.
	// unimplemented DiscReader (WIA), skip the fields.
	if (!d->discReader) {
		// Cannot read the disc contents.
		// We're done for now.
		return (int)d->fields->count();
	}

	// Region code.
	// bi2.bin and/or RVL_RegionSetting is loaded in the constructor,
	// and the region code is stored in d->gcnRegion.
	const char *region = d->gcnRegionToString(d->gcnRegion, discHeader->id4[3]);
	if (region) {
		d->fields->addField_string(C_("GameCube", "Region"), region);
	} else {
		// Invalid region code.
		d->fields->addField_string(C_("GameCube", "Region"),
			rp_sprintf(C_("GameCube", "Unknown (0x%08X)"), d->gcnRegion));
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
				comment_data.append(comment->gamename_full, field_len);
				comment_data += '\n';
			} else if (comment->gamename[0] != 0) {
				field_len = (int)strnlen(comment->gamename, sizeof(comment->gamename));
				comment_data.append(comment->gamename, field_len);
				comment_data += '\n';
			}

			// Company.
			if (comment->company_full[0] != 0) {
				field_len = (int)strnlen(comment->company_full, sizeof(comment->company_full));
				comment_data.append(comment->company_full, field_len);
				comment_data += '\n';
			} else if (comment->company[0] != 0) {
				field_len = (int)strnlen(comment->company, sizeof(comment->company));
				comment_data.append(comment->company, field_len);
				comment_data += '\n';
			}

			// Game description.
			if (comment->gamedesc[0] != 0) {
				// Add a second newline if necessary.
				if (!comment_data.empty()) {
					comment_data += '\n';
				}

				field_len = (int)strnlen(comment->gamedesc, sizeof(comment->gamedesc));
				comment_data.append(comment->gamedesc, field_len);
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
						d->fields->addField_string(C_("GameCube", "Game Info"),
							cp1252_to_utf8(comment_data.data(), (int)comment_data.size()));
						break;

					case GCN_REGION_JAPAN:
					case GCN_REGION_SOUTH_KOREA:
						// Japan uses Shift-JIS.
						d->fields->addField_string(C_("GameCube", "Game Info"),
							cp1252_sjis_to_utf8(comment_data.data(), (int)comment_data.size()));
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
	d->fields->addField_ageRatings("Age Rating", age_ratings);

	// Display the Wii partition table(s).
	if (wiiPtLoaded == 0) {
		// Get the game name from opening.bnr.
		string game_name = d->wii_getBannerName();
		if (!game_name.empty()) {
			d->fields->addField_string(C_("GameCube", "Game Info"), game_name);
		} else {
			// Empty game name may be either because it's
			// homebrew, a prototype, or a key error.
			if (d->gamePartition->verifyResult() != KeyManager::VERIFY_OK) {
				// Key error.
				string err("ERROR: ");
				err += d->wii_getCryptoStatus(d->gamePartition);
				d->fields->addField_string(C_("GameCube", "Game Info"), err);
			}
		}

		// Update version.
		const char *sysMenu = nullptr;
		unsigned int ios_slot = 0, ios_major = 0, ios_minor = 0;
		bool isDebugIOS = false;
		if (d->updatePartition) {
			// Find the System Menu version.
			// - Retail: RVL-WiiSystemmenu-v*.wad file.
			// - Debug: firmware.64.56.21.29.wad
			//   - 64: Memory configuration (64 or 128)
			//   - 56: IOS slot
			//   - 21.29: IOS version. (21.29 == v5405)
			IFst::Dir *dirp = d->updatePartition->opendir("/_sys/");
			if (dirp) {
				IFst::DirEnt *dirent;
				while ((dirent = d->updatePartition->readdir(dirp)) != nullptr) {
					if (!dirent->name || dirent->type != DT_REG)
						continue;

					// Check for a retail System Menu.
					unsigned int version;
					int ret = sscanf(dirent->name, "RVL-WiiSystemmenu-v%u.wad", &version);
					if (ret == 1) {
						// Found a retail System Menu.
						sysMenu = WiiSystemMenuVersion::lookup(version);
						break;
					}

					// Check for a debug IOS.
					unsigned int ios_mem;
					ret = sscanf(dirent->name, "firmware.%u.%u.%u.%u.wad",
						&ios_mem, &ios_slot, &ios_major, &ios_minor);
					if (ret == 4 && (ios_mem == 64 || ios_mem == 128)) {
						// Found a debug IOS.
						isDebugIOS = true;
						break;
					}
				}
				d->updatePartition->closedir(dirp);
			}
		}

		if (isDebugIOS) {
			d->fields->addField_string(C_("GameCube", "Update"),
				rp_sprintf("IOS%u %u.%u (v%u)", ios_slot, ios_major, ios_minor,
					(ios_major << 8) | ios_minor));
		} else {
			if (!sysMenu) {
				if (!d->updatePartition) {
					sysMenu = C_("GameCube", "None");
				} else {
					sysMenu = d->wii_getCryptoStatus(d->updatePartition);
				}
			}
			d->fields->addField_string(C_("GameCube", "Update"), sysMenu);
		}

		// Partition table.
		auto partitions = new std::vector<std::vector<string> >();
		partitions->resize(d->wiiPtbl.size());

		unsigned int ptidx = 0;
		for (auto iter = partitions->begin(); iter != partitions->end(); ++iter, ptidx++) {
			vector<string> &data_row = *iter;
			data_row.reserve(5);	// 5 fields per row.

			// Partition entry.
			const GameCubePrivate::WiiPartEntry &entry = d->wiiPtbl[ptidx];

			// Partition number.
			data_row.push_back(rp_sprintf("%dp%d", entry.vg, entry.pt));

			// Partition type.
			string str;
			static const char *const part_type_tbl[3] = {
				// tr: GameCubePrivate::PARTITION_GAME
				NOP_C_("GameCube|Partition", "Game"),
				// tr: GameCubePrivate::PARTITION_UPDATE
				NOP_C_("GameCube|Partition", "Update"),
				// tr: GameCubePrivate::PARTITION_CHANNEL
				NOP_C_("GameCube|Partition", "Channel"),
			};
			if (entry.type <= GameCubePrivate::PARTITION_CHANNEL) {
				str = dpgettext_expr(RP_I18N_DOMAIN, "GameCube|Partition", part_type_tbl[entry.type]);
			} else {
				// If all four bytes are ASCII letters and/or numbers,
				// print it as-is. (SSBB demo channel)
				// Otherwise, print the hexadecimal value.
				// NOTE: Must be BE32 for proper display.
				union {
					uint32_t be32_type;
					char chr[4];
				} part_type;
				part_type.be32_type = cpu_to_be32(entry.type);
				if (isalnum(part_type.chr[0]) && isalnum(part_type.chr[1]) &&
				    isalnum(part_type.chr[2]) && isalnum(part_type.chr[3]))
				{
					// All four bytes are ASCII letters and/or numbers.
					str = latin1_to_utf8(part_type.chr, sizeof(part_type.chr));
				} else {
					// Non-ASCII data. Print the hex values instead.
					str = rp_sprintf("%08X", entry.type);
				}
				break;
			}
			data_row.push_back(str);

			// Encryption key.
			// TODO: Use a string table?
			const char *key_name;
			switch (entry.partition->encKey()) {
				case WiiPartition::ENCKEY_UNKNOWN:
				default:
					// tr: Unknown encryption key.
					key_name = C_("GameCube|KeyIdx", "Unknown");
					break;
				case WiiPartition::ENCKEY_COMMON:
					// tr: Retail encryption key.
					key_name = C_("GameCube|KeyIdx", "Retail");
					break;
				case WiiPartition::ENCKEY_KOREAN:
					// tr: Korean encryption key.
					key_name = C_("GameCube|KeyIdx", "Korean");
					break;
				case WiiPartition::ENCKEY_VWII:
					// tr: vWii-specific encryption key.
					key_name = C_("GameCube|KeyIdx", "vWii");
					break;
				case WiiPartition::ENCKEY_DEBUG:
					// tr: Debug encryption key.
					key_name = C_("GameCube|KeyIdx", "Debug");
					break;
				case WiiPartition::ENCKEY_NONE:
					// tr: No encryption.
					key_name = C_("GameCube|KeyIdx", "None");
					break;
			}
			data_row.push_back(key_name);

			// Used size.
			const int64_t used_size = entry.partition->partition_size_used();
			if (used_size >= 0) {
				data_row.push_back(LibRpBase::formatFileSize(used_size));
			} else {
				// tr: Unknown used size.
				data_row.push_back(C_("GameCube|Partition", "Unknown"));
			}

			// Partition size.
			data_row.push_back(LibRpBase::formatFileSize(entry.partition->partition_size()));
		}

		// Fields.
		static const char *const partitions_names[] = {
			// tr: Partition number.
			NOP_C_("GameCube|Partition", "#"),
			// tr: Partition type.
			NOP_C_("GameCube|Partition", "Type"),
			// tr: Encryption key.
			NOP_C_("GameCube|Partition", "Key"),
			// tr: Actual data used within the partition.
			NOP_C_("GameCube|Partition", "Used Size"),
			// tr: Total size of the partition.
			NOP_C_("GameCube|Partition", "Total Size"),
		};
		vector<string> *v_partitions_names = RomFields::strArrayToVector_i18n(
			"GameCube|Partition", partitions_names, ARRAY_SIZE(partitions_names));
		d->fields->addField_listData("Partitions", v_partitions_names, partitions);
	} else {
		// Could not load partition tables.
		// FIXME: Show an error?
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Pointer to const rp_image* to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCube::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	assert(imageType >= IMG_INT_MIN && imageType <= IMG_INT_MAX);
	assert(pImage != nullptr);
	if (!pImage) {
		// Invalid parameters.
		return -EINVAL;
	} else if (imageType < IMG_INT_MIN || imageType > IMG_INT_MAX) {
		// ImageType is out of range.
		*pImage = nullptr;
		return -ERANGE;
	}

	RP_D(GameCube);
	if (imageType != IMG_INT_BANNER) {
		// Only IMG_INT_BANNER is supported by GameCube.
		*pImage = nullptr;
		return -ENOENT;
	} else if (d->img_banner) {
		// Image has already been loaded.
		*pImage = d->img_banner;
		return 0;
	} else if (!d->file) {
		// File isn't open.
		*pImage = nullptr;
		return -EBADF;
	} else if (!d->isValid) {
		// Save file isn't valid.
		*pImage = nullptr;
		return -EIO;
	} else if (!d->discReader) {
		// Cannot read the disc contents.
		*pImage = nullptr;
		return -ENOENT;
	}

	// Internal images are currently only supported for GCN,
	// and possibly Triforce.
	if ((d->discType & GameCubePrivate::DISC_SYSTEM_MASK) >= GameCubePrivate::DISC_SYSTEM_WII) {
		// opening.bnr doesn't have an image.
		*pImage = nullptr;
		return -ENOENT;
	}

	// Load opening.bnr. (GCN/Triforce only)
	// FIXME: Does Triforce have opening.bnr?
	if (d->gcn_loadOpeningBnr() != 0) {
		// Could not load opening.bnr.
		*pImage = nullptr;
		return -ENOENT;
	}

	// Convert the banner from GameCube RGB5A3 to ARGB32.
	rp_image *banner = ImageDecoder::fromGcn16(ImageDecoder::PXF_RGB5A3,
		BANNER_IMAGE_W, BANNER_IMAGE_H,
		d->gcn_opening_bnr->banner, sizeof(d->gcn_opening_bnr->banner));
	if (!banner) {
		// Error converting the banner.
		*pImage = nullptr;
		return -EIO;
	}

	// Finished decoding the banner.
	d->img_banner = banner;
	*pImage = banner;
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
