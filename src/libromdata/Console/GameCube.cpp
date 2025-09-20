/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCube.cpp: Nintendo GameCube and Wii disc image reader.              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GameCube.hpp"

#include "gcn_structs.h"
#include "wii_structs.h"
#include "wii_banner.h"
#include "data/NintendoPublishers.hpp"
#include "data/WiiSystemMenuVersion.hpp"
#include "data/NintendoLanguage.hpp"
#include "GameCubeRegions.hpp"
#include "WiiCommon.hpp"

// Other rom-properties libraries
#include "librpfile/RelatedFile.hpp"
#include "librpbase/Achievements.hpp"
#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// WiiPartition reader
#include "disc/WiiPartition.hpp"
// NASOSReader to check if it's a NASOS format disc image
#include "disc/NASOSReader.hpp"

// For sections delegated to other RomData subclasses.
#include "GameCubeBNR.hpp"
#include "WiiBNR.hpp"

// WiiTicket for EncryptionKeys
#include "WiiTicket.hpp"

// for strnlen() if it's not available in <string.h>
#include "librptext/libc.h"

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

class GameCubePrivate final : public RomDataPrivate
{
public:
	explicit GameCubePrivate(const IRpFilePtr &file);
	~GameCubePrivate() final;

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(GameCubePrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 13+1> exts;
	static const array<const char*, 14+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// NDDEMO header
	static const array<uint8_t, 64> nddemo_header;

	enum DiscType {
		DISC_UNKNOWN = -1,	// Unknown disc type

		// Low byte: System ID
		DISC_SYSTEM_GCN = 0,		// GameCube disc image
		DISC_SYSTEM_TRIFORCE = 1,	// Triforce disc/ROM image [TODO]
		DISC_SYSTEM_WII = 2,		// Wii disc image
		DISC_SYSTEM_UNKNOWN = 0xFF,
		DISC_SYSTEM_MASK = 0xFF,

		// High byte: Image format
		DISC_FORMAT_RAW   = (0U << 8),		// Raw image (ISO, GCM)
		DISC_FORMAT_SDK   = (1U << 8),		// Raw image with SDK header
		DISC_FORMAT_TGC   = (2U << 8),		// TGC (embedded disc image) (GCN only?)

		// WIA and RVZ formats are similar
		// TODO: Remove this once proper SparseDiscReader subclasses are added.
		DISC_FORMAT_WIA   = (3U << 8),		// WIA image (Header only!)
		DISC_FORMAT_RVZ   = (4U << 8),		// RVZ image (Header only!)

		DISC_FORMAT_PARTITION = (0xFEU << 8),	// Standalone Wii partition
		DISC_FORMAT_UNKNOWN = (0xFFU << 8),
		DISC_FORMAT_MASK = (0xFFU << 8),
	};

	// Disc type
	int discType;

	// Do we have certain things loaded?
	bool hasRegionCode;
	bool hasRvlRegionSetting;
	bool hasDiscHeader;	// true most of the time, except inc values update partitions

	// Disc header
	GCN_DiscHeader discHeader;
	RVL_RegionSetting regionSetting;

	// Region code (bi2.bin for GCN, RVL_RegionSetting for Wii.)
	uint32_t gcnRegion;

	// Disc reader
	IDiscReaderPtr discReader;

	// opening.bnr
	struct {
		GcnPartitionPtr gcnPartition;	// GcnPartition for opening.bnr
		RomDataPtr romData;		// either GameCubeBNR or WiiBNR
	} opening_bnr;

	/**
	 * Wii partition tables.
	 * Decoded from the actual on-disc tables.
	 */
	struct WiiPartEntry {
		off64_t start;		// Starting address, in bytes
		off64_t size;		// Estimated partition size, in bytes

		WiiPartitionPtr partition;	// Partition object
		uint32_t type;		// Partition type (See WiiPartitionType.)
		uint8_t vg;		// Volume group number
		uint8_t pt;		// Partition number
	};
	vector<WiiPartEntry> wiiPtbl;

	// Pointers to specific partitions within wiiPtbl.
	WiiPartition *updatePartition;
	WiiPartition *gamePartition;

	/**
	 * Is this a NASOS format disc image?
	 *
	 * RomDataFactory handles SparseDiscReader subclasses itself now,
	 * so this works by checking if d->file is a NASOSReader.
	 *
	 * @return True if this is a NASOS format disc image.
	 */
	bool isNASOSFormatDiscImage(void) const
	{
		const NASOSReader *const nasos = dynamic_cast<NASOSReader*>(file.get());
		return (nasos != nullptr);
	}

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
	 * Load opening.bnr.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadOpeningBnr(void);

	/**
	 * Add the game information field from opening.bnr.
	 *
	 * GameCube:
	 * - This adds an RFT_STRING field for BNR1, and
	 *   RFT_STRING_MULTI for BNR2.
	 *
	 * Wii:
	 * - This adds an RFT_STRING_MULTI field with all available languages.
	 *
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int addGameInfo(void);

	/**
	 * Get the encryption status of a partition. (Wii only)
	 *
	 * This is used to check if the encryption keys are available
	 * for a partition, or if not, why not.
	 *
	 * @param partition Partition to check.
	 * @return nullptr if partition is readable; error message if not.
	 */
	const char *wii_getCryptoStatus(const WiiPartition *partition) const;

	/**
	 * Get the game ID, with unprintable characters replaced with '_'.
	 * @return Game ID
	 */
	inline string getGameID(void) const;

	/**
	 * Get the required IOS version. (Wii only)
	 * @return IOS version, or empty string on error.
	 */
	string wii_getIOSVersion(void) const;

	/**
	 * Get the encryption key name for a WiiPartition.
	 * @param partition WiiPartition
	 * @return Encryption key name, or nullptr on error.
	 */
	const char *wii_getEncryptionKeyName(const WiiPartition *partition) const;
};

ROMDATA_IMPL(GameCube)
ROMDATA_IMPL_IMG(GameCube)

/** GameCubePrivate **/

/* RomDataInfo */
const array<const char*, 13+1> GameCubePrivate::exts = {{
	".gcm", ".rvm",
	".wbfs",
	".ciso", ".cso",
	".tgc",
	".dec",	// .iso.dec
	".gcz",
	".dpf", ".rpf",

	// Partially supported. (Header only!)
	".wia",
	".rvz",	// based on WIA

	// NOTE: May cause conflicts on Windows
	// if fallback handling isn't working.
	".iso",

	nullptr
}};
const array<const char*, 14+1> GameCubePrivate::mimeTypes = {{
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-gamecube-rom",
	"application/x-gamecube-iso-image",
	"application/x-gamecube-tgc",
	"application/x-wii-rom",
	"application/x-wii-iso-image",
	"application/x-wbfs",
	"application/x-wia",

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-cso",		// technically a different format...
	"application/x-compressed-iso",	// KDE detects CISO as this
	"application/x-nasos-image",
	"application/x-gcz-image",
	"application/x-dpf-image",
	"application/x-rpf-image",
	"application/x-rvz-image",

	nullptr
}};
const RomDataInfo GameCubePrivate::romDataInfo = {
	"GameCube", exts.data(), mimeTypes.data()
};

// NDDEMO header
const array<uint8_t, 64> GameCubePrivate::nddemo_header = {{
	0x30, 0x30, 0x00, 0x45, 0x30, 0x31, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x4E, 0x44, 0x44, 0x45, 0x4D, 0x4F, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
}};

GameCubePrivate::GameCubePrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, discType(DISC_UNKNOWN)
	, hasRegionCode(false)
	, hasRvlRegionSetting(false)
	, hasDiscHeader(false)
	, gcnRegion(~0U)
	, updatePartition(nullptr)
	, gamePartition(nullptr)
{
	// Clear the various structs.
	memset(&discHeader, 0, sizeof(discHeader));
	memset(&regionSetting, 0, sizeof(regionSetting));
}

GameCubePrivate::~GameCubePrivate()
{
	// Wii partition pointers.
	updatePartition = nullptr;
	gamePartition = nullptr;

	// Clear the existing partition table vector.
	wiiPtbl.clear();
}

/**
 * Load the Wii volume group and partition tables.
 * Partition tables are loaded into wiiPtbl.
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCubePrivate::loadWiiPartitionTables(void)
{
	if (!wiiPtbl.empty()) {
		// Partition tables have already been loaded.
		return 0;
	} else if (!this->file || !this->file->isOpen() || !this->discReader) {
		// File isn't open.
		return -EBADF;
	} else if ((discType & DISC_SYSTEM_MASK) != DISC_SYSTEM_WII) {
		// Unsupported disc type.
		return -EIO;
	}

	// Assuming a maximum of 128 partitions per table.
	// (This is a rather high estimate.)
	RVL_VolumeGroupTable vgtbl;
	array<RVL_PartitionTableEntry, 1024> pt;

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
	const off64_t discSize = discReader->size();
	if (discSize < 0) {
		// Error getting the size of the disc image.
		return -errno;
	}

	// Check the crypto and hash method.
	unsigned int cryptoMethod = 0;
	if (discHeader.disc_noCrypto != 0 || isNASOSFormatDiscImage()) {
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
		} else if (count > pt.size()) {
			count = static_cast<unsigned int>(pt.size());
		}

		// Read the partition table entries.
		const off64_t pt_addr = vgtbl.vg[i].addr.geto_be();
		static constexpr size_t pt_size = pt.size() * sizeof(pt[0]);
		size = discReader->seekAndRead(pt_addr, pt.data(), pt_size);
		if (size != pt_size) {
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
			entry.start = pt[j].addr.geto_be();
			entry.type = be32_to_cpu(pt[j].type);
		}
	}
	if (wiiPtbl.empty()) {
		// No partitions...
		return -ENOENT;
	}

	// Sort partitions by starting address in order to calculate the sizes.
	std::sort(wiiPtbl.begin(), wiiPtbl.end(),
		[](const WiiPartEntry &a, const WiiPartEntry &b) noexcept -> bool {
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
		[](const WiiPartEntry &a, const WiiPartEntry &b) noexcept -> bool {
			if (a.vg < b.vg) return true;
			if (a.vg > b.vg) return false;

			if (a.pt < b.pt) return true;
			/*if (a.pt > b.pt)*/ return false;
		}
	);

	// Create the WiiPartition objects.
	for (auto &p : wiiPtbl) {
		p.partition = std::make_shared<WiiPartition>(discReader, p.start, p.size,
			(WiiPartition::CryptoMethod)cryptoMethod);

		if (p.type == RVL_PT_UPDATE && !updatePartition) {
			// System Update partition.
			updatePartition = p.partition.get();
		} else if (p.type == RVL_PT_GAME && !gamePartition) {
			// Game partition.
			gamePartition = p.partition.get();
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
	if (isalnum_ascii(discHeader.company[0]) &&
	    isalnum_ascii(discHeader.company[1]))
	{
		// Disc ID is alphanumeric.
		const array<char, 3> s_company = {{
			discHeader.company[0],
			discHeader.company[1],
			'\0'
		}};
		return fmt::format(FRUN(C_("RomData", "Unknown ({:s})")), s_company.data());
	}

	// Disc ID is not alphanumeric.
	return fmt::format(FRUN(C_("RomData", "Unknown ({:0>2X} {:0>2X})")),
		static_cast<uint8_t>(discHeader.company[0]),
		static_cast<uint8_t>(discHeader.company[1]));
}

/**
 * Load opening.bnr.
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCubePrivate::loadOpeningBnr(void)
{
	if (opening_bnr.romData) {
		// Banner is already loaded.
		return 0;
	}

	// FIXME: No DiscReader for WIA or RVZ yet.
	switch (discType & DISC_FORMAT_MASK) {
		default:
			break;
		case DISC_FORMAT_WIA:
		case DISC_FORMAT_RVZ:
			return -EIO;
	}

	assert((bool)discReader);
	assert((discType & DISC_SYSTEM_MASK) != DISC_SYSTEM_TRIFORCE);
	if (!discReader) {
		return -EIO;
	} else if ((discType & DISC_SYSTEM_MASK) == DISC_SYSTEM_TRIFORCE) {
		// Not supported.
		// TODO: Do Triforce games have an equivalent to opening.bnr?
		return -ENOTSUP;
	}

	GcnPartition *partition = nullptr;
	IRpFilePtr f_opening_bnr;
	switch (discType & DISC_SYSTEM_MASK) {
		default:
			assert(!"System not supported!");
			return -ENOTSUP;

		case DISC_SYSTEM_GCN: {
			// NOTE: The GCN partition needs to stay open,
			// since we have a subclass for reading the object.
			// since we don't need to access more than one file.
			opening_bnr.gcnPartition = std::make_shared<GcnPartition>(discReader, 0);
			if (!opening_bnr.gcnPartition->isOpen()) {
				// Could not open the partition.
				opening_bnr.gcnPartition.reset();
				return -EIO;
			}

			partition = opening_bnr.gcnPartition.get();
			break;
		}

		case DISC_SYSTEM_WII: {
			// Wii partition tables need to be loaded first.
			// They might have already been loaded by loadFieldData(),
			// but not loadMetaData().
			const int wiiPtLoaded = loadWiiPartitionTables();
			if (wiiPtLoaded != 0) {
				// Unable to load Wii partition tables.
				return -EIO;
			}
			partition = gamePartition;
			break;
		}
	}
	assert(partition != nullptr);
	if (!partition) {
		opening_bnr.gcnPartition.reset();
		return -EIO;
	}

	// Attempt to open "opening.bnr".
	f_opening_bnr = partition->open("/opening.bnr");
	if (!f_opening_bnr) {
		// Error opening "opening.bnr".
		const int err = -partition->lastError();
		opening_bnr.gcnPartition.reset();
		return err;
	}

	// Attempt to open the appropriate subclass.
	RomDataPtr romData;
	switch (discType & DISC_SYSTEM_MASK) {
		default:
			assert(!"System not supported!");
			return -ENOTSUP;
		case DISC_SYSTEM_GCN:
			romData = std::make_shared<GameCubeBNR>(f_opening_bnr, this->gcnRegion);
			break;
		case DISC_SYSTEM_WII:
			romData = std::make_shared<WiiBNR>(f_opening_bnr, this->gcnRegion, discHeader.id4[3]);
			break;
	}
	if (!romData->isOpen()) {
		// Unable to open the subclass.
		opening_bnr.gcnPartition.reset();
		return -EIO;
	}

	// GameCubeBNR subclass is open.
	opening_bnr.romData = std::move(romData);;
	return 0;
}

/**
 * Add the game information field from opening.bnr.
 *
 * GameCube:
 * - This adds an RFT_STRING field for BNR1, and
 *   RFT_STRING_MULTI for BNR2.
 *
 * Wii:
 * - This adds an RFT_STRING_MULTI field with all available languages.
 *
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCubePrivate::addGameInfo(void)
{
	assert((discType & DISC_SYSTEM_MASK) != DISC_SYSTEM_TRIFORCE);
	if ((discType & DISC_SYSTEM_MASK) == DISC_SYSTEM_TRIFORCE) {
		// Not supported.
		// TODO: Do Triforce games have opening.bnr?
		return -ENOENT;
	}

	if (!opening_bnr.romData) {
		// Attempt to load opening.bnr.
		if (const_cast<GameCubePrivate*>(this)->loadOpeningBnr() != 0) {
			// Error loading opening.bnr.
			return -ENOENT;
		}

		// Make sure it was actually loaded.
		if (!opening_bnr.romData) {
			// opening.bnr was not loaded.
			return -EIO;
		}
	}

	// Add the field from the opening.bnr RomData object.
	int ret = 0;
	switch (discType & DISC_SYSTEM_MASK) {
		default:
			assert(!"System not supported!");
			ret = -ENOTSUP;
			break;
		case DISC_SYSTEM_GCN: {
			GameCubeBNR *const bnr = dynamic_cast<GameCubeBNR*>(opening_bnr.romData.get());
			assert(bnr != nullptr);
			if (!bnr) {
				return -EIO;
			}
			ret = bnr->addField_gameInfo(&this->fields);
			break;
		}
		case DISC_SYSTEM_WII: {
			const RomFields *const imetFields = opening_bnr.romData->fields();
			assert(imetFields != nullptr);
			if (imetFields) {
				fields.addFields_romFields(imetFields, 0);
			}
			break;
		}
	}
	return ret;
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
const char *GameCubePrivate::wii_getCryptoStatus(const WiiPartition *partition) const
{
	const KeyManager::VerifyResult res = partition->verifyResult();
	if (res == KeyManager::VerifyResult::KeyNotFound) {
		// This may be an invalid key index.
		if (partition->encKey() == WiiTicket::EncryptionKeys::Unknown) {
			// Invalid key index.
			return C_("Wii", "Invalid common key index.");
		}
	}

	if (res == KeyManager::VerifyResult::IncrementingValues) {
		// Debug discs may have incrementing values instead of a
		// valid update partition.
		return C_("Wii", "Incrementing values");
	}

	const char *err = KeyManager::verifyResultToString(res);
	if (!err) {
		err = C_("RomData", "Unknown error. (THIS IS A BUG!)");
	}
	return err;
}

/**
 * Get the game ID, with unprintable characters replaced with '_'.
 * @return Game ID
 */
inline string GameCubePrivate::getGameID(void) const
{
	// Replace any non-printable characters with underscores.
	// (GameCube NDDEMO has ID6 "00\0E01".)
	string id6;
	id6.resize(6, '_');
	for (size_t i = 0; i < 6; i++) {
		if (ISPRINT(discHeader.id6[i])) {
			id6[i] = discHeader.id6[i];
		}
	}
	return id6;
}

/**
 * Get the required IOS version. (Wii only)
 * @return IOS version, or empty string on error.
 */
string GameCubePrivate::wii_getIOSVersion(void) const
{
	assert((discType & DISC_SYSTEM_MASK) == DISC_SYSTEM_WII);
	assert(gamePartition != nullptr);
	if ((discType & DISC_SYSTEM_MASK) != DISC_SYSTEM_WII || !gamePartition) {
		return {};
	}

	const RVL_TMD_Header *const tmdHeader = gamePartition->tmdHeader();
	assert(tmdHeader != nullptr);
	if (!tmdHeader) {
		return {};
	}

	const uint32_t ios_lo = be32_to_cpu(tmdHeader->sys_version.lo);
	if (tmdHeader->sys_version.hi == cpu_to_be32(0x00000001) &&
	    ios_lo > 2 && ios_lo < 0x300)
	{
		// Standard IOS slot.
		return fmt::format(FSTR("IOS{:d}"), ios_lo);
	} else if (tmdHeader->sys_version.id != 0) {
		// Non-standard IOS slot.
		// Print the full title ID.
		return fmt::format(FSTR("{:0>8X}-{:0>8X}"),
			be32_to_cpu(tmdHeader->sys_version.hi),
			be32_to_cpu(tmdHeader->sys_version.lo));
	}

	// No IOS version...
	return {};
}

/**
 * Get the encryption key name for a WiiPartition.
 * @param partition WiiPartition
 * @return Encryption key name, or nullptr on error.
 */
const char *GameCubePrivate::wii_getEncryptionKeyName(const WiiPartition *partition) const
{
	WiiTicket::EncryptionKeys encKey;
	if (isNASOSFormatDiscImage()) {
		// NASOS disc image.
		// If this would normally be an encrypted image, use encKeyReal().
		encKey = (discHeader.disc_noCrypto == 0
			? partition->encKeyReal()
			: partition->encKey());
	} else {
		// Other disc image. Use encKey().
		encKey = partition->encKey();
	}

	return WiiTicket::encKeyName_static(encKey);
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
GameCube::GameCube(const IRpFilePtr &file)
	: super(new GameCubePrivate(file))
{
	// This class handles disc images.
	RP_D(GameCube);
	d->fileType = FileType::DiscImage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the disc header.
	uint8_t header[4096+256];
	d->file->rewind();
	size_t size = d->file->read(&header, sizeof(header));
	if (size != sizeof(header)) {
		d->file.reset();
		return;
	}

	// Check if this disc image is supported.
	const char *const filename = file->filename();
	const DetectInfo info = {
		{0, sizeof(header), header},
		FileSystem::file_ext(filename),	// ext
		0		// szFile (not needed for GameCube)
	};
	d->discType = isRomSupported_static(&info);

	d->isValid = (d->discType >= 0);
	if (!d->isValid) {
		// Not supported.
		d->file.reset();
		return;
	}

	// TODO: DiscReaderFactory?
	// TODO: More MIME types for e.g. Triforce, CISO, TGC, etc.
	switch (d->discType & GameCubePrivate::DISC_FORMAT_MASK) {
		case GameCubePrivate::DISC_FORMAT_RAW:
		case GameCubePrivate::DISC_FORMAT_PARTITION:
			d->discReader = std::make_shared<DiscReader>(d->file);
			break;
		case GameCubePrivate::DISC_FORMAT_SDK:
			// Skip the SDK header.
			d->discReader = std::make_shared<DiscReader>(d->file, 32768, -1);
			break;

		case GameCubePrivate::DISC_FORMAT_TGC: {
			d->mimeType = "application/x-gamecube-tgc";
			d->fileType = FileType::EmbeddedDiscImage;
			// Check the TGC header for the disc offset.
			const GCN_TGC_Header *tgcHeader = reinterpret_cast<const GCN_TGC_Header*>(header);
			const uint32_t gcm_offset = be32_to_cpu(tgcHeader->header_size);
			d->discReader = std::make_shared<DiscReader>(d->file, gcm_offset, -1);
			break;
		}

		case GameCubePrivate::DISC_FORMAT_WIA:
			// TODO: Implement WiaReader.
			// For now, only the header will be readable.
			d->mimeType = "application/x-wia";
			d->discReader = nullptr;
			break;
		case GameCubePrivate::DISC_FORMAT_RVZ:
			// TODO: Implement RvzReader.
			// For now, only the header will be readable.
			d->mimeType = "application/x-rvz-image";
			d->discReader = nullptr;
			break;

		case GameCubePrivate::DISC_FORMAT_UNKNOWN:
		default:
			d->discType = GameCubePrivate::DISC_UNKNOWN;
			break;
	}

	// Check if the disc type is still valid.
	d->isValid = (d->discType >= 0);
	if (!d->isValid) {
		// Not supported.
		d->file.reset();
		return;
	}

	if (!d->discReader) {
		// No WiaReader or RvzReader yet. If this is WIA or RVZ,
		// retrieve the header from header[].
		switch (d->discType & GameCubePrivate::DISC_FORMAT_MASK) {
			case GameCubePrivate::DISC_FORMAT_WIA:
			case GameCubePrivate::DISC_FORMAT_RVZ:
				// GCN/Wii header starts at 0x58.
				memcpy(&d->discHeader, &header[0x58], sizeof(d->discHeader));
				break;
			default:
				// Non-WIA formats must have a valid DiscReader.
				goto notSupported;
		}

		// We have the disc header.
		// NOTE: Only the disc ID region code is present.
		d->hasDiscHeader = true;
		d->hasRegionCode = true;

		// Done for now...
		return;
	}

	if (!d->discReader->isOpen()) {
		// Error opening the DiscReader.
		goto notSupported;
	}

	// Save the disc header for later.
	d->discReader->rewind();
	if ((d->discType & GameCubePrivate::DISC_FORMAT_MASK) != GameCubePrivate::DISC_FORMAT_PARTITION) {
		// Regular disc image.
		size = d->discReader->read(&d->discHeader, sizeof(d->discHeader));
		if (size != sizeof(d->discHeader)) {
			// Error reading the disc header.
			goto notSupported;
		}
		d->hasDiscHeader = true;
		d->hasRegionCode = true;
	} else {
		// Standalone partition.
		d->fileType = FileType::Partition;
		d->wiiPtbl.resize(1);
		GameCubePrivate::WiiPartEntry &pt = d->wiiPtbl[0];

		// Open the partition.
		// TODO: Identify channel partitions by the title ID high?
		pt.start = 0;
		pt.size = d->file->size();
		pt.vg = 0;
		pt.pt = 0;
		pt.partition = std::make_shared<WiiPartition>(d->discReader, pt.start, pt.size);

		// Determine the partition type.
		// TODO: Super Smash Bros. Brawl "Masterpieces" partitions.
		// TODO: Check tid.hi?
		const Nintendo_TitleID_BE_t tid = pt.partition->titleID();
		if (tid.lo == be32_to_cpu('UPD') ||	// IOS only
		    tid.lo == be32_to_cpu('UPE') ||	// USA region
		    tid.lo == be32_to_cpu('UPJ') ||	// JPN region
		    tid.lo == be32_to_cpu('UPP') ||	// EUR region
		    tid.lo == be32_to_cpu('UPK') ||	// KOR region
		    tid.lo == be32_to_cpu('UPC'))	// CHN region (maybe?)
		{
			// Update partition.
			// Last character is title ID, except for 'UPD' which is 'all regions'.
			// (Usually used for IOS-only update paritions.)
			pt.type = RVL_PT_UPDATE;
			d->updatePartition = pt.partition.get();
		} else if (tid.lo == be32_to_cpu('INS')) {
			// Channel partition.
			pt.type = RVL_PT_CHANNEL;
		} else {
			// Game partition.
			// TODO: Extract partitions from Brawl and check.
			pt.type = RVL_PT_GAME;
			d->gamePartition = pt.partition.get();
		}

		// Read the partition header.
		size = pt.partition->read(&d->discHeader, sizeof(d->discHeader));
		if (size == sizeof(d->discHeader)) {
			d->hasDiscHeader = true;
		} else {
			// Read failed. This may happen if we have an Incrementing Values
			// update partition.
			d->hasDiscHeader = false;
			if (pt.partition->verifyResult() != KeyManager::VerifyResult::IncrementingValues) {
				// Not Incrementing Values.
				// Error reading the partition header.
				pt.partition.reset();
				d->wiiPtbl.clear();
				goto notSupported;
			}
		}

		// Need to change encryption bytes to 00.
		// TODO: Only if actually encrypted?
		d->discHeader.hash_verify = 0;
		d->discHeader.disc_noCrypto = 0;

		// TODO: Figure out region code for standalone partitions.
		d->hasRegionCode = false;
	}

	if (((d->discType & GameCubePrivate::DISC_SYSTEM_MASK) != GameCubePrivate::DISC_SYSTEM_UNKNOWN) ||
	      d->isNASOSFormatDiscImage())
	{
		// Verify that the NASOS header matches the disc format.
		bool isOK = true;
		switch (d->discType & GameCubePrivate::DISC_SYSTEM_MASK) {
			case GameCubePrivate::DISC_SYSTEM_GCN:
				// Must have GCN magic number or nddemo header.
				if (d->discHeader.magic_gcn == cpu_to_be32(GCN_MAGIC)) {
					// GCN magic number is present.
					break;
				} else if (!memcmp(&d->discHeader, GameCubePrivate::nddemo_header.data(),
				                   GameCubePrivate::nddemo_header.size()))
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
			goto notSupported;
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
		} else if (!memcmp(&d->discHeader, GameCubePrivate::nddemo_header.data(),
			           GameCubePrivate::nddemo_header.size()))
		{
			// NDDEMO disc.
			d->discType &= ~GameCubePrivate::DISC_SYSTEM_MASK;
			d->discType |=  GameCubePrivate::DISC_SYSTEM_GCN;
		} else {
			// Unknown system type.
			goto notSupported;
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
				goto notSupported;
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
					goto notSupported;
				}

				d->gcnRegion = be32_to_cpu(d->regionSetting.region_code);
				d->hasRvlRegionSetting = true;
			}
			break;

		default:
			// Unknown system.
			goto notSupported;
	}

	if (!d->mimeType) {
		// MIME type hasn't been set yet.
		// Set it based on the system ID.
		switch (d->discType & GameCubePrivate::DISC_SYSTEM_MASK) {
			case GameCubePrivate::DISC_SYSTEM_GCN:
			case GameCubePrivate::DISC_SYSTEM_TRIFORCE:
				// TODO: Separate MIME type for Triforce.
				d->mimeType = "application/x-gamecube-iso-image";
				break;
			case GameCubePrivate::DISC_SYSTEM_WII:
				d->mimeType = "application/x-wii-iso-image";
				break;
			default:
				assert(!"Invalid system ID...");
				break;
		}
	}

	// Is PAL?
	d->isPAL = (d->gcnRegion == GCN_REGION_EUR);

	// Disc image loaded successfully.
	return;

notSupported:
	// This disc image is not supported.
	d->discReader.reset();
	d->file.reset();
	d->discType = GameCubePrivate::DISC_UNKNOWN;
	d->isValid = false;
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
				if (d->opening_bnr.romData) {
					d->opening_bnr.romData->close();
				}
				d->opening_bnr.gcnPartition.reset();
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
	    info->header.size < 0x100)
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
	if (!memcmp(gcn_header, GameCubePrivate::nddemo_header.data(),
	            GameCubePrivate::nddemo_header.size()))
	{
		// NDDEMO disc.
		return (GameCubePrivate::DISC_SYSTEM_GCN | GameCubePrivate::DISC_FORMAT_RAW);
	}

	// Check for SDK headers.
	// TODO: More comprehensive?
	// TODO: Checksum at 0x0830. (For GCN, makeGCM always puts 0xAB0B here...)
	static constexpr uint32_t sdk_0x0000 = 0xFFFF0000;	// BE32
	// TODO: Some RVMs have extended headers at 0x0800, but not the 0x082C value.
	static constexpr uint32_t sdk_0x0820 = 0x0002F000;	// BE32
	static constexpr uint32_t sdk_0x082C = 0x0000E006;	// BE32
	const uint32_t *const pData32 =
		reinterpret_cast<const uint32_t*>(info->header.pData);
	if (pData32[0] == cpu_to_be32(sdk_0x0000)) {
		if (info->header.size < 0x0830) {
			// Can't check 0x082C, so assume it has the SDK headers.
			return (GameCubePrivate::DISC_SYSTEM_UNKNOWN | GameCubePrivate::DISC_FORMAT_SDK);
		}

		if (pData32[0x082C/4] == cpu_to_be32(sdk_0x082C) ||
		    pData32[0x0820/4] == cpu_to_be32(sdk_0x0820))
		{
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

	// Check for WIA or RVZ.
	// TODO: Remove this when a proper SparseDiscReader subclass is added.
	static constexpr uint32_t wia_magic = 'WIA\x01';
	static constexpr uint32_t rvz_magic = 'RVZ\x01';
	if (pData32[0] == cpu_to_be32(rvz_magic) ||
	    unlikely(pData32[0] == cpu_to_be32(wia_magic)))
	{
		// This is a WIA or RVZ image.
		const uint32_t disc_format = (likely(pData32[0] == cpu_to_be32(rvz_magic)))
			? GameCubePrivate::DISC_FORMAT_RVZ
			: GameCubePrivate::DISC_FORMAT_WIA;

		// NOTE: We're using the WIA system ID if it's valid.
		// Otherwise, fall back to GCN/Wii magic.
		switch (be32_to_cpu(pData32[0x48/sizeof(uint32_t)])) {
			case 1:
				// GameCube disc image. (WIA format)
				return (GameCubePrivate::DISC_SYSTEM_GCN | disc_format);
			case 2:
				// Wii disc image. (WIA format)
				return (GameCubePrivate::DISC_SYSTEM_WII | disc_format);
			default:
				break;
		}

		// Check the GameCube/Wii magic.
		// TODO: WIA struct when full WIA support is added.
		gcn_header = reinterpret_cast<const GCN_DiscHeader*>(&info->header.pData[0x58]);
		if (gcn_header->magic_wii == cpu_to_be32(WII_MAGIC)) {
			// Wii disc image. (WIA format)
			return (GameCubePrivate::DISC_SYSTEM_WII | disc_format);
		} else if (gcn_header->magic_gcn == cpu_to_be32(GCN_MAGIC)) {
			// GameCube disc image. (WIA format)
			return (GameCubePrivate::DISC_SYSTEM_GCN | disc_format);
		}

		// Unrecognized WIA image...
		return (GameCubePrivate::DISC_SYSTEM_UNKNOWN | disc_format);
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
	static const array<array<const char*, 4>, 4> sysNames = {{
		{{"Nintendo GameCube", "GameCube", "GCN", nullptr}},
		{{"Nintendo/Sega/Namco Triforce", "Triforce", "TF", nullptr}},
		{{"Nintendo Wii", "Wii", "Wii", nullptr}},
		{{nullptr, nullptr, nullptr, nullptr}},
	}};

	// Special check for GCN abbreviation in Japan.
	if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_ROM_LOCAL) {
		// Localized system name.
		if (((d->discType & 3) == GameCubePrivate::DISC_SYSTEM_GCN) &&
		    ((type & SYSNAME_TYPE_MASK) == SYSNAME_TYPE_ABBREVIATION))
		{
			// GameCube abbreviation.
			// If this is Japan or South Korea, use "NGC".
			const uint32_t cc = SystemRegion::getCountryCode();
			if (cc == 'JP' || cc == 'KR') {
				return "NGC";
			}
		}
	}

	return sysNames[d->discType & 3][type & SYSNAME_TYPE_MASK];
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
		case IMG_INT_BANNER:
			return {{nullptr, 96, 32, 0}};
		case IMG_EXT_MEDIA:
			return {{nullptr, 160, 160, 0}};
		case IMG_EXT_COVER:
			return {{nullptr, 160, 224, 0}};
		case IMG_EXT_COVER_3D:
			return {{nullptr, 176, 248, 0}};
		case IMG_EXT_COVER_FULL:
			return {
				{nullptr, 512, 340, 0},
				{"HQ", 1024, 680, 1},
			};
		default:
			break;
	}

	// Unsupported image type.
	return {};
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
	if (!d->fields.empty()) {
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
	// - Wii only: 6
	d->fields.reserve(7+6);

	// TODO: Trim the titles. (nulls, spaces)
	// NOTE: The titles are dup()'d as C strings, so maybe not nulls.
	// TODO: Display the disc image format?

	// Disc header is normally present, except for standalone
	// Wii update partitions with incrementing values.
	if (d->hasDiscHeader) {
		// Game title
		// TODO: Is Shift-JIS actually permissible here?
		const char *const title_title = C_("RomData", "Title");
		switch (d->gcnRegion) {
			case GCN_REGION_USA:
			case GCN_REGION_EUR:
			case GCN_REGION_ALL:	// TODO: Assume JP?
			default:
				// USA/PAL uses cp1252.
				d->fields.addField_string(title_title, cp1252_to_utf8(
						discHeader->game_title, sizeof(discHeader->game_title)));
				break;

			case GCN_REGION_JPN:
			case GCN_REGION_KOR:
			case GCN_REGION_CHN:
			case GCN_REGION_TWN:
				// Japan uses Shift-JIS.
				d->fields.addField_string(title_title, cp1252_sjis_to_utf8(
						discHeader->game_title, sizeof(discHeader->game_title)));
				break;
		}

		// Game ID
		d->fields.addField_string(C_("RomData", "Game ID"), d->getGameID());

		// Publisher
		d->fields.addField_string(C_("RomData", "Publisher"), d->getPublisher());

		// Other disc header fields
		d->fields.addField_string_numeric(C_("RomData", "Disc #"),
			discHeader->disc_number+1, RomFields::Base::Dec);
		d->fields.addField_string_numeric(C_("RomData", "Revision"),
			discHeader->revision, RomFields::Base::Dec, 2);
	}

	// The remaining fields are not located in the disc header.
	// If we can't read the disc contents for some reason, e.g.
	// unimplemented DiscReader (WIA), skip the fields.
	if (!d->discReader) {
		// Cannot read the disc contents.
		// We're done for now.
		return static_cast<int>(d->fields.count());
	}

	// Region code
	// bi2.bin and/or RVL_RegionSetting is loaded in the constructor,
	// and the region code is stored in d->gcnRegion.
	if (d->hasRegionCode) {
		bool isDefault = true;	// assuming the ID4 represents the disc region if nothing else says otherwise
		const char *region;
		if ((d->discType & GameCubePrivate::DISC_SYSTEM_MASK) != GameCubePrivate::DISC_SYSTEM_WII) {
			// GameCube: Only 0 (JPN), 1 (USA), and 2 (EUR) are valid.
			// 3 (ALL) is valid only when using certain debugging hardware.
			if (d->gcnRegion <= GCN_REGION_EUR) {
				region = GameCubeRegions::gcnRegionToString(d->gcnRegion, discHeader->id4[3], &isDefault);
			} else if (d->gcnRegion == GCN_REGION_ALL) {
				region = C_("GameCube", "Region-Free (with certain debugging hardware)");
			} else {
				// Not valid.
				region = nullptr;
			}
		} else {
			// Wii: All region codes are valid.
			region = GameCubeRegions::gcnRegionToString(d->gcnRegion, discHeader->id4[3], &isDefault);
		}
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
				// tr: {0:s} == full region name, {1:s} == abbreviation
				s_region = fmt::format(FRUN(C_("Wii", "{0:s} ({1:s})")), region, suffix);
			} else {
				s_region = region;
			}

			d->fields.addField_string(region_code_title, s_region);
		} else {
			// Invalid region code.
			d->fields.addField_string(region_code_title,
				fmt::format(FRUN(C_("RomData", "Unknown (0x{:0>8X})")), d->gcnRegion));
		}

		if ((d->discType & GameCubePrivate::DISC_SYSTEM_MASK) != GameCubePrivate::DISC_SYSTEM_WII) {
			// GameCube-specific fields.

			// Add the Game Info field from opening.bnr.
			d->addGameInfo();

			// Finished reading the field data.
			return static_cast<int>(d->fields.count());
		}
	}

	/** Wii-specific fields **/

	// Load the Wii partition tables.
	const int wiiPtLoaded = d->loadWiiPartitionTables();

	// TMD fields
	if (d->gamePartition) {
		const RVL_TMD_Header *const tmdHeader = d->gamePartition->tmdHeader();
		if (tmdHeader) {
			// Title ID
			// TID Lo is usually the same as the game ID,
			// except for some diagnostics discs.
			d->fields.addField_string(C_("Nintendo", "Title ID"),
				fmt::format(FSTR("{:0>8X}-{:0>8X}"),
					be32_to_cpu(tmdHeader->title_id.hi),
					be32_to_cpu(tmdHeader->title_id.lo)));

			// Access rights
			vector<string> *const v_access_rights_hdr = new vector<string>({
				"AHBPROT",
				C_("Wii", "DVD Video")
			});
			d->fields.addField_bitfield(C_("Wii", "Access Rights"),
				v_access_rights_hdr, 0, be32_to_cpu(tmdHeader->access_rights));

			// Required IOS version
			// TODO: Is this the best place for it?
			const string s_ios_version = d->wii_getIOSVersion();
			if (!s_ios_version.empty()) {
				d->fields.addField_string(C_("Wii", "IOS Version"), s_ios_version);
			}
		}
	}

	// Get age rating(s). (Wii only)
	// RVL_RegionSetting is loaded in the constructor.
	// Note that not all 16 fields are present on Wii,
	// though the fields do match exactly, so no
	// mapping is necessary.
	if (d->hasRvlRegionSetting) {
		RomFields::age_ratings_t age_ratings;
		// Valid ratings: 0-1, 3-9
		static constexpr uint16_t valid_ratings = 0x3FB;

		for (int i = static_cast<int>(age_ratings.size())-1; i >= 0; i--) {
			if (!(valid_ratings & (1U << i))) {
				// Rating is not applicable for Wii.
				age_ratings[i] = 0;
				continue;
			}

			// Wii ratings field:
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
		d->fields.addField_ageRatings(C_("RomData", "Age Ratings"), age_ratings);
	}

	// Display the Wii partition table(s).
	if (wiiPtLoaded == 0) {
		// Add the game name from opening.bnr.
		int ret = d->addGameInfo();
		if (ret != 0) {
			// Unable to load the game name from opening.bnr.
			// This might be because it's homebrew, a prototype, or a key error.
			const char *const game_info_title = C_("GameCube", "Game Info");
			if (!d->gamePartition) {
				// No game partition.
				if ((d->discType & GameCubePrivate::DISC_FORMAT_MASK) != GameCubePrivate::DISC_FORMAT_PARTITION) {
					d->fields.addField_string(game_info_title,
						C_("GameCube", "ERROR: No game partition was found."));
				}
			} else if (d->gamePartition->verifyResult() != KeyManager::VerifyResult::OK) {
				// Key error.
				const char *status = d->wii_getCryptoStatus(d->gamePartition);
				d->fields.addField_string(game_info_title,
					fmt::format(FRUN(C_("GameCube", "ERROR: {:s}")),
						(status ? status : C_("GameCube", "Unknown"))));
			}
		}

		// Update version.
		unsigned int sysMenuVersion = 0;
		unsigned int ios_slot = 0, ios_major = 0, ios_minor = 0;
		unsigned int ios_retail_count = 0;
		time_t update_date = -1;	// from update.inf
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
			IFst::Dir *const dirp = d->updatePartition->opendir("/_sys/");
			if (dirp) {
				const IFst::DirEnt *dirent;
				while ((dirent = d->updatePartition->readdir(dirp)) != nullptr) {
					if (!dirent->name || dirent->type != DT_REG) {
						continue;
					}

					// Check for a retail System Menu.
					if (dirent->name[0] == 'R') {
						unsigned int version;
						int ret = sscanf(dirent->name, "RVL-WiiSystemmenu-v%u.wad", &version);
						if (ret == 1) {
							// Found a retail System Menu.
							sysMenuVersion = version;
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

			// Check if __update.inf exists.
			// If it does, read the datestamp.
			const IRpFilePtr update_inf = d->updatePartition->open("/__update.inf");
			if (update_inf) {
				char buf[11];
				size_t size = update_inf->read(buf, sizeof(buf));
				if (size == sizeof(buf) && buf[sizeof(buf)-1] == '\0') {
					struct tm tm;
					memset(&tm, 0, sizeof(tm));
					int ret = sscanf(buf, "%d/%d/%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
					// NOTE: timegm() silently produces the wrong values if the string data
					// is out of range, so verify it here.
					if (ret == 3 &&
					    tm.tm_year >= 2000 && tm.tm_year <= 2999 &&
					    tm.tm_mon >= 1 && tm.tm_mon <= 12 &&
					    tm.tm_mday >= 1 && tm.tm_mday <= 31)
					{
						// Adjust the year and month to match struct tm specifications.
						tm.tm_year -= 1900;
						tm.tm_mon -= 1;

						update_date = timegm(&tm);
					}
				}
			}
		}

		// tr: Update version included on this disc
		const char *const update_title = C_("Nintendo", "Update");
		if (isDebugIOS || ios_retail_count == 1) {
			d->fields.addField_string(update_title,
				fmt::format(FSTR("IOS{:d} {:d}.{:d} (v{:d})"),
					ios_slot, ios_major, ios_minor,
					(ios_major << 8) | ios_minor));
		} else {
			if (sysMenuVersion != 0) {
				// Look up the system menu version name.
				const char *const sysMenu = WiiSystemMenuVersion::lookup(sysMenuVersion);
				if (sysMenu) {
					d->fields.addField_string(update_title, sysMenu);
				} else {
					// Unknown system menu version.
					// Use the raw version number.
					d->fields.addField_string(update_title,
						fmt::format(FSTR("v{:d}"), sysMenuVersion));
				}
			} else {
				// No system menu version number.
				// FIXME: Sometimes shows "Something happened" even if it wasn't a crypto error.
				const char *updateStatus;
				if (!d->updatePartition) {
					updateStatus = C_("Nintendo", "None");
				} else {
					updateStatus = d->wii_getCryptoStatus(d->updatePartition);
				}
				d->fields.addField_string(update_title, updateStatus);
			}
		}

		if (update_date > -1) {
			d->fields.addField_dateTime(C_("Nintendo", "Update Date"), update_date,
				RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_IS_UTC);
		}

		// Partition table
		auto *const vv_partitions = new RomFields::ListData_t();
		vv_partitions->resize(d->wiiPtbl.size());

		auto src_iter = d->wiiPtbl.cbegin();
		auto dest_iter = vv_partitions->begin();
		const auto vv_partitions_end = vv_partitions->end();
		for ( ; dest_iter != vv_partitions_end; ++src_iter, ++dest_iter) {
			vector<string> &data_row = *dest_iter;
			data_row.reserve(5);	// 5 fields per row.

			// Partition entry
			const GameCubePrivate::WiiPartEntry &entry = *src_iter;

			// Partition number
			data_row.emplace_back(fmt::format(FSTR("{:d}p{:d}"), entry.vg, entry.pt));

			// Partition type
			string s_ptype;
			static const array<const char*, 3> part_type_tbl = {{
				// tr: GameCubePrivate::RVL_PT_GAME (Game partition)
				NOP_C_("Wii|Partition", "Game"),
				// tr: GameCubePrivate::RVL_PT_UPDATE (Update partition)
				NOP_C_("Wii|Partition", "Update"),
				// tr: GameCubePrivate::RVL_PT_CHANNEL (Channel partition)
				NOP_C_("Wii|Partition", "Channel"),
			}};
			if (entry.type <= RVL_PT_CHANNEL) {
				s_ptype = pgettext_expr("Wii|Partition", part_type_tbl[entry.type]);
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
				if (isalnum_ascii(part_type.chr[0]) && isalnum_ascii(part_type.chr[1]) &&
				    isalnum_ascii(part_type.chr[2]) && isalnum_ascii(part_type.chr[3]))
				{
					// All four bytes are ASCII letters and/or numbers.
					s_ptype = latin1_to_utf8(part_type.chr, sizeof(part_type.chr));
				} else {
					// Non-ASCII data. Print the hex values instead.
					s_ptype = fmt::format(FSTR("{:0>8X}"), entry.type);
				}
			}
			data_row.push_back(std::move(s_ptype));

			// Encryption key
			const char *s_key_name = d->wii_getEncryptionKeyName(entry.partition.get());
			if (!s_key_name) {
				// tr: EncryptionKeys::Unknown
				s_key_name = C_("RomData", "Unknown");
			}
			data_row.emplace_back(s_key_name);

			// Used size
			const off64_t used_size = entry.partition->partition_size_used();
			if (used_size >= 0) {
				data_row.push_back(LibRpText::formatFileSize(used_size));
			} else {
				// tr: Unknown used size.
				data_row.emplace_back(C_("Wii|Partition", "Unknown"));
			}

			// Partition size
			data_row.push_back(LibRpText::formatFileSize(entry.partition->partition_size()));
		}

		// Fields
		static const array<const char*, 5> partitions_names = {{
			// tr: Partition number.
			NOP_C_("Wii|Partition", "#"),
			// tr: Partition type.
			NOP_C_("Wii|Partition", "Type"),
			// tr: Encryption key.
			NOP_C_("Wii|Partition", "Key"),
			// tr: Actual data used within the partition.
			NOP_C_("Wii|Partition", "Used Size"),
			// tr: Total size of the partition.
			NOP_C_("Wii|Partition", "Total Size"),
		}};
		vector<string> *const v_partitions_names = RomFields::strArrayToVector_i18n(
			"Wii|Partition", partitions_names);

		RomFields::AFLD_PARAMS params;
		params.headers = v_partitions_names;
		params.data.single = vv_partitions;
		d->fields.addField_listData(C_("Wii", "Partitions"), &params);
	} else {
		// Could not load partition tables.
		// FIXME: Show an error?
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int GameCube::loadMetaData(void)
{
	RP_D(GameCube);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->discType < 0) {
		// Unknown disc type.
		return -EIO;
	}

	// Disc header is read in the constructor.
	const GCN_DiscHeader *const discHeader = &d->discHeader;
	d->metaData.reserve(6);	// Maximum of 6 metadata properties.

	// Add opening.bnr metadata if it's available.
	bool addedBnrMetaData = false;
	do {
		if ((d->discType & GameCubePrivate::DISC_SYSTEM_MASK) == GameCubePrivate::DISC_SYSTEM_TRIFORCE)
			break;

		if (!d->opening_bnr.romData) {
			d->loadOpeningBnr();
			if (!d->opening_bnr.romData) {
				// Still unable to load the metadata.
				break;
			}
		}

		// Get the metadata from opening.bnr.
		const RomMetaData *const bnrMetaData = d->opening_bnr.romData->metaData();
		if (bnrMetaData && !bnrMetaData->empty()) {
			int ret = d->metaData.addMetaData_metaData(bnrMetaData);
			if (ret >= 0) {
				// Metadata added successfully.
				addedBnrMetaData = true;
			}
		}
	} while (0);

	if (!addedBnrMetaData) {
		// Unable to load opening.bnr.
		// Use the disc header.

		// Title
		// TODO: Use opening.bnr title for GameCube instead?
		// TODO: Is Shift-JIS actually permissible here?
		switch (d->gcnRegion) {
			case GCN_REGION_USA:
			case GCN_REGION_EUR:
			case GCN_REGION_ALL:	// TODO: Assume JP?
			default:
				// USA/PAL uses cp1252.
				d->metaData.addMetaData_string(Property::Title,
					cp1252_to_utf8(
						discHeader->game_title, sizeof(discHeader->game_title)));
				break;

			case GCN_REGION_JPN:
			case GCN_REGION_KOR:
			case GCN_REGION_CHN:
			case GCN_REGION_TWN:
				// Japan uses Shift-JIS.
				d->metaData.addMetaData_string(Property::Title,
					cp1252_sjis_to_utf8(
						discHeader->game_title, sizeof(discHeader->game_title)));
				break;
		}

		// Publisher
		d->metaData.addMetaData_string(Property::Publisher, d->getPublisher());
	}

	// TODO: Disc number?

	/** Custom properties! **/

	// Game ID
	d->metaData.addMetaData_string(Property::GameID, d->getGameID());

	// IOS version
	if ((d->discType & GameCubePrivate::DISC_SYSTEM_MASK) == GameCubePrivate::DISC_SYSTEM_WII) {
		d->metaData.addMetaData_string(Property::OSVersion, d->wii_getIOSVersion());
	}

	// Encryption key (game partition only)
	if (d->gamePartition) {
		const char *s_key_name = d->wii_getEncryptionKeyName(d->gamePartition);
		if (s_key_name) {
			d->metaData.addMetaData_string(Property::EncryptionKey, s_key_name);
		}
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData.count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCube::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(GameCube);
	if (imageType != IMG_INT_BANNER) {
		// Only IMG_INT_BANNER is supported by GameCube.
		pImage.reset();
		return -ENOENT;
	} else if (!d->isValid) {
		// Disc image isn't valid.
		pImage.reset();
		return -EIO;
	}

	// Internal images are currently only supported for GCN.
	if ((d->discType & GameCubePrivate::DISC_SYSTEM_MASK) != GameCubePrivate::DISC_SYSTEM_GCN) {
		// opening.bnr doesn't have an image.
		pImage.reset();
		return -ENOENT;
	}

	// Verify the disc format.
	switch (d->discType & GameCubePrivate::DISC_FORMAT_MASK) {
		default:
			break;

		case GameCubePrivate::DISC_FORMAT_WIA:
		case GameCubePrivate::DISC_FORMAT_RVZ:
			// WIA/RVZ isn't fully supported, so we can't load images.
			pImage.reset();
			return -ENOENT;
	}

	// Load opening.bnr. (GCN/Triforce only)
	// FIXME: Does Triforce have opening.bnr?
	if (d->loadOpeningBnr() != 0) {
		// Could not load opening.bnr.
		pImage.reset();
		return -ENOENT;
	}

	// Forward this call to the GameCubeBNR object.
	if (d->opening_bnr.romData) {
		return d->opening_bnr.romData->loadInternalImage(imageType, pImage);
	}

	// No GameCubeBNR object.
	pImage.reset();
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
 * @param imageType	[in]     Image type
 * @param extURLs	[out]    Output vector
 * @param size		[in,opt] Requested image size. This may be a requested
 *                               thumbnail size in pixels, or an ImageSizeType
 *                               enum value.
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCube::extURLs(ImageType imageType, vector<ExtURL> &extURLs, int size) const
{
	extURLs.clear();
	ASSERT_extURLs(imageType);

	RP_D(const GameCube);
	if (d->discType < 0) {
		// Unknown disc type.
		return -EIO;
	} else if ((d->discType & GameCubePrivate::DISC_FORMAT_MASK) == GameCubePrivate::DISC_FORMAT_TGC) {
		// TGC game IDs aren't unique, so we can't get
		// an image URL that makes any sense.
		return -ENOENT;
	} else if (!d->hasDiscHeader) {
		// No disc header. Cannot get the game ID.
		return -ENOENT;
	}

	// Disc header is read in the constructor.
	const GCN_DiscHeader *const discHeader = &d->discHeader;

	// Check for known unusable game IDs.
	// - RELSAB: Generic ID used for GCN prototypes and Wii update partitions.
	//   - Also sen as RELS01, so just check for RELSxx.
	// - RABAxx: Generic ID used for Wii prototypes and devkit updaters.
	// - _INSZZ: Channel partition.
	if (discHeader->id4[0] == '_' ||
	    discHeader->id4_32 == cpu_to_be32('RELS') ||
	    discHeader->id4_32 == cpu_to_be32('RABA'))
	{
		// Cannot download images for this game ID.
		return -ENOENT;
	}

	// Get the image sizes and sort them based on the
	// requested image size.
	const vector<ImageSizeDef> sizeDefs = supportedImageSizes(imageType);
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
	// Current image type
	const string imageTypeName = fmt::format(FSTR("{:s}{:s}"),
		imageTypeName_base, (sizeDef->name ? sizeDef->name : ""));

	// Determine the GameTDB region code(s).
	const vector<uint16_t> tdb_lc = GameCubeRegions::gcnRegionToGameTDB(d->gcnRegion, discHeader->id4[3]);

	// Game ID
	// Replace any non-printable characters with underscores.
	// (NDDEMO has ID6 "00\0E01".)
	char id6[7];
	for (unsigned int i = 0; i < 6; i++) {
		id6[i] = (ISPRINT(discHeader->id6[i]))
			? d->discHeader.id6[i]
			: '_';
	}
	id6[6] = 0;

	// External images with multiple discs must be handled differently.
	const bool isDisc2 =
		(imageType >= IMG_EXT_MIN && imageType <= IMG_EXT_MAX) &&
		 discHeader->disc_number > 0;

	// ExtURLs
	// TODO: If multiple image sizes are added, add the
	// "default" size to the end of ExtURLs in case the
	// user has high-resolution downloads disabled.
	// See Nintendo3DS for an example.
	// (NOTE: For GameTDB, currently only applies to coverfullHQ on GCN/Wii.)
	size_t vsz = tdb_lc.size();
	if (isDisc2) {
		// Need to increase the initial size.
		// Increasing the size later invalidates the iterator.
		vsz *= 2;
	}

	extURLs.resize(vsz);
	auto extURL_iter = extURLs.begin();

	// Is this not the first disc?
	if (isDisc2) {
		// Disc 2 (or 3, or 4...)
		// Request the disc 2 image first.
		const string discName = fmt::format(FSTR("{:s}{:d}"),
			imageTypeName, static_cast<unsigned int>(discHeader->disc_number) + 1);

		for (const uint16_t lc : tdb_lc) {
			const string lc_str = SystemRegion::lcToStringUpper(lc);
			ExtURL &extURL = *extURL_iter;
			extURL.url = d->getURL_GameTDB("wii", discName.c_str(), lc_str.c_str(), id6, ".png");
			extURL.cache_key = d->getCacheKey_GameTDB("wii", discName.c_str(), lc_str.c_str(), id6, ".png");
			extURL.width = sizeDef->width;
			extURL.height = sizeDef->height;
			++extURL_iter;
		}
	}

	// First disc, or not a disc scan.
	for (const uint16_t lc : tdb_lc) {
		const string lc_str = SystemRegion::lcToStringUpper(lc);
		ExtURL &extURL = *extURL_iter;
		extURL.url = d->getURL_GameTDB("wii", imageTypeName.c_str(), lc_str.c_str(), id6, ".png");
		extURL.cache_key = d->getCacheKey_GameTDB("wii", imageTypeName.c_str(), lc_str.c_str(), id6, ".png");
		extURL.width = sizeDef->width;
		extURL.height = sizeDef->height;
		extURL.high_res = false;	// Only one size is available.
		++extURL_iter;
	}

	// All URLs added.
	return 0;
}

/**
 * Check for "viewed" achievements.
 *
 * @return Number of achievements unlocked.
 */
int GameCube::checkViewedAchievements(void) const
{
	RP_D(const GameCube);
	if (!d->isValid || ((d->discType & GameCubePrivate::DISC_SYSTEM_MASK) != GameCubePrivate::DISC_SYSTEM_WII)) {
		// Disc is either not valid or is not Wii.
		return 0;
	}

	Achievements *const pAch = Achievements::instance();

	const int wiiPtLoaded = const_cast<GameCubePrivate*>(d)->loadWiiPartitionTables();
	if (wiiPtLoaded != 0) {
		// Unable to load Wii partition tables.
		return 0;
	}

	// Check for the main partition.
	const WiiPartition *pt = d->gamePartition;
	if (!pt) {
		pt = d->updatePartition;
	}
	if (!pt) {
		// No partitions...
		return 0;
	}

	int ret = 0;
	WiiTicket::EncryptionKeys encKey;
	if (d->isNASOSFormatDiscImage()) {
		// NASOS disc image.
		// If this would normally be an encrypted image, use encKeyReal().
		encKey = (d->discHeader.disc_noCrypto == 0)
			? pt->encKeyReal()
			: pt->encKey();
	} else {
		// Other disc image. Use encKey().
		encKey = pt->encKey();
	}

	switch (encKey) {
		default:
			break;
		case WiiTicket::EncryptionKeys::Key_RVT_Debug:
		case WiiTicket::EncryptionKeys::Key_RVT_Korean:
		case WiiTicket::EncryptionKeys::Key_CAT_Starbuck_vWii_Common:
			// Debug encryption.
			pAch->unlock(Achievements::ID::ViewedDebugCryptedFile);
			ret++;
			break;
	}

	return ret;
}

} // namespace LibRomData
