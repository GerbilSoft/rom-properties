/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiSave.cpp: Nintendo Wii save game file reader.                        *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WiiSave.hpp"
#include "gcn_card.h"
#include "wii_banner.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// Decryption
#include "librpbase/crypto/KeyManager.hpp"
#ifdef ENABLE_DECRYPTION
#  include "librpbase/disc/CBCReader.hpp"
// For sections delegated to other RomData subclasses.
#  include "librpbase/disc/PartitionFile.hpp"
#  include "WiiWIBN.hpp"
#endif /* ENABLE_DECRYPTION */

// WiiTicket for EncryptionKeys
// TODO: Use for title key decryption.
#include "../Console/WiiTicket.hpp"

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

class WiiSavePrivate final : public RomDataPrivate
{
public:
	WiiSavePrivate(const IRpFilePtr &file);
	~WiiSavePrivate() final;

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(WiiSavePrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// Save game structs
	Wii_SaveGame_Header_t svHeader;	// Only if encryption keys are available.
	Wii_Bk_Header_t bkHeader;

	bool svLoaded;	// True if svHeader was read.

	// Wii_Bk_Header_t magic
	static const uint8_t bk_header_magic[8];

	/**
	 * Round a value to the next highest multiple of 64.
	 * @param value Value.
	 * @return Next highest multiple of 64.
	 */
	template<typename T>
	static inline T toNext64(T val)
	{
		return (val + (T)63) & ~((T)63);
	}

#ifdef ENABLE_DECRYPTION
	// CBC reader for the main data area.
	CBCReaderPtr cbcReader;
	WiiWIBN *wibnData;

	// Key indexes (0 == AES, 1 == IV)
	std::array<WiiTicket::EncryptionKeys, 2> key_idx;
	// Key status
	std::array<KeyManager::VerifyResult, 2> key_status;
#endif /* ENABLE_DECRYPTION */
};

ROMDATA_IMPL(WiiSave)
ROMDATA_IMPL_IMG(WiiSave)

/** WiiSavePrivate **/

/* RomDataInfo */
const char *const WiiSavePrivate::exts[] = {
	".bin",
	// TODO: Custom extension?

	nullptr
};
const char *const WiiSavePrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-wii-save",

	nullptr
};
const RomDataInfo WiiSavePrivate::romDataInfo = {
	"WiiSave", exts, mimeTypes
};

// Wii_Bk_Header_t magic.
const uint8_t WiiSavePrivate::bk_header_magic[8] = {
	0x00, 0x00, 0x00, 0x70, 0x42, 0x6B, 0x00, 0x01
};

WiiSavePrivate::WiiSavePrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, svLoaded(false)
#ifdef ENABLE_DECRYPTION
	, wibnData(nullptr)
#endif /* ENABLE_DECRYPTION */
{
	// Clear the various structs.
	memset(&svHeader, 0, sizeof(svHeader));
	memset(&bkHeader, 0, sizeof(bkHeader));

#ifdef ENABLE_DECRYPTION
	key_idx.fill(WiiTicket::EncryptionKeys::Max);
	key_status.fill(KeyManager::VerifyResult::Unknown);
#endif /* ENABLE_DECRYPTION */
}

WiiSavePrivate::~WiiSavePrivate()
{
#ifdef ENABLE_DECRYPTION
	delete wibnData;
#endif /* ENABLE_DECRYPTION */
}

/**
 * Read a Nintendo Wii save game file.
 *
 * A WAD file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the WAD file.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open disc image.
 */
