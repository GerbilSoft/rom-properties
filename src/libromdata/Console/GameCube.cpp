/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCube.cpp: Nintendo GameCube and Wii disc image reader.              *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "GameCube.hpp"
#include "librpbase/RomData_p.hpp"

#include "gcn_structs.h"
#include "gcn_banner.h"
#include "wii_structs.h"
#include "wii_banner.h"
#include "data/NintendoPublishers.hpp"
#include "data/WiiSystemMenuVersion.hpp"
#include "data/NintendoLanguage.hpp"
#include "GameCubeRegions.hpp"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
using namespace LibRpBase;

// libi18n
#include "libi18n/i18n.h"

// librptexture
using LibRpTexture::rp_image;

// DiscReader
#include "librpbase/disc/DiscReader.hpp"
#include "disc/WbfsReader.hpp"
#include "disc/CisoGcnReader.hpp"
#include "disc/NASOSReader.hpp"
#include "disc/nasos_gcn.h"	// for magic numbers
#include "disc/WiiPartition.hpp"

// For sections delegated to other RomData subclasses.
#include "GameCubeBNR.hpp"

// for strnlen() if it's not available in <string.h>
#include "librpbase/TextFuncs_libc.h"

// C includes. (C++ namespace)
#include "librpbase/ctypex.h"
#include <cassert>
#include <cerrno>
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
		// NDDEMO header.
		static const uint8_t nddemo_header[64];

		enum DiscType {
			DISC_UNKNOWN = -1,	// Unknown disc type.

			// Low byte: System ID.
			DISC_SYSTEM_GCN = 0,		// GameCube disc image
			DISC_SYSTEM_TRIFORCE = 1,	// Triforce disc/ROM image [TODO]
			DISC_SYSTEM_WII = 2,		// Wii disc image
			DISC_SYSTEM_UNKNOWN = 0xFF,
			DISC_SYSTEM_MASK = 0xFF,

			// High byte: Image format.
			DISC_FORMAT_RAW   = (0 << 8),		// Raw image (ISO, GCM)
			DISC_FORMAT_SDK   = (1 << 8),		// Raw image with SDK header
			DISC_FORMAT_TGC   = (2 << 8),		// TGC (embedded disc image) (GCN only?)
			DISC_FORMAT_WBFS  = (3 << 8),		// WBFS image (Wii only)
			DISC_FORMAT_CISO  = (4 << 8),		// CISO image
			DISC_FORMAT_WIA   = (5 << 8),		// WIA image (Header only!)
			DISC_FORMAT_NASOS = (6 << 8),		// NASOS image
			DISC_FORMAT_PARTITION = (7 << 8),	// Standalone Wii partition
			DISC_FORMAT_UNKNOWN = (0xFF << 8),
			DISC_FORMAT_MASK = (0xFF << 8),
		};

		// Disc type and reader.
		int discType;
		IDiscReader *discReader;

		// Disc header.
		GCN_DiscHeader discHeader;
		RVL_RegionSetting regionSetting;

		// opening.bnr
		union {
			struct {
				// opening.bnr file and object.
				GcnPartition *partition;
				GameCubeBNR *data;
			} gcn;
			struct {
				// Wii opening.bnr. (IMET section)
				Wii_IMET_t *imet;
			} wii;
		} opening_bnr;

		// Region code. (bi2.bin for GCN, RVL_RegionSetting for Wii.)
		uint32_t gcnRegion;
		bool hasRegionCode;

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
			int64_t start;		// Starting address, in bytes.
			int64_t size;		// Estimated partition size, in bytes.

			WiiPartition *partition;	// Partition object.
			uint32_t type;		// Partition type. (See WiiPartitionType.)
			uint8_t vg;		// Volume group number.
			uint8_t pt;		// Partition number.
		};

		vector<WiiPartEntry> wiiPtbl;
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

	public:
		/**
		 * Get the disc publisher.
		 * @return Disc publisher.
		 */
		string getPublisher(void) const;

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
		 * [GameCube] Get the game information from opening.bnr.
		 * For BNR2, this uses the comment that most
		 * closely matches the host system language.
		 * @return Game information, or nullptr if opening.bnr was not loaded.
		 */
		string gcn_getGameInfo(void) const;

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
	, discType(DISC_UNKNOWN)
	, discReader(nullptr)
	, gcnRegion(~0)
	, hasRegionCode(false)
	, wiiPtblLoaded(false)
	, updatePartition(nullptr)
	, gamePartition(nullptr)
{
	// Clear the various structs.
	memset(&discHeader, 0, sizeof(discHeader));
	memset(&regionSetting, 0, sizeof(regionSetting));
	memset(&opening_bnr, 0, sizeof(opening_bnr));
}

