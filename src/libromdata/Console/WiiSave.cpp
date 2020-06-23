/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiSave.cpp: Nintendo Wii save game file reader.                        *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WiiSave.hpp"
#include "gcn_card.h"
#include "wii_banner.h"

// librpbase, librpfile, librptexture
using namespace LibRpBase;
using LibRpFile::IRpFile;
using LibRpTexture::rp_image;

// Decryption.
#include "librpbase/crypto/KeyManager.hpp"
#include "disc/WiiPartition.hpp"	// for key information
#ifdef ENABLE_DECRYPTION
# include "librpbase/disc/CBCReader.hpp"
// For sections delegated to other RomData subclasses.
# include "librpbase/disc/PartitionFile.hpp"
# include "WiiWIBN.hpp"
#endif /* ENABLE_DECRYPTION */

// C++ STL classes.
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(WiiSave)
ROMDATA_IMPL_IMG(WiiSave)

class WiiSavePrivate : public RomDataPrivate
{
	public:
		WiiSavePrivate(WiiSave *q, IRpFile *file);
		~WiiSavePrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(WiiSavePrivate)

	public:
		// Save game structs.
		Wii_SaveGame_Header_t svHeader;	// Only if encryption keys are available.
		Wii_Bk_Header_t bkHeader;

		bool svLoaded;	// True if svHeader was read.

		// Wii_Bk_Header_t magic.
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
		CBCReader *cbcReader;
		WiiWIBN *wibnData;
#endif /* ENABLE_DECRYPTION */
		// Key indexes. (0 == AES, 1 == IV)
		std::array<WiiPartition::EncryptionKeys, 2> key_idx;
		// Key status.
		std::array<KeyManager::VerifyResult, 2> key_status;
};

/** WiiSavePrivate **/

// Wii_Bk_Header_t magic.
const uint8_t WiiSavePrivate::bk_header_magic[8] = {
	0x00, 0x00, 0x00, 0x70, 0x42, 0x6B, 0x00, 0x01
};

WiiSavePrivate::WiiSavePrivate(WiiSave *q, IRpFile *file)
	: super(q, file)
	, svLoaded(false)
#ifdef ENABLE_DECRYPTION
	, cbcReader(nullptr)
	, wibnData(nullptr)
#endif /* ENABLE_DECRYPTION */
{
	// Clear the various structs.
	memset(&svHeader, 0, sizeof(svHeader));
	memset(&bkHeader, 0, sizeof(bkHeader));

	key_idx.fill(WiiPartition::Key_Max);
	key_status.fill(KeyManager::VerifyResult::Unknown);
}

WiiSavePrivate::~WiiSavePrivate()
{
#ifdef ENABLE_DECRYPTION
	UNREF(wibnData);
	UNREF(cbcReader);
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
WiiSave::WiiSave(IRpFile *file)
	: super(new WiiSavePrivate(this, file))
{
	// This class handles application packages.
	RP_D(WiiSave);
	d->className = "WiiSave";
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
		UNREF_AND_NULL_NOCHK(d->file);
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
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Found the Bk header.
	d->isValid = true;

	// Get the decryption keys.
	// NOTE: Continuing even if the keys can't be loaded,
	// since we can still show the Bk header fields.

	// TODO: Debug vs. Retail?
	d->key_idx[0] = WiiPartition::Key_Rvl_SD_AES;
	d->key_idx[1] = WiiPartition::Key_Rvl_SD_IV;

#ifdef ENABLE_DECRYPTION
	// Initialize the CBC reader for the main data area.

	// TODO: WiiVerifyKeys class.
	KeyManager *const keyManager = KeyManager::instance();
	assert(keyManager != nullptr);

	// Key verification data.
	// TODO: Move out of WiiPartition and into WiiVerifyKeys?
	KeyManager::KeyData_t keyData[2];
	for (size_t i = 0; i < d->key_idx.size(); i++) {
		const char *const keyName = WiiPartition::encryptionKeyName_static(d->key_idx[i]);
		const uint8_t *const verifyData = WiiPartition::encryptionVerifyData_static(d->key_idx[i]);
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
		d->cbcReader = new CBCReader(d->file, 0, bkHeaderAddr,
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
		PartitionFile *const ptFile = new PartitionFile(d->cbcReader,
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
				wibn->unref();
			}
		}
		ptFile->unref();
	}
#else /* !ENABLE_DECRYPTION */
	// Cannot decrypt anything...
	d->key_status[0] = KeyManager::VerifyResult::NoSupport;
	d->key_status[1] = KeyManager::VerifyResult::NoSupport;
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
	UNREF_AND_NULL(d->cbcReader);
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
		const char *const *exts = supportedFileExtensions_static();
		if (!exts) {
			// Should not happen...
			return -1;
		}
		for (; *exts != nullptr; exts++) {
			if (!strcasecmp(info->ext, *exts)) {
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
const char *const *WiiSave::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".bin",
		// TODO: Custom extension?

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
const char *const *WiiSave::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"application/x-wii-save",

		nullptr
	};
	return mimeTypes;
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
	return vector<RomData::ImageSizeDef>();
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
	if (!d->fields->empty()) {
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
	d->fields->reserve(4);	// Maximum of 4 fields.

	// Check if the headers are valid.
	// TODO: Do this in the constructor instead?
	const bool isSvValid = (svHeader->savegame_id.id != 0);
	const bool isBkValid = (!memcmp(bkHeader->full_magic, d->bk_header_magic, sizeof(d->bk_header_magic)));

	// Savegame header.
	if (isSvValid) {
		// Savegame ID. (title ID)
		d->fields->addField_string(C_("WiiSave", "Savegame ID"),
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
			d->fields->addField_string(C_("WiiSave", "Game ID"),
				latin1_to_utf8(bkHeader->id4, sizeof(bkHeader->id4)));
		}
	}

	// Permissions. (TODO)
	if (isSvValid) {
		d->fields->addField_string_numeric(C_("WiiSave", "Permissions"),
			svHeader->permissions,
			RomFields::Base::Hex, 2, RomFields::STRF_MONOSPACE);
	}

	// MAC address.
	if (isBkValid) {
		d->fields->addField_string(C_("WiiSave", "MAC Address"),
			rp_sprintf("%02X:%02X:%02X:%02X:%02X:%02X",
				bkHeader->wii_mac[0], bkHeader->wii_mac[1],
				bkHeader->wii_mac[2], bkHeader->wii_mac[3],
				bkHeader->wii_mac[4], bkHeader->wii_mac[5]));
	}

	// TODO: Get title information from the encrypted data.
	// (Is there an IMET header?)

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Pointer to const rp_image* to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiSave::loadInternalImage(ImageType imageType, const rp_image **pImage)
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
	*pImage = nullptr;
	return 0;
}

/**
 * Get the animated icon data.
 *
 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
 * object has an animated icon.
 *
 * @return Animated icon data, or nullptr if no animated icon is present.
 */
const IconAnimData *WiiSave::iconAnimData(void) const
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