WiiSave::WiiSave(const IRpFilePtr &file)
	: super(new WiiSavePrivate(file))
{
	// This class handles save files.
	RP_D(WiiSave);
	d->mimeType = "application/x-wii-save";	// unofficial, not on fd.o
	d->fileType = FileType::SaveFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the save file header.
	// NOTE:
	// - Reading with save file header, banner, max number of icons, and the Bk header.
	// - Bk header is the only unencrypted header.
	// - Need to get encryption keys.
	static const unsigned int svSizeMin = (unsigned int)(
		sizeof(Wii_SaveGame_Header_t) +
		sizeof(Wii_WIBN_Header_t) +
		BANNER_WIBN_IMAGE_SIZE + BANNER_WIBN_ICON_SIZE +
		sizeof(Wii_Bk_Header_t));
	static const unsigned int svSizeTotal = (unsigned int)(
		svSizeMin + (BANNER_WIBN_ICON_SIZE * (CARD_MAXICONS-1)));
	auto svData = aligned_uptr<uint8_t>(16, svSizeTotal);

	d->file->rewind();
	size_t size = d->file->read(svData.get(), svSizeTotal);
	if (size < svSizeMin) {
		d->file.reset();
		return;
	} else if (size > svSizeTotal) {
		// NOTE: Shouldn't happen...
		assert(!"Read MORE data than requested!");
		size = svSizeTotal;
	}

	// Check for the Bk header at the designated locations.
	unsigned int bkHeaderAddr = (unsigned int)(
		svSizeMin - sizeof(Wii_Bk_Header_t));
	for (; bkHeaderAddr < size; bkHeaderAddr += BANNER_WIBN_ICON_SIZE) {
		const Wii_Bk_Header_t *bkHeader =
			reinterpret_cast<const Wii_Bk_Header_t*>(svData.get() + bkHeaderAddr);
		if (!memcmp(bkHeader->full_magic, d->bk_header_magic, sizeof(d->bk_header_magic))) {
			// Found the full magic.
			memcpy(&d->bkHeader, bkHeader, sizeof(d->bkHeader));
			break;
		}
	}

	if (d->bkHeader.magic != cpu_to_be16(WII_BK_MAGIC)) {
		// Bk header not found.
		d->isValid = false;
		d->file.reset();
		return;
	}

	// Found the Bk header.
	d->isValid = true;

#ifdef ENABLE_DECRYPTION
	// Get the decryption keys.
	// NOTE: Continuing even if the keys can't be loaded,
	// since we can still show the Bk header fields.

	// TODO: Debug vs. Retail?
	d->key_idx[0] = WiiTicket::EncryptionKeys::Key_RVL_SD_AES;
	d->key_idx[1] = WiiTicket::EncryptionKeys::Key_RVL_SD_IV;

	// Initialize the CBC reader for the main data area.
	// TODO: WiiVerifyKeys class.
	KeyManager *const keyManager = KeyManager::instance();
	assert(keyManager != nullptr);

	// Key verification data
	KeyManager::KeyData_t keyData[2];
	for (size_t i = 0; i < d->key_idx.size(); i++) {
		const char *const keyName = WiiTicket::encryptionKeyName_static(static_cast<int>(d->key_idx[i]));
		const uint8_t *const verifyData = WiiTicket::encryptionVerifyData_static(static_cast<int>(d->key_idx[i]));
		assert(keyName != nullptr);
		assert(keyName[0] != '\0');
		assert(verifyData != nullptr);

		// Get and verify the key.
		d->key_status[i] = keyManager->getAndVerify(keyName, &keyData[i], verifyData, 16);
	}

	if ((d->key_status[0] == KeyManager::VerifyResult::OK) &&
	    (d->key_status[1] == KeyManager::VerifyResult::OK))
	{
		// Create a CBC reader to decrypt the banner and icon.
		// TODO: Verify some known data?
		d->cbcReader = std::make_shared<CBCReader>(d->file, 0, bkHeaderAddr,
			keyData[0].key, keyData[1].key);

		// Read the save game header.
		// NOTE: Continuing even if this fails, since we can show
		// other information from the ticket and TMD.
		size = d->cbcReader->read(&d->svHeader, sizeof(d->svHeader));
		if (size == sizeof(d->svHeader)) {
			// Verify parts of the header.
			// - Title ID: must start with 0001xxxx
			// - Padding: must be 00 00
			// - TODO: MD5?
			if ((be32_to_cpu(d->svHeader.savegame_id.hi) >> 16) == 0x0001 &&
			    d->svHeader.unknown1 == 0 &&
			    d->svHeader.unknown2[0] == 0 &&
			    d->svHeader.unknown2[1] == 0)
			{
				// Save game header is valid.
				d->svLoaded = true;
			}
		}

		// Create the PartitionFile.
		// TODO: Only if the save game header is valid?
		// TODO: Get the size from the save game header?
		PartitionFilePtr ptFile = std::make_shared<PartitionFile>(d->cbcReader.get(),
			sizeof(Wii_SaveGame_Header_t),
			bkHeaderAddr - sizeof(Wii_SaveGame_Header_t));
		if (ptFile->isOpen()) {
			// Open the WiiWIBN.
			WiiWIBN *const wibn = new WiiWIBN(ptFile);
			if (wibn->isOpen()) {
				// Opened successfully.
				d->wibnData = wibn;
			} else {
				// Unable to open the WiiWIBN.
				delete wibn;
			}
		}
	}
#endif /* ENABLE_DECRYPTION */
}