GameCubePrivate::~GameCubePrivate()
{
	// Wii partition pointers.
	updatePartition = nullptr;
	gamePartition = nullptr;

	// Clear the existing partition table vector.
	for (auto iter = wiiPtbl.cbegin(); iter != wiiPtbl.cend(); ++iter) {
		delete iter->partition;
	}
	wiiPtbl.clear();

	if (discType > DISC_UNKNOWN) {
		// Delete opening.bnr data.
		switch (discType & DISC_SYSTEM_MASK) {
			case DISC_SYSTEM_GCN:
				if (opening_bnr.gcn.data) {
					opening_bnr.gcn.data->unref();
				}
				delete opening_bnr.gcn.partition;
				break;
			case DISC_SYSTEM_WII:
				delete opening_bnr.wii.imet;
				break;
			default:
				break;
		}
	}

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
	// - https://wiibrew.org/wiki/Wii_Disc#Partitions_information
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

	// Check the crypto and hash method.
	// TODO: Lookup table instead of branches?
	unsigned int cryptoMethod = 0;
	if (discHeader.disc_noCrypto != 0 || (discType & DISC_FORMAT_MASK) == DISC_FORMAT_NASOS) {
		// No encryption.
		cryptoMethod |= WiiPartition::CM_UNENCRYPTED;
	}
	if (discHeader.hash_verify != 0) {
		// No hashes.
		cryptoMethod |= WiiPartition::CM_32K;
	}

	// Process each volume group.
	for (unsigned int i = 0; i < 4; i++) {
		unsigned int count = be32_to_cpu(vgtbl.vg[i].count);
		if (count == 0) {
			continue;
		} else if (count > ARRAY_SIZE(pt)) {
			count = ARRAY_SIZE(pt);
		}

		// Read the partition table entries.
		int64_t pt_addr = static_cast<int64_t>(be32_to_cpu(vgtbl.vg[i].addr)) << 2;
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

			entry.vg = static_cast<uint8_t>(i);
			entry.pt = static_cast<uint8_t>(j);
			entry.start = static_cast<int64_t>(be32_to_cpu(pt[j].addr)) << 2;
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
		// TODO: NASOS images are decrypted, but we should
		// still show how they'd be encrypted.
		iter->partition = new WiiPartition(discReader, iter->start, iter->size,
			(WiiPartition::CryptoMethod)cryptoMethod);

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
 * Get the disc publisher.
 * @return Disc publisher.
 */
string GameCubePrivate::getPublisher(void) const
{
	const char *const publisher = NintendoPublishers::lookup(discHeader.company);
	if (publisher) {
		return publisher;
	}

	// Unknown publisher.
	if (ISALNUM(discHeader.company[0]) &&
	    ISALNUM(discHeader.company[1]))
	{
		// Disc ID is alphanumeric.
		return rp_sprintf(C_("RomData", "Unknown (%.2s)"),
			discHeader.company);
	}

	// Disc ID is not alphanumeric.
	return rp_sprintf(C_("GameCube", "Unknown (%02X %02X)"),
		static_cast<uint8_t>(discHeader.company[0]),
		static_cast<uint8_t>(discHeader.company[1]));
}

/**
 * Load opening.bnr. (GameCube version)
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCubePrivate::gcn_loadOpeningBnr(void)
{
	assert(discReader != nullptr);
	assert((discType & DISC_SYSTEM_MASK) == DISC_SYSTEM_GCN);
	if (!discReader) {
		return -EIO;
	} else if ((discType & DISC_SYSTEM_MASK) != DISC_SYSTEM_GCN) {
		// Not supported.
		// TODO: Do Triforce games have opening.bnr?
		return -ENOTSUP;
	}

	if (opening_bnr.gcn.data) {
		// Banner is already loaded.
		return 0;
	}

	// NOTE: The GCN partition needs to stay open,
	// since we have a subclass for reading the object.
	// since we don't need to access more than one file.
	unique_ptr<GcnPartition> gcnPartition(new GcnPartition(discReader, 0));
	if (!gcnPartition->isOpen()) {
		// Could not open the partition.
		return -EIO;
	}

	IRpFile *const f_opening_bnr = gcnPartition->open("/opening.bnr");
	if (!f_opening_bnr) {
		// Error opening "opening.bnr".
		return -gcnPartition->lastError();
	}

	// Attempt to open a GameCubeBNR subclass.
	GameCubeBNR *const bnr = new GameCubeBNR(f_opening_bnr);
	f_opening_bnr->unref();
	if (!bnr->isOpen()) {
		// Unable to open the subcalss.
		bnr->unref();
		return -EIO;
	}

	// GameCubeBNR subclass is open.
	opening_bnr.gcn.partition = gcnPartition.release();
	opening_bnr.gcn.data = bnr;
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

	if (opening_bnr.wii.imet) {
		// Banner is already loaded.
		return 0;
	}

	if (!gamePartition) {
		// No game partition...
		return -ENOENT;
	}

	IRpFile *const f_opening_bnr = gamePartition->open("/opening.bnr");
	if (!f_opening_bnr) {
		// Error opening "opening.bnr".
		return -gamePartition->lastError();
	}

	// Read the IMET struct.
	unique_ptr<Wii_IMET_t> pBanner(new Wii_IMET_t);
	size_t size = f_opening_bnr->read(pBanner.get(), sizeof(*pBanner));
	if (size != sizeof(*pBanner)) {
		// Read error.
		const int err = f_opening_bnr->lastError();
		f_opening_bnr->unref();
		return (err != 0 ? -err : -EIO);
	}
	f_opening_bnr->unref();

	// Verify the IMET magic.
	if (pBanner->magic != cpu_to_be32(WII_IMET_MAGIC)) {
		// Magic is incorrect.
		// TODO: Better error code?
		return -EIO;
	}

	// Banner is loaded.
	opening_bnr.wii.imet = pBanner.release();
	return 0;
}

/**
 * [GameCube] Get the game information from opening.bnr.
 * For BNR2, this uses the comment that most
 * closely matches the host system language.
 * @return Game information, or nullptr if opening.bnr was not loaded.
 */
string GameCubePrivate::gcn_getGameInfo(void) const
{
	assert((discType & DISC_SYSTEM_MASK) == DISC_SYSTEM_GCN);
	if ((discType & DISC_SYSTEM_MASK) != DISC_SYSTEM_GCN) {
		// Not supported.
		// TODO: Do Triforce games have opening.bnr?
		return string();
	}

	if (!opening_bnr.gcn.data) {
		// Attempt to load opening.bnr.
		if (const_cast<GameCubePrivate*>(this)->gcn_loadOpeningBnr() != 0) {
			// Error loading opening.bnr.
			return string();
		}

		// Make sure it was actually loaded.
		if (!opening_bnr.gcn.data) {
			// opening.bnr was not loaded.
			return string();
		}
	}

	// Get the comment from the GameCubeBNR.
	const gcn_banner_comment_t *comment = opening_bnr.gcn.data->getComment();
	if (!comment) {
		// Unable to get the comment...
		return string();
	}

	string gameInfo;
	gameInfo.reserve(sizeof(*comment));

	// Fields are not necessarily null-terminated.
	// NOTE: We're converting from cp1252 or Shift-JIS
	// *after* concatenating all the strings, which is
	// why we're using strnlen() here.
	size_t field_len;

	// Game name.
	if (comment->gamename_full[0] != '\0') {
		field_len = strnlen(comment->gamename_full, sizeof(comment->gamename_full));
		gameInfo.append(comment->gamename_full, field_len);
		gameInfo += '\n';
	} else if (comment->gamename[0] != '\0') {
		field_len = strnlen(comment->gamename, sizeof(comment->gamename));
		gameInfo.append(comment->gamename, field_len);
		gameInfo += '\n';
	}

	// Company.
	if (comment->company_full[0] != '\0') {
		field_len = strnlen(comment->company_full, sizeof(comment->company_full));
		gameInfo.append(comment->company_full, field_len);
		gameInfo += '\n';
	} else if (comment->company[0] != '\0') {
		field_len = strnlen(comment->company, sizeof(comment->company));
		gameInfo.append(comment->company, field_len);
		gameInfo += '\n';
	}

	// Game description.
	if (comment->gamedesc[0] != '\0') {
		// Add a second newline if necessary.
		if (!gameInfo.empty()) {
			gameInfo += '\n';
		}

		field_len = strnlen(comment->gamedesc, sizeof(comment->gamedesc));
		gameInfo.append(comment->gamedesc, field_len);
	}

	// Remove trailing newlines.
	// TODO: Optimize this by using a `for` loop and counter. (maybe ptr)
	while (!gameInfo.empty() && gameInfo[gameInfo.size()-1] == '\n') {
		gameInfo.resize(gameInfo.size()-1);
	}

	if (!gameInfo.empty()) {
		// Convert from cp1252 or Shift-JIS.
		switch (gcnRegion) {
			case GCN_REGION_USA:
			case GCN_REGION_EUR:
			case GCN_REGION_ALL:	// TODO: Assume JP?
			default:
				// USA/PAL uses cp1252.
				gameInfo = cp1252_to_utf8(gameInfo);
				break;

			case GCN_REGION_JPN:
			case GCN_REGION_KOR:
			case GCN_REGION_CHN:
			case GCN_REGION_TWN:
				// Japan uses Shift-JIS.
				gameInfo = cp1252_sjis_to_utf8(gameInfo);
				break;
		}
	}

	// We're done here.
	return gameInfo;
}

/**
 * Get the game name from opening.bnr. (Wii version)
 * This uses the name that most closely matches
 * the host system language.
 * @return Game name, or empty string if opening.bnr was not loaded.
 */
string GameCubePrivate::wii_getBannerName(void) const
{
	assert((discType & DISC_SYSTEM_MASK) == DISC_SYSTEM_WII);
	if ((discType & DISC_SYSTEM_MASK) != DISC_SYSTEM_WII) {
		// Not supported.
		return string();
	}

	if (!opening_bnr.wii.imet) {
		// Attempt to load opening.bnr.
		if (const_cast<GameCubePrivate*>(this)->wii_loadOpeningBnr() != 0) {
			// Error loading opening.bnr.
			return string();
		}

		// Make sure it was actually loaded.
		if (!opening_bnr.wii.imet) {
			// opening.bnr was not loaded.
			return string();
		}
	}

	// Get the system language.
	// TODO: Verify against the region code somehow?
	int lang = NintendoLanguage::getWiiLanguage();

	// If the language-specific name is empty,
	// revert to English.
	if (opening_bnr.wii.imet->names[lang][0][0] == 0) {
		// Revert to English.
		lang = WII_LANG_ENGLISH;
	}

	// NOTE: The banner may have two lines.
	// Each line is a maximum of 21 characters.
	// Convert from UTF-16 BE and split into two lines at the same time.
	string info = utf16be_to_utf8(opening_bnr.wii.imet->names[lang][0], 21);
	if (opening_bnr.wii.imet->names[lang][1][0] != 0) {
		info += '\n';
		info += utf16be_to_utf8(opening_bnr.wii.imet->names[lang][1], 21);
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
 * will be ref()'d and must be kept open in order to load
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
		// Could not ref() the file handle.
		return;
	}

	// Read the disc header.
	uint8_t header[4096+256];
	d->file->rewind();
	size_t size = d->file->read(&header, sizeof(header));
	if (size != sizeof(header)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

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
			case GameCubePrivate::DISC_FORMAT_PARTITION:
				d->discReader = new DiscReader(d->file);
				break;
			case GameCubePrivate::DISC_FORMAT_SDK:
				// Skip the SDK header.
				d->discReader = new DiscReader(d->file, 32768, -1);
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
			case GameCubePrivate::DISC_FORMAT_NASOS:
				d->discReader = new NASOSReader(d->file);
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
		d->file->unref();
		d->file = nullptr;
		return;
	}

	if (!d->discReader) {
		// No WiaReader yet. If this is WIA,
		// retrieve the header from header[].
		if ((d->discType & GameCubePrivate::DISC_FORMAT_MASK) == GameCubePrivate::DISC_FORMAT_WIA) {
			// GCN/Wii header starts at 0x58.
			memcpy(&d->discHeader, &header[0x58], sizeof(d->discHeader));
		} else {
			// Non-WIA formats must have a valid DiscReader.
			d->discType = GameCubePrivate::DISC_UNKNOWN;
			d->isValid = false;
			d->file->unref();
			d->file = nullptr;
		}
		return;
 	}

	// Save the disc header for later.
	d->discReader->rewind();
	if ((d->discType & GameCubePrivate::DISC_FORMAT_MASK) != GameCubePrivate::DISC_FORMAT_PARTITION) {
		// Regular disc image.
		size = d->discReader->read(&d->discHeader, sizeof(d->discHeader));
		if (size != sizeof(d->discHeader)) {
			// Error reading the disc header.
			delete d->discReader;
			d->file->unref();
			d->discReader = nullptr;
			d->file = nullptr;
			d->discType = GameCubePrivate::DISC_UNKNOWN;
			d->isValid = false;
			return;
		}
		d->hasRegionCode = true;
	} else {
		// Standalone partition.
		d->fileType = FTYPE_PARTITION;

		// Determine the partition type.
		// If title ID low is '\0UPD' or '\0UPE', assume it's an update partition.
		// Otherwise, it's probably a game partition.
		// TODO: Identify channel partitions by the title ID high?
		RVL_TitleID_t title_id;
		size = d->file->seekAndRead(offsetof(RVL_Ticket, title_id), &title_id, sizeof(title_id));
		if (size != sizeof(title_id)) {
			// Error reading the title ID.
			delete d->discReader;
			d->file->unref();
			d->discReader = nullptr;
			d->file = nullptr;
			d->discType = GameCubePrivate::DISC_UNKNOWN;
			d->isValid = false;
			return;
		}

		d->wiiPtbl.resize(1);
		GameCubePrivate::WiiPartEntry &pt = d->wiiPtbl[0];
		pt.start = 0;
		pt.size = d->file->size();
		pt.vg = 0;
		pt.pt = 0;
		pt.partition = new WiiPartition(d->discReader, pt.start, pt.size);

		if (title_id.lo == be32_to_cpu('\0UPD') || title_id.lo == be32_to_cpu('\0UPE')) {
			// Update partition.
			pt.type = RVL_PT_UPDATE;
			d->updatePartition = pt.partition;
		} else if (title_id.lo == be32_to_cpu('\0INS')) {
			// Channel partition.
			pt.type = RVL_PT_CHANNEL;
		} else {
			// Game partition.
			pt.type = RVL_PT_GAME;
			d->gamePartition = pt.partition;
		}

		// Read the partition header.
		size = pt.partition->read(&d->discHeader, sizeof(d->discHeader));
		if (size != sizeof(d->discHeader)) {
			// Error reading the partition header.
			delete pt.partition;
			d->wiiPtbl.clear();

			delete d->discReader;
			d->file->unref();
			d->discReader = nullptr;
			d->file = nullptr;
			d->discType = GameCubePrivate::DISC_UNKNOWN;
			d->isValid = false;
			return;
		}

		// Need to change encryption bytes to 00.
		d->discHeader.hash_verify = 0;
		d->discHeader.disc_noCrypto = 0;
		d->wiiPtblLoaded = true;

		// TODO: Figure out region code for standalone partitions.
		d->hasRegionCode = false;
	}

	if (((d->discType & GameCubePrivate::DISC_FORMAT_MASK) == GameCubePrivate::DISC_FORMAT_NASOS) &&
	    ((d->discType & GameCubePrivate::DISC_SYSTEM_MASK) != GameCubePrivate::DISC_SYSTEM_UNKNOWN))
	{
		// Verify that the NASOS header matches the disc format.
		bool isOK = true;
		switch (d->discType & GameCubePrivate::DISC_SYSTEM_MASK) {
			case GameCubePrivate::DISC_SYSTEM_GCN:
				// Must have GCN magic number or nddemo header.
				if (d->discHeader.magic_gcn == cpu_to_be32(GCN_MAGIC)) {
					// GCN magic number is present.
					break;
				} else if (!memcmp(&d->discHeader, GameCubePrivate::nddemo_header,
				           sizeof(GameCubePrivate::nddemo_header)))
				{
					// nddemo header is present.
					break;
				}

				// Not a GCN image.
				isOK = false;
				break;

			case GameCubePrivate::DISC_SYSTEM_WII:
				// Must have Wii magic number.
				if (d->discHeader.magic_wii != cpu_to_be32(WII_MAGIC)) {
					// Wii magic number is NOT present.
					isOK = false;
				}
				break;

			default:
				// Unsupported...
				isOK = false;
				break;
		}

		if (!isOK) {
			// Incorrect image format.
			delete d->discReader;
			d->file->unref();
			d->discReader = nullptr;
			d->file = nullptr;
			d->discType = GameCubePrivate::DISC_UNKNOWN;
			d->isValid = false;
			return;
		}
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
		// - SDK has a 32 KB SDK header before the disc image.
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
			d->file->unref();
			d->discReader = nullptr;
			d->file = nullptr;
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
				d->file->unref();
				d->discReader = nullptr;
				d->file = nullptr;
				d->discType = GameCubePrivate::DISC_UNKNOWN;
				d->isValid = false;
				return;
			}

			d->gcnRegion = be32_to_cpu(bootInfo.region_code);
			d->hasRegionCode = true;
			break;
		}

		case GameCubePrivate::DISC_SYSTEM_WII:
			// TODO: Figure out region code for standalone partitions.
			if (d->hasRegionCode) {
				size = d->discReader->seekAndRead(RVL_RegionSetting_ADDRESS, &d->regionSetting, sizeof(d->regionSetting));
				if (size != sizeof(d->regionSetting)) {
					// Cannot read RVL_RegionSetting.
					delete d->discReader;
					d->file->unref();
					d->discReader = nullptr;
					d->file = nullptr;
					d->discType = GameCubePrivate::DISC_UNKNOWN;
					d->isValid = false;
					return;
				}

				d->gcnRegion = be32_to_cpu(d->regionSetting.region_code);
			}
			break;

		default:
			// Unknown system.
			delete d->discReader;
			d->file->unref();
			d->discReader = nullptr;
			d->file = nullptr;
			d->discType = GameCubePrivate::DISC_UNKNOWN;
			d->isValid = false;
			return;
	}
}