/**
 * Close the opened file.
 */
void WiiSave::close(void)
{
#ifdef ENABLE_DECRYPTION
	RP_D(WiiSave);

	// Close any child RomData subclasses.
	if (d->wibnData) {
		d->wibnData->close();
	}

	// Close associated files used with child RomData subclasses.
	d->cbcReader.reset();
#endif /* ENABLE_DECRYPTION */

	// Call the superclass function.
	super::close();
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiSave::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	if (!info) {
		// Either no detection information was specified,
		// or the file extension is missing.
		return -1;
	}

	// TODO: Add support for encrypted channel backups?

	// Wii save files are encrypted. An unencrypted 'Bk' header
	// exists after the banner, but it might be past the data
	// read by RomDataFactory, so we ca'nt rely on it.
	// Therefore, we're using the file extension.
	if (info->ext && info->ext[0] != 0) {
		for (const char *const *ext = WiiSavePrivate::exts;
		     *ext != nullptr; ext++)
		{
			if (!strcasecmp(info->ext, *ext)) {
				// File extension is supported.
				return 0;
			}
		}
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *WiiSave::systemName(unsigned int type) const
{
	RP_D(const WiiSave);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Wii has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"WiiSave::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Nintendo Wii", "Wii", "Wii", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t WiiSave::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON | IMGBF_INT_BANNER;
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
vector<RomData::ImageSizeDef> WiiSave::supportedImageSizes_static(ImageType imageType)
{
#ifdef ENABLE_DECRYPTION
	// TODO: Check the actual WiiWIBN object?
	return WiiWIBN::supportedImageSizes_static(imageType);
#else /* !ENABLE_DECRYPTION */
	// TODO: Return the correct size information anyway?
	RP_UNUSED(imageType);
	return {};
#endif /* ENABLE_DECRYPTION */
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
uint32_t WiiSave::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

#ifdef ENABLE_DECRYPTION
	RP_D(const WiiSave);
	if (d->wibnData) {
		// Return imgpf from the WiiWIBN object.
		return d->wibnData->imgpf(imageType);
	}
#endif /* ENABLE_DECRYPTION */

	// No image processing flags by default.
	return 0;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int WiiSave::loadFieldData(void)
{
	RP_D(WiiSave);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// Wii save and backup headers.
	const Wii_SaveGame_Header_t *const svHeader = &d->svHeader;
	const Wii_Bk_Header_t *const bkHeader = &d->bkHeader;
	d->fields.reserve(5);	// Maximum of 5 fields.

	// Check if the headers are valid.
	// TODO: Do this in the constructor instead?
	const bool isSvValid = (svHeader->savegame_id.id != 0);
	const bool isBkValid = (!memcmp(bkHeader->full_magic, d->bk_header_magic, sizeof(d->bk_header_magic)));

	// Savegame header.
	if (isSvValid) {
		// Savegame ID. (title ID)
		d->fields.addField_string(C_("WiiSave", "Savegame ID"),
			rp_sprintf("%08X-%08X",
				be32_to_cpu(svHeader->savegame_id.hi),
				be32_to_cpu(svHeader->savegame_id.lo)));

	}

	// Game ID.
	// NOTE: Uses the ID from the Bk header.
	// TODO: Check if it matches the savegame header?
	if (isBkValid) {
		if (ISALNUM(bkHeader->id4[0]) &&
		    ISALNUM(bkHeader->id4[1]) &&
		    ISALNUM(bkHeader->id4[2]) &&
		    ISALNUM(bkHeader->id4[3]))
		{
			// Print the game ID.
			// TODO: Is the publisher code available anywhere?
			d->fields.addField_string(C_("RomData", "Game ID"),
				latin1_to_utf8(bkHeader->id4, sizeof(bkHeader->id4)));
		}
	}

	// Permissions.
	if (isSvValid) {
		// Unix-style permissions field.
		char s_perms[] = "----------";
		const uint8_t perms = svHeader->permissions;
		if (perms & 0x20)
			s_perms[1] = 'r';
		if (perms & 0x10)
			s_perms[2] = 'w';
		if (perms & 0x08)
			s_perms[4] = 'r';
		if (perms & 0x04)
			s_perms[5] = 'w';
		if (perms & 0x02)
			s_perms[7] = 'r';
		if (perms & 0x01)
			s_perms[8] = 'w';

		d->fields.addField_string(C_("WiiSave", "Permissions"), s_perms,
			RomFields::STRF_MONOSPACE);
	}

#ifdef ENABLE_DECRYPTION
	// NoCopy? (separate from permissions)
	if (d->wibnData) {
		// Flags bitfield.
		static const char *const flags_names[] = {
			NOP_C_("WiiSave|Flags", "No Copy from NAND"),
		};
		vector<string> *const v_flags_names = RomFields::strArrayToVector_i18n(
			"WiiSave|Flags", flags_names, ARRAY_SIZE(flags_names));
		const uint32_t flags = (d->wibnData->isNoCopyFlagSet() ? 1 : 0);
		d->fields.addField_bitfield(C_("WiiSave", "Flags"),
			v_flags_names, 3, flags);
	}
#endif /* ENABLE_DECRYPTION */

	// MAC address.
	if (isBkValid) {
		d->fields.addField_string(C_("WiiSave", "MAC Address"),
			rp_sprintf("%02X:%02X:%02X:%02X:%02X:%02X",
				bkHeader->wii_mac[0], bkHeader->wii_mac[1],
				bkHeader->wii_mac[2], bkHeader->wii_mac[3],
				bkHeader->wii_mac[4], bkHeader->wii_mac[5]));
	}

	// TODO: Get title information from the encrypted data.
	// (Is there an IMET header?)

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiSave::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

#ifdef ENABLE_DECRYPTION
	// Forward this call to the WiiWIBN object.
	RP_D(WiiSave);
	if (d->wibnData) {
		return d->wibnData->loadInternalImage(imageType, pImage);
	}
#endif /* ENABLE_DECRYPTION */

	// No WiiWIBN object.
	pImage.reset();
	return -ENOENT;
}

/**
 * Get the animated icon data.
 *
 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
 * object has an animated icon.
 *
 * @return Animated icon data, or nullptr if no animated icon is present.
 */
IconAnimDataConstPtr WiiSave::iconAnimData(void) const
{
#ifdef ENABLE_DECRYPTION
	// Forward this call to the WiiWIBN object.
	RP_D(const WiiSave);
	if (d->wibnData) {
		return d->wibnData->iconAnimData();
	}
#endif /* ENABLE_DECRYPTION */

	// No WiiWIBN object.
	return nullptr;
}

}