/**
 * Close the opened file.
 */
void GameCube::close(void)
{
	RP_D(GameCube);
	if (d->discType > GameCubePrivate::DISC_UNKNOWN) {
		// Close opening.bnr subclasses.
		// NOTE: Don't delete them, since they
		// may be needed for images and fields.
		switch (d->discType & GameCubePrivate::DISC_SYSTEM_MASK) {
			case GameCubePrivate::DISC_SYSTEM_GCN:
				if (d->opening_bnr.gcn.data) {
					d->opening_bnr.gcn.data->close();
				}
				delete d->opening_bnr.gcn.partition;
				d->opening_bnr.gcn.partition = nullptr;
				break;
			case GameCubePrivate::DISC_SYSTEM_WII:
				// No subclass for Wii yet.
				break;
			default:
				break;
		}
	}

	// Call the superclass function.
	super::close();
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
	const GCN_DiscHeader *gcn_header =
		reinterpret_cast<const GCN_DiscHeader*>(info->header.pData);
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

	// Check for SDK headers.
	// TODO: More comprehensive?
	// TODO: Checksum at 0x0830. (For GCN, makeGCM always puts 0xAB0B here...)
	static const uint32_t sdk_0x0000 = 0xFFFF0000;	// BE32
	static const uint32_t sdk_0x082C = 0x0000E006;	// BE32
	const uint32_t *const pData32 =
		reinterpret_cast<const uint32_t*>(info->header.pData);
	if (pData32[0] == cpu_to_be32(sdk_0x0000)) {
		if (info->header.size < 0x0830) {
			// Can't check 0x082C, so assume it has the SDK headers.
			return (GameCubePrivate::DISC_SYSTEM_UNKNOWN | GameCubePrivate::DISC_FORMAT_SDK);
		}

		if (pData32[0x082C/4] == cpu_to_be32(sdk_0x082C)) {
			// This is a valid GCN/Wii SDK disc image header.
			return (GameCubePrivate::DISC_SYSTEM_UNKNOWN | GameCubePrivate::DISC_FORMAT_SDK);
		}
	}

	// Check for TGC.
	const GCN_TGC_Header *const tgcHeader =
		reinterpret_cast<const GCN_TGC_Header*>(info->header.pData);
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
			// Check for magic numbers.
			gcn_header = reinterpret_cast<const GCN_DiscHeader*>(&info->header.pData[hdd_sector_size]);
			if (gcn_header->magic_wii == cpu_to_be32(WII_MAGIC)) {
				// Wii disc image. (WBFS format)
				return (GameCubePrivate::DISC_SYSTEM_WII | GameCubePrivate::DISC_FORMAT_WBFS);
			} else if (gcn_header->magic_gcn == cpu_to_be32(GCN_MAGIC)) {
				// GameCube disc image. (WBFS format)
				// NOTE: Not really useful, but `wit` supports
				// converting GameCube disc images to WBFS format.
				return (GameCubePrivate::DISC_SYSTEM_GCN | GameCubePrivate::DISC_FORMAT_WBFS);
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
	static const uint32_t wia_magic = 'WIA\x01';
	if (pData32[0] == cpu_to_be32(wia_magic)) {
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
		// FIXME BEFORE COMMIT: TEST THIS
		gcn_header = reinterpret_cast<const GCN_DiscHeader*>(&info->header.pData[0x58]);
		if (gcn_header->magic_wii == cpu_to_be32(WII_MAGIC)) {
			// Wii disc image. (WIA format)
			return (GameCubePrivate::DISC_SYSTEM_WII | GameCubePrivate::DISC_FORMAT_WIA);
		} else if (gcn_header->magic_gcn == cpu_to_be32(GCN_MAGIC)) {
			// GameCube disc image. (WIA format)
			return (GameCubePrivate::DISC_SYSTEM_GCN | GameCubePrivate::DISC_FORMAT_WIA);
		}

		// Unrecognized WIA image...
		return (GameCubePrivate::DISC_SYSTEM_UNKNOWN | GameCubePrivate::DISC_FORMAT_WIA);
	}

	// Check for NASOS.
	// TODO: WII9?
	if (pData32[0] == cpu_to_be32(NASOS_MAGIC_GCML)) {
		// GameCube NASOS image.
		return (GameCubePrivate::DISC_SYSTEM_GCN | GameCubePrivate::DISC_FORMAT_NASOS);
	} else if (pData32[0] == cpu_to_be32(NASOS_MAGIC_WII5)) {
		// Wii NASOS image. (single-layer)
		return (GameCubePrivate::DISC_SYSTEM_WII | GameCubePrivate::DISC_FORMAT_NASOS);
	}

	// Check for a standalone Wii partition.
	if (pData32[0] == cpu_to_be32(0x00010001)) {
		// Signature type is correct.
		// TODO: Allow signature type only without the issuer?
		if (info->header.size >= 0x144) {
			if (pData32[0x140/4] == cpu_to_be32('Root')) {
				// Issuer field starts with "Root".
				return (GameCubePrivate::DISC_SYSTEM_WII | GameCubePrivate::DISC_FORMAT_PARTITION);
			}
		}
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

	// Bits 0-1: Type. (long, short, abbreviation)
	// Bits 2-3: DISC_SYSTEM_MASK (GCN, Wii, Triforce)

	static const char *const sysNames[4][4] = {
		// FIXME: "NGC" in Japan?
		{"Nintendo GameCube", "GameCube", "GCN", nullptr},
		{"Nintendo/Sega/Namco Triforce", "Triforce", "TF", nullptr},
		{"Nintendo Wii", "Wii", "Wii", nullptr},
		{nullptr, nullptr, nullptr, nullptr},
	};

	return sysNames[d->discType & 3][type & SYSNAME_TYPE_MASK];
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
		".dec",	// .iso.dec

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
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *GameCube::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types from FreeDesktop.org.
		"application/x-gamecube-rom",
		"application/x-gamecube-iso-image",
		"application/x-wii-rom",
		"application/x-wii-iso-image",
		"application/x-wbfs",
		"application/x-wia",

		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"application/x-nasos-image",

		nullptr
	};
	return mimeTypes;
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
 * Get a list of all available image sizes for the specified image type.
 *
 * The first item in the returned vector is the "default" size.
 * If the width/height is 0, then an image exists, but the size is unknown.
 *
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> GameCube::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

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
	return vector<ImageSizeDef>();
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
	ASSERT_imgpf(imageType);

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
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GameCube::loadFieldData(void)
{
	RP_D(GameCube);
	if (!d->fields->empty()) {
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
	const GCN_DiscHeader *const discHeader = &d->discHeader;

	// TODO: Reserve fewer fields for GCN?
	// Maximum number of fields:
	// - GameCube and Wii: 7 (includes Game Info)
	// - Wii only: 5
	d->fields->reserve(12);

	// TODO: Trim the titles. (nulls, spaces)
	// NOTE: The titles are dup()'d as C strings, so maybe not nulls.
	// TODO: Display the disc image format?

	// Game title.
	// TODO: Is Shift-JIS actually permissible here?
	const char *const title_title = C_("RomData", "Title");
	switch (d->gcnRegion) {
		case GCN_REGION_USA:
		case GCN_REGION_EUR:
		case GCN_REGION_ALL:	// TODO: Assume JP?
		default:
			// USA/PAL uses cp1252.
			d->fields->addField_string(title_title,
				cp1252_to_utf8(
					discHeader->game_title, sizeof(discHeader->game_title)));
			break;

		case GCN_REGION_JPN:
		case GCN_REGION_KOR:
		case GCN_REGION_CHN:
		case GCN_REGION_TWN:
			// Japan uses Shift-JIS.
			d->fields->addField_string(title_title,
				cp1252_sjis_to_utf8(
					discHeader->game_title, sizeof(discHeader->game_title)));
			break;
	}

	// Game ID.
	// The ID6 cannot have non-printable characters.
	// (NDDEMO has ID6 "00\0E01".)
	for (int i = ARRAY_SIZE(discHeader->id6)-1; i >= 0; i--) {
		if (!ISPRINT(discHeader->id6[i])) {
			// Non-printable character found.
			return -ENOENT;
		}
	}
	d->fields->addField_string(C_("GameCube", "Game ID"),
		latin1_to_utf8(discHeader->id6, ARRAY_SIZE(discHeader->id6)));

	// Publisher.
	d->fields->addField_string(C_("RomData", "Publisher"), d->getPublisher());

	// Other fields.
	d->fields->addField_string_numeric(C_("RomData", "Disc #"),
		discHeader->disc_number+1, RomFields::FB_DEC);
	d->fields->addField_string_numeric(C_("RomData", "Revision"),
		discHeader->revision, RomFields::FB_DEC, 2);

	// The remaining fields are not located in the disc header.
	// If we can't read the disc contents for some reason, e.g.
	// unimplemented DiscReader (WIA), skip the fields.
	if (!d->discReader) {
		// Cannot read the disc contents.
		// We're done for now.
		return static_cast<int>(d->fields->count());
	}

	const char *const game_info_title = C_("GameCube", "Game Info");

	// Region code.
	// bi2.bin and/or RVL_RegionSetting is loaded in the constructor,
	// and the region code is stored in d->gcnRegion.
	if (d->hasRegionCode) {
		bool isDefault;
		const char *const region =
			GameCubeRegions::gcnRegionToString(d->gcnRegion, discHeader->id4[3], &isDefault);
		const char *const region_code_title = C_("RomData", "Region Code");
		if (region) {
			// Append the GCN region name (USA/JPN/EUR/KOR) if
			// the ID4 value differs.
			const char *suffix = nullptr;
			if (!isDefault) {
				suffix = GameCubeRegions::gcnRegionToAbbrevString(d->gcnRegion);
			}

			string s_region;
			if (suffix) {
				// tr: %1%s == full region name, %2$s == abbreviation
				s_region = rp_sprintf_p(C_("GameCube", "%1$s (%2$s)"), region, suffix);
			} else {
				s_region = region;
			}

			d->fields->addField_string(region_code_title, s_region);
		} else {
			// Invalid region code.
			d->fields->addField_string(region_code_title,
				rp_sprintf(C_("RomData", "Unknown (0x%08X)"), d->gcnRegion));
		}

		if ((d->discType & GameCubePrivate::DISC_SYSTEM_MASK) != GameCubePrivate::DISC_SYSTEM_WII) {
			// GameCube-specific fields.

			// Game information from opening.bnr.
			string comment = d->gcn_getGameInfo();
			if (!comment.empty()) {
				// Show the comment.
				d->fields->addField_string(game_info_title, comment);
			}

			// Finished reading the field data.
			return static_cast<int>(d->fields->count());
		}
	}

	/** Wii-specific fields. **/

	// Load the Wii partition tables.
	int wiiPtLoaded = d->loadWiiPartitionTables();

	// TMD fields.
	if (d->gamePartition) {
		const RVL_TMD_Header *const tmdHeader = d->gamePartition->tmdHeader();
		if (tmdHeader) {
			// Title ID.
			// TID Lo is usually the same as the game ID,
			// except for some diagnostics discs.
			d->fields->addField_string(C_("GameCube", "Title ID"),
				rp_sprintf("%08X-%08X",
					be32_to_cpu(tmdHeader->title_id.hi),
					be32_to_cpu(tmdHeader->title_id.lo)));

			// Access rights.
			vector<string> *const v_access_rights_hdr = new vector<string>();
			v_access_rights_hdr->reserve(2);
			v_access_rights_hdr->push_back("AHBPROT");
			v_access_rights_hdr->push_back(C_("GameCube", "DVD Video"));
			d->fields->addField_bitfield(C_("GameCube", "Access Rights"),
				v_access_rights_hdr, 0, be32_to_cpu(tmdHeader->access_rights));
		}
	}

	// Get age rating(s).
	// RVL_RegionSetting is loaded in the constructor.
	// Note that not all 16 fields are present on GCN,
	// though the fields do match exactly, so no
	// mapping is necessary.
	if (d->hasRegionCode) {
		RomFields::age_ratings_t age_ratings;
		// Valid ratings: 0-1, 3-9
		static const uint16_t valid_ratings = 0x3FB;

		for (int i = static_cast<int>(age_ratings.size())-1; i >= 0; i--) {
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
		d->fields->addField_ageRatings(C_("RomData", "Age Ratings"), age_ratings);
	}

	// Display the Wii partition table(s).
	if (wiiPtLoaded == 0) {
		// Get the game name from opening.bnr.
		string game_name = d->wii_getBannerName();
		if (!game_name.empty()) {
			d->fields->addField_string(game_info_title, game_name);
		} else {
			// Empty game name may be either because it's
			// homebrew, a prototype, or a key error.
			if (!d->gamePartition) {
				// No game partition.
				if ((d->discType & GameCubePrivate::DISC_FORMAT_MASK) != GameCubePrivate::DISC_FORMAT_PARTITION) {
					d->fields->addField_string(game_info_title,
						C_("GameCube", "ERROR: No game partition was found."));
				}
			} else if (d->gamePartition->verifyResult() != KeyManager::VERIFY_OK) {
				// Key error.
				const char *status = d->wii_getCryptoStatus(d->gamePartition);
				d->fields->addField_string(game_info_title,
					rp_sprintf(C_("GameCube", "ERROR: %s"),
						(status ? status : C_("GameCube", "Unknown"))));
			}
		}

		// Update version.
		const char *sysMenu = nullptr;
		unsigned int ios_slot = 0, ios_major = 0, ios_minor = 0;
		unsigned int ios_retail_count = 0;
		bool isDebugIOS = false;
		if (d->updatePartition) {
			// Get the update version.
			//
			// On retail discs, the update partition usually contains
			// a System Menu, but some (RHMP99, Harvest Moon PAL) only
			// contain Boot2 and IOS.
			//
			// Debug discs generally only have two copies of IOS.
			// Both copies are the same version, with one compiled for
			// 64M systems and one for 128M systems.
			//
			// Filename patterns:
			// - Retail:
			//   - System menu: RVL-WiiSystemmenu-v*.wad file.
			//   - IOS: IOS21-64-v514.wad
			//     - 21: IOS slot
			//     - 64: Memory configuration (64 only)
			//     - 514: IOS version. (v514 == 2.2)
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
					if (dirent->name[0] == 'R') {
						unsigned int version;
						int ret = sscanf(dirent->name, "RVL-WiiSystemmenu-v%u.wad", &version);
						if (ret == 1) {
							// Found a retail System Menu.
							sysMenu = WiiSystemMenuVersion::lookup(version);
							break;
						}
					}

					// Check for a debug IOS.
					if (dirent->name[0] == 'f') {
						unsigned int ios_mem;
						int ret = sscanf(dirent->name, "firmware.%u.%u.%u.%u.wad",
							&ios_mem, &ios_slot, &ios_major, &ios_minor);
						if (ret == 4 && (ios_mem == 64 || ios_mem == 128)) {
							// Found a debug IOS.
							isDebugIOS = true;
							break;
						}
					}

					// Check for a retail IOS.
					if (dirent->name[0] == 'I') {
						unsigned int ios_mem;
						int ret = sscanf(dirent->name, "IOS%u-%u-v%u.wad",
							&ios_slot, &ios_mem, &ios_major);
						if (ret == 3 && ios_mem == 64) {
							// Found a retail IOS.
							// NOTE: ios_major has a combined version number,
							// so it needs to be split into major/minor.
							ios_minor = ios_major & 0xFF;
							ios_major >>= 8;
							ios_retail_count++;
						}
					}
				}
				d->updatePartition->closedir(dirp);
			}
		}

		const char *const update_title = C_("GameCube", "Update");
		if (isDebugIOS || ios_retail_count == 1) {
			d->fields->addField_string(update_title,
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
			d->fields->addField_string(update_title, sysMenu);
		}

		// Partition table.
		auto vv_partitions = new vector<vector<string> >();
		vv_partitions->resize(d->wiiPtbl.size());

		auto src_iter = d->wiiPtbl.cbegin();
		auto dest_iter = vv_partitions->begin();
		for ( ; dest_iter != vv_partitions->end(); ++src_iter, ++dest_iter) {
			vector<string> &data_row = *dest_iter;
			data_row.reserve(5);	// 5 fields per row.

			// Partition entry.
			const GameCubePrivate::WiiPartEntry &entry = *src_iter;

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
				if (ISALNUM(part_type.chr[0]) && ISALNUM(part_type.chr[1]) &&
				    ISALNUM(part_type.chr[2]) && ISALNUM(part_type.chr[3]))
				{
					// All four bytes are ASCII letters and/or numbers.
					str = latin1_to_utf8(part_type.chr, sizeof(part_type.chr));
				} else {
					// Non-ASCII data. Print the hex values instead.
					str = rp_sprintf("%08X", entry.type);
				}
			}
			data_row.push_back(str);

			// Encryption key.
			// TODO: Use a string table?
			WiiPartition::EncKey encKey;
			if ((d->discType & GameCubePrivate::DISC_FORMAT_MASK) == GameCubePrivate::DISC_FORMAT_NASOS) {
				// NASOS disc image.
				// If this would normally be an encrypted image, use encKeyReal().
				encKey = (d->discHeader.disc_noCrypto == 0
					? entry.partition->encKeyReal()
					: entry.partition->encKey());
			} else {
				// Other disc image. Use encKey().
				encKey = entry.partition->encKey();
			}

			static const char *const wii_key_tbl[] = {
				// tr: WiiPartition::ENCKEY_COMMON - Retail encryption key.
				NOP_C_("GameCube|KeyIdx", "Retail"),
				// tr: WiiPartition::ENCKEY_KOREAN - Korean encryption key.
				NOP_C_("GameCube|KeyIdx", "Korean"),
				// tr: WiiPartition::ENCKEY_VWII - vWii-specific encryption key.
				NOP_C_("GameCube|KeyIdx", "vWii"),
				// tr: WiiPartition::ENCKEY_DEBUG - Debug encryption key.
				NOP_C_("GameCube|KeyIdx", "Debug"),
				// tr: WiiPartition::ENCKEY_NONE - No encryption.
				NOP_C_("GameCube|KeyIdx", "None"),
			};
			static_assert(ARRAY_SIZE(wii_key_tbl) == WiiPartition::ENCKEY_MAX,
				"wii_key_tbl[] size is incorrect.");

			const char *s_key_name;
			if (encKey >= 0 && encKey < ARRAY_SIZE(wii_key_tbl)) {
				s_key_name = dpgettext_expr(RP_I18N_DOMAIN, "GameCube|KeyIdx", wii_key_tbl[encKey]);
			} else {
				// WiiPartition::ENCKEY_UNKNOWN
				s_key_name = C_("RomData", "Unknown");
			}
			data_row.push_back(s_key_name);

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
		vector<string> *const v_partitions_names = RomFields::strArrayToVector_i18n(
			"GameCube|Partition", partitions_names, ARRAY_SIZE(partitions_names));

		RomFields::AFLD_PARAMS params;
		params.headers = v_partitions_names;
		params.list_data = vv_partitions;
		d->fields->addField_listData(C_("GameCube", "Partitions"), &params);
	} else {
		// Could not load partition tables.
		// FIXME: Show an error?
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int GameCube::loadMetaData(void)
{
	RP_D(GameCube);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->discType < 0) {
		// Unknown disc type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(3);	// Maximum of 3 metadata properties.

	// Disc header is read in the constructor.
	const GCN_DiscHeader *const discHeader = &d->discHeader;

	// If this is GameCube, use opening.bnr if available.
	// TODO: Wii IMET?
	bool addedBnrMetaData = false;
	switch (d->discType & GameCubePrivate::DISC_SYSTEM_MASK) {
		default:
			break;

		case GameCubePrivate::DISC_SYSTEM_GCN: {
			if (!d->opening_bnr.gcn.data) {
				d->gcn_loadOpeningBnr();
				if (!d->opening_bnr.gcn.data) {
					// Still unable to load the metadata.
					break;
				}
			}

			// Get the metadata from opening.bnr.
			const RomMetaData *const bnrMetaData = d->opening_bnr.gcn.data->metaData();
			if (bnrMetaData && !bnrMetaData->empty()) {
				int ret = d->metaData->addMetaData_metaData(bnrMetaData);
				if (ret >= 0) {
					// Metadata added successfully.
					addedBnrMetaData = true;
				}
			}
			break;
		}
	}

	if (!addedBnrMetaData) {
		// Unable to load opening.bnr.
		// Use the disc header.

		// Title.
		// TODO: Use opening.bnr title for GameCube instead?
		// TODO: Is Shift-JIS actually permissible here?
		switch (d->gcnRegion) {
			case GCN_REGION_USA:
			case GCN_REGION_EUR:
			case GCN_REGION_ALL:	// TODO: Assume JP?
			default:
				// USA/PAL uses cp1252.
				d->metaData->addMetaData_string(Property::Title,
					cp1252_to_utf8(
						discHeader->game_title, sizeof(discHeader->game_title)));
				break;

			case GCN_REGION_JPN:
			case GCN_REGION_KOR:
			case GCN_REGION_CHN:
			case GCN_REGION_TWN:
				// Japan uses Shift-JIS.
				d->metaData->addMetaData_string(Property::Title,
					cp1252_sjis_to_utf8(
						discHeader->game_title, sizeof(discHeader->game_title)));
				break;
		}

		// Publisher.
		d->metaData->addMetaData_string(Property::Publisher, d->getPublisher());
	}

	// TODO: Disc number?

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
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
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(GameCube);
	if (imageType != IMG_INT_BANNER) {
		// Only IMG_INT_BANNER is supported by GameCube.
		*pImage = nullptr;
		return -ENOENT;
	} else if (!d->isValid) {
		// Disc image isn't valid.
		*pImage = nullptr;
		return -EIO;
	}

	// Internal images are currently only supported for GCN.
	if ((d->discType & GameCubePrivate::DISC_SYSTEM_MASK) != GameCubePrivate::DISC_SYSTEM_GCN) {
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

	// Forward this call to the GameCubeBNR object.
	if (d->opening_bnr.gcn.data) {
		return d->opening_bnr.gcn.data->loadInternalImage(imageType, pImage);
	}

	// No GameCubeBNR object.
	*pImage = nullptr;
	return -ENOENT;
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
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	RP_D(const GameCube);
	if (d->discType < 0) {
		// Unknown disc type.
		return -EIO;
	} else if ((d->discType & GameCubePrivate::DISC_FORMAT_MASK) == GameCubePrivate::DISC_FORMAT_TGC) {
		// TGC game IDs aren't unique, so we can't get
		// an image URL that makes any sense.
		return -ENOENT;
	}

	// Check for known unusable game IDs.
	// - RELSAB: Generic ID used for prototypes and Wii update partitions.
	// - _INSZZ: Channel partition.
	if (d->discHeader.id4[0] == '_' ||
	    !memcmp(d->discHeader.id6, "RELSAB", 6))
	{
		// Cannot download images for this game ID.
		return -ENOENT;
	}

	// Get the image sizes and sort them based on the
	// requested image size.
	vector<ImageSizeDef> sizeDefs = supportedImageSizes(imageType);
	if (sizeDefs.empty()) {
		// No image sizes.
		return -ENOENT;
	}

	// Select the best size.
	const ImageSizeDef *const sizeDef = d->selectBestSize(sizeDefs, size);
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
	vector<const char*> tdb_regions =
		GameCubeRegions::gcnRegionToGameTDB(d->gcnRegion, d->discHeader.id4[3]);

	// Game ID.
	// Replace any non-printable characters with underscores.
	// (NDDEMO has ID6 "00\0E01".)
	char id6[7];
	for (int i = 0; i < 6; i++) {
		id6[i] = (ISPRINT(d->discHeader.id6[i])
			? d->discHeader.id6[i]
			: '_');
	}
	id6[6] = 0;

	// External images with multiple discs must be handled differently.
	const bool isDisc2 =
		(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX) &&
		 d->discHeader.disc_number > 0;

	// ExtURLs.
	// TODO: If multiple image sizes are added, add the
	// "default" size to the end of ExtURLs in case the
	// user has high-resolution downloads disabled.
	// See Nintendo3DS for an example.
	// (NOTE: For GameTDB, currently only applies to coverfullHQ on GCN/Wii.)
	size_t vsz = tdb_regions.size();
	if (isDisc2) {
		// Need to increase the initial size.
		// Increasing the size later invalidates the iterator.
		vsz *= 2;
	}

	pExtURLs->resize(vsz);
	auto extURL_iter = pExtURLs->begin();

	// Is this not the first disc?
	if (isDisc2) {
		// Disc 2 (or 3, or 4...)
		// Request the disc 2 image first.
		char discName[16];
		snprintf(discName, sizeof(discName), "%s%u",
			 imageTypeName, d->discHeader.disc_number+1);

		for (auto tdb_iter = tdb_regions.cbegin();
		     tdb_iter != tdb_regions.cend(); ++tdb_iter, ++extURL_iter)
		{
			extURL_iter->url = d->getURL_GameTDB("wii", discName, *tdb_iter, id6, ".png");
			extURL_iter->cache_key = d->getCacheKey_GameTDB("wii", discName, *tdb_iter, id6, ".png");
			extURL_iter->width = sizeDef->width;
			extURL_iter->height = sizeDef->height;
		}
	}

	// First disc, or not a disc scan.
	for (auto tdb_iter = tdb_regions.cbegin();
	     tdb_iter != tdb_regions.cend(); ++tdb_iter, ++extURL_iter)
	{
		extURL_iter->url = d->getURL_GameTDB("wii", imageTypeName, *tdb_iter, id6, ".png");
		extURL_iter->cache_key = d->getCacheKey_GameTDB("wii", imageTypeName, *tdb_iter, id6, ".png");
		extURL_iter->width = sizeDef->width;
		extURL_iter->height = sizeDef->height;
		extURL_iter->high_res = false;	// Only one size is available.
	}

	// All URLs added.
	return 0;
}

}
