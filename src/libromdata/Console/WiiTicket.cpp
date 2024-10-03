/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiTicket.cpp: Nintendo Wii (and Wii U) ticket reader.                  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WiiTicket.hpp"
#include "wii_structs.h"

// librpbase
#ifdef ENABLE_DECRYPTION
#  include "librpbase/crypto/IAesCipher.hpp"
#  include "librpbase/crypto/AesCipherFactory.hpp"
#endif /* ENABLE_DECRYPTION */
using namespace LibRpBase;

// Other rom-properties libraries
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::unique_ptr;

namespace LibRomData {

class WiiTicketPrivate final : public RomDataPrivate
{
public:
	explicit WiiTicketPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(WiiTicketPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// Ticket (v0 and v1)
	RVL_Ticket_V1 ticket;

	// Encryption key verification result
	KeyManager::VerifyResult verifyResult;

	// Encryption key in use
	WiiTicket::EncryptionKeys encKey;

public:
	/**
	 * Determine which encryption key is in use.
	 * Result is stored in this->encKey.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int getEncKey(void);

#ifdef ENABLE_DECRYPTION
public:
	// Verification key names
	static const array<const char*, static_cast<size_t>(WiiTicket::EncryptionKeys::Max)> EncryptionKeyNames;

	// Verification key data
	static const uint8_t EncryptionKeyVerifyData[static_cast<size_t>(WiiTicket::EncryptionKeys::Max)][16];
#endif /* ENABLE_DECRYPTION */
};

ROMDATA_IMPL(WiiTicket)

/** WiiTicketPrivate **/

/* RomDataInfo */
const char *const WiiTicketPrivate::exts[] = {
	".tik",

	nullptr
};
const char *const WiiTicketPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-nintendo-ticket",

	nullptr
};
const RomDataInfo WiiTicketPrivate::romDataInfo = {
	"WiiTicket", exts, mimeTypes
};

#ifdef ENABLE_DECRYPTION

// Verification key names.
const std::array<const char*, static_cast<size_t>(WiiTicket::EncryptionKeys::Max)> WiiTicketPrivate::EncryptionKeyNames = {{
	// Retail
	"rvl-common",
	"rvl-korean",
	"wup-starbuck-vwii-common",

	// Debug
	"rvt-debug",
	"rvt-korean",
	"cat-starbuck-vwii-common",

	// SD card (TODO: Retail vs. Debug?)
	"rvl-sd-aes",
	"rvl-sd-iv",
	"rvl-sd-md5",

	// Wii U mode keys
	"wup-starbuck-wiiu-common",
	"cat-starbuck-wiiu-common",
}};

const uint8_t WiiTicketPrivate::EncryptionKeyVerifyData[static_cast<size_t>(WiiTicket::EncryptionKeys::Max)][16] = {
	/** Retail **/

	// rvl-common
	{0xCF,0xB7,0xFF,0xA0,0x64,0x0C,0x7A,0x7D,
	 0xA7,0x22,0xDC,0x16,0x40,0xFA,0x04,0x58},
	// rvl-korean
	{0x98,0x1C,0xD4,0x51,0x17,0xF2,0x23,0xB6,
	 0xC8,0x84,0x4A,0x97,0xA6,0x93,0xF2,0xE3},
	// wup-starbuck-vwii-common
	{0x04,0xF1,0x33,0x3F,0xF8,0x05,0x7B,0x8F,
	 0xA7,0xF1,0xED,0x6E,0xAC,0x23,0x33,0xFA},

	/** Debug **/

	// rvt-debug
	{0x22,0xC4,0x2C,0x5B,0xCB,0xFE,0x75,0xAC,
	 0xEB,0xC3,0x6B,0xAF,0x90,0xB3,0xB4,0xF5},
	// rvt-korean
	{0x31,0x81,0xF2,0xCA,0xFE,0x70,0x58,0xCB,
	 0x3C,0x0F,0xB9,0x9D,0x2D,0x45,0x74,0xDA},
	// cat-starbuck-vwii-common
	{0x0B,0xFB,0x83,0x83,0x38,0xCB,0x1A,0x83,
	 0x5E,0x1C,0xEC,0xCA,0xDC,0x5D,0xF1,0xFA},

	/** SD card (TODO: Retail vs. Debug?) **/

	// rvl-sd-aes
	{0x8C,0x1C,0xBA,0x01,0x02,0xB9,0x6F,0x65,
	 0x24,0x7C,0x85,0x3C,0x0F,0x3B,0x8C,0x37},
	// rvl-sd-iv
	{0xE3,0xEE,0xE5,0x0F,0xDC,0xFD,0xBE,0x89,
	 0x20,0x05,0xF2,0xB9,0xD8,0x1D,0xF1,0x27},
	// rvl-sd-md5
	{0xF8,0xE1,0x8D,0x89,0x06,0xC7,0x21,0x32,
	 0x9D,0xE0,0x14,0x19,0x30,0xC3,0x88,0x1F},

	/** Wii U mode keys **/

	// wup-starbuck-wiiu-common
	{0x05,0xBA,0x63,0x98,0x8A,0x50,0x90,0x4D,
	 0xEC,0x93,0xAC,0xF3,0x07,0x8F,0x3E,0x90},

	// cat-starbuck-wiiu-common
	{0xF3,0xE2,0xED,0xF4,0x8D,0x99,0x2A,0x5B,
	 0xD8,0xE1,0x3F,0xA2,0x9B,0x89,0x73,0xAA},
};
#endif /* ENABLE_DECRYPTION */

WiiTicketPrivate::WiiTicketPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
#ifdef ENABLE_DECRYPTION
	, verifyResult(KeyManager::VerifyResult::Unknown)
#else /* !ENABLE_DECRYPTION */
	, verifyResult(KeyManager::VerifyResult::NoSupport)
#endif /* ENABLE_DECRYPTION */
	, encKey(WiiTicket::EncryptionKeys::Unknown)
{
	// Clear the ticket struct.
	memset(&ticket, 0, sizeof(ticket));
}

/**
 * Determine which encryption key is in use.
 * Result is stored in this->encKey.
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiTicketPrivate::getEncKey(void)
{
	if (encKey != WiiTicket::EncryptionKeys::Unknown) {
		// Key was already determined.
		return 0;
	}

	// Determine the key in use by checking the issuer.
	// TODO: WiiTicket probably isn't the best place for Wii U keys...
	char issuer[64];
	memcpy(issuer, ticket.v0.signature_issuer, sizeof(ticket.v0.signature_issuer));
	issuer[sizeof(issuer)-1] = '\0';

	uint32_t ca, xs;
	char c;
	int ret = sscanf(issuer, "Root-CA%08X-XS%08X%c", &ca, &xs, &c);
	if (ret != 2) {
		// Not a valid issuer...
		return -EINVAL;
	}

	uint8_t common_key_index = ticket.v0.common_key_index;
	if (common_key_index > 2) {
		// Out of range. Assume Wii common key.
		common_key_index = 0;
	}

	// Check CA and XS.
	WiiTicket::EncryptionKeys encKey;
	if (ca == 1 && xs == 3) {
		// RVL retail
		encKey = static_cast<WiiTicket::EncryptionKeys>(
			(int)WiiTicket::EncryptionKeys::Key_RVL_Common + common_key_index);
	} else if (ca == 2 && xs == 6) {
		// RVT debug (TODO: There's also XS00000004)
		encKey = static_cast<WiiTicket::EncryptionKeys>(
			(int)WiiTicket::EncryptionKeys::Key_RVT_Debug + common_key_index);
	} else if (ca == 3 && xs == 0xc) {
		// CTR/WUP retail
		encKey = WiiTicket::EncryptionKeys::Key_WUP_Starbuck_WiiU_Common;
	} else if (ca == 4 && xs == 0xf) {
		// CAT debug
		encKey = WiiTicket::EncryptionKeys::Key_CAT_Starbuck_WiiU_Common;
	} else if (ca == 4 && xs == 0x9) {
		// CAT debug (early titles; same as CTR debug)
		encKey = WiiTicket::EncryptionKeys::Key_CAT_Starbuck_WiiU_Common;
	} else {
		// Unsupported CA/XS combination.
		return -EINVAL;
	}

	this->encKey = encKey;
	return 0;
}

/** WiiTicket **/

/**
 * Read a Nintendo Wii (or Wii U) ticket file. (.tik)
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
WiiTicket::WiiTicket(const IRpFilePtr &file)
	: super(new WiiTicketPrivate(file))
{
	RP_D(WiiTicket);
	d->mimeType = WiiTicketPrivate::mimeTypes[0];	// unofficial
	d->fileType = FileType::Ticket;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ticket. (either v0 or v1, depending on how much was read)
	d->file->rewind();
	size_t size = d->file->read(&d->ticket, sizeof(d->ticket));
	if (size < sizeof(RVL_Ticket)) {
		// Ticket is too small.
		d->file.reset();
		return;
	}

	// Check if this ticket is supported.
	const char *const filename = file->filename();
	const DetectInfo info = {
		{0, sizeof(d->ticket), reinterpret_cast<const uint8_t*>(&d->ticket)},
		FileSystem::file_ext(filename),	// ext
		d->file->size()			// szFile
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiTicket::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->ext || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(RVL_Ticket))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// NOTE: File extension must match.
	bool ok = false;
	for (const char *const *ext = WiiTicketPrivate::exts;
	     *ext != nullptr; ext++)
	{
		if (!strcasecmp(info->ext, *ext)) {
			// File extension is supported.
			ok = true;
			break;
		}
	}
	if (!ok) {
		// File extension doesn't match.
		return -1;
	}

	// Compare the ticket version to the file size.
	const RVL_Ticket *const ticket = reinterpret_cast<const RVL_Ticket*>(info->header.pData);
	switch (ticket->ticket_format_version) {
		default:
			// Unsupported ticket version.
			return -1;
		case 0:
			if (info->szFile != sizeof(RVL_Ticket)) {
				// Incorrect file size.
				return -1;
			}
			break;
		case 1:
			if (info->szFile < static_cast<off64_t>(sizeof(RVL_Ticket_V1))) {
				// Incorrect file size.

				// NOTE: Updates may have larger tickets.
				// NES REMIX (USA) (Update) has a 2640-byte ticket.
				// It seems to have a certificate chain appended?
				// We'll allow any ticket >= 848 bytes for now.

				// NOTE 2: Wii U boot1 has a 696-byte v1 ticket.
				// (20 bytes larger than v0 tickets.)
				if (ticket->title_id.hi == cpu_to_be32(0x00050010) &&
				    ticket->title_id.lo == cpu_to_be32(0x10000100))
				{
					// This is Wii U boot1.
					if (info->szFile == 676+20) {
						// Size matches v0 + 20 bytes.
						break;
					}
				}

				// Still not valid.
				return -1;
			}
			break;
	}

	// Validate the ticket signature format.
	switch (be32_to_cpu(ticket->signature_type)) {
		default:
			// Unsupported signature format.
			return -1;
		case RVL_CERT_SIGTYPE_RSA2048_SHA1:
			// RSA-2048 with SHA-1 (Wii, DSi)
			break;
		case WUP_CERT_SIGTYPE_RSA2048_SHA256:
		case WUP_CERT_SIGTYPE_RSA2048_SHA256 | WUP_CERT_SIGTYPE_FLAG_DISC:
			// RSA-2048 with SHA-256 (Wii U, 3DS)
			// NOTE: Requires ticket format v1 or later.
			if (ticket->ticket_format_version < 1)
				return -1;
			break;
	}

	// Certificate issuer must start with "Root-".
	if (memcmp(ticket->signature_issuer, "Root-", 5) != 0) {
		// Incorrect issuer.
		return -1;
	}

	// This appears to be a valid Nintendo ticket.
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *WiiTicket::systemName(unsigned int type) const
{
	RP_D(const WiiTicket);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GBA has the same name worldwide, so we can
	// ignore the region selection.
	// TODO: Abbreviation might be different... (Japan uses AGB?)
	static_assert(SYSNAME_TYPE_MASK == 3,
		"WiiTicket::systemName() array index optimization needs to be updated.");

	// Use the title ID to determine the system.
	static const array<array<const char*, 4>, 8> sysNames = {{
		{{"Nintendo Wii", "Wii", "Wii", nullptr}},		// Wii IOS
		{{"Nintendo Wii", "Wii", "Wii", nullptr}},		// Wii
		{{"GBA NetCard", "NetCard", "NetCard", nullptr}},	// GBA NetCard
		{{"Nintendo DSi", "DSi", "DSi", nullptr}},		// DSi
		{{"Nintendo 3DS", "3DS", "3DS", nullptr}},		// 3DS
		{{"Nintendo Wii U", "Wii U", "Wii U", nullptr}},	// Wii U
		{{nullptr, nullptr, nullptr, nullptr}},			// unused
		{{"Nintendo Wii U", "Wii U", "Wii U", nullptr}},	// Wii U (vWii)
	}};

	const unsigned int sysID = be16_to_cpu(d->ticket.v0.title_id.sysID);
	return (likely(sysID < ARRAY_SIZE(sysNames)))
		? sysNames[sysID][type & SYSNAME_TYPE_MASK]
		: nullptr;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int WiiTicket::loadFieldData(void)
{
	RP_D(WiiTicket);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Ticket isn't valid.
		return -EIO;
	}

	// Ticket is read in the constructor.
	const RVL_Ticket *const ticket = &d->ticket.v0;
	d->fields.reserve(5);	// Maximum of 4 fields.

	// Title ID
	char s_title_id[24];
	snprintf(s_title_id, sizeof(s_title_id), "%08X-%08X",
		be32_to_cpu(ticket->title_id.hi),
		be32_to_cpu(ticket->title_id.lo));
	d->fields.addField_string(C_("Nintendo", "Title ID"), s_title_id, RomFields::STRF_MONOSPACE);

	// Issuer
	d->fields.addField_string(C_("Nintendo", "Issuer"),
		latin1_to_utf8(ticket->signature_issuer, sizeof(ticket->signature_issuer)),
		RomFields::STRF_MONOSPACE | RomFields::STRF_TRIM_END);

	// Console ID
	d->fields.addField_string_numeric(C_("Nintendo", "Console ID"),
		be32_to_cpu(ticket->console_id), RomFields::Base::Hex, 8,
		RomFields::STRF_MONOSPACE);

	// Key index
	d->fields.addField_string_numeric(C_("Nintendo", "Key Index"), ticket->common_key_index);

	// Encryption key in use
	// NOTE: Indicating "(Wii U)" for Wii U-specific keys.
	// TODO: Consolidate with GameCube::loadFieldData()'s "Wii|EncKey"?
	const char *const s_encKey_title = C_("RomData", "Encryption Key");
	const char *const s_key_name = encKeyName();
	if (s_key_name) {
		d->fields.addField_string(s_encKey_title, s_key_name);
	} else {
		// Unable to get the encryption key?
		d->fields.addField_string(C_("RomData", "Warning"),
			C_("RomData", "Could not determine the required encryption key."),
			RomFields::STRF_WARNING);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int WiiTicket::loadMetaData(void)
{
	RP_D(WiiTicket);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Ticket isn't valid.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// Ticket is read in the constructor.
	const RVL_Ticket *const ticket = &d->ticket.v0;

	// Title ID (using as Title)
	char s_title_id[24];
	snprintf(s_title_id, sizeof(s_title_id), "%08X-%08X",
		be32_to_cpu(ticket->title_id.hi),
		be32_to_cpu(ticket->title_id.lo));
	d->metaData->addMetaData_string(Property::Title, s_title_id);

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/** Ticket accessors **/

/**
 * Get the ticket format version.
 * @return Ticket format version
 */
unsigned int WiiTicket::ticketFormatVersion(void) const
{
	RP_D(const WiiTicket);
	return (likely(d->isValid)) ? d->ticket.v0.ticket_format_version : 0;
}

/**
 * Get the ticket. (v0)
 *
 * NOTE: The v1 ticket doesn't have any useful extra data,
 * so we're only offering the v0 ticket.
 *
 * @return Ticket
 */
const RVL_Ticket *WiiTicket::ticket_v0(void) const
{
	RP_D(const WiiTicket);
	return (likely(d->isValid)) ? &d->ticket.v0 : nullptr;
}

#ifdef ENABLE_DECRYPTION
/**
 * Get the decrypted title key.
 * The title ID is used as the IV.
 *
 * @param pKeyBuf	[out] Pointer to key buffer
 * @param size		[in] Size of pKeyBuf (must be 16)
 * @return 0 on success; negative POSIX error code on error. (Check verifyResult() for key verification errors.)
 */
ATTR_ACCESS_SIZE(write_only, 2, 3)
int WiiTicket::decryptTitleKey(uint8_t *pKeyBuf, size_t size)
{
	RP_D(WiiTicket);
	assert(d->isValid);
	assert(size >= 16);
	if (!d->isValid) {
		// Not valid...
		return -EIO;
	} else if (size != 16) {
		// Key buffer size is incorrect.
		return -EINVAL;
	}

	// Determine the encryption key in use.
	int ret = d->getEncKey();
	if (ret != 0) {
		// Error getting the encryption key.
		return ret;
	}

	// Get the Key Manager instance.
	KeyManager *const keyManager = KeyManager::instance();
	assert(keyManager != nullptr);

	// Initialize the AES cipher.
	unique_ptr<IAesCipher> cipher(AesCipherFactory::create());
	if (!cipher || !cipher->isInit()) {
		// Error initializing the cipher.
		d->verifyResult = KeyManager::VerifyResult::IAesCipherInitErr;
		return -EIO;
	}

	// Get the common key.
	KeyManager::KeyData_t keyData;
	KeyManager::VerifyResult verifyResult = keyManager->getAndVerify(
		WiiTicket::encryptionKeyName_static(static_cast<int>(d->encKey)), &keyData,
		WiiTicket::encryptionVerifyData_static(static_cast<int>(d->encKey)), 16);
	if (verifyResult != KeyManager::VerifyResult::OK) {
		// An error occurred while loading the common key.
		d->verifyResult = verifyResult;
		return -EINVAL;
	}

	// Load the common key into the AES cipher. (CBC mode)
	ret = cipher->setKey(keyData.key, keyData.length);
	ret |= cipher->setChainingMode(IAesCipher::ChainingMode::CBC);
	if (ret != 0) {
		// Error initializing the cipher.
		// TODO: Return verifyResult?
		d->verifyResult = KeyManager::VerifyResult::IAesCipherInitErr;
		return -EIO;
	}

	// Get the IV.
	// First 8 bytes are the title ID.
	// Second 8 bytes are all 0.
	uint8_t iv[16];
	memcpy(iv, d->ticket.v0.title_id.u8, 8);
	memset(&iv[8], 0, 8);

	// Decrypt the title key.
	memcpy(pKeyBuf, d->ticket.v0.enc_title_key, sizeof(d->ticket.v0.enc_title_key));
	if (cipher->decrypt(pKeyBuf, sizeof(d->ticket.v0.enc_title_key), iv, sizeof(iv)) != sizeof(d->ticket.v0.enc_title_key)) {
		// Error decrypting the title key.
		// TODO: Return verifyResult?
		d->verifyResult = KeyManager::VerifyResult::IAesCipherDecryptErr;
		return -EIO;
	}

	// Title key decrypted.
	d->verifyResult = KeyManager::VerifyResult::OK;
	return 0;
}
#endif /* ENABLE_DECRYPTION */

/**
 * Encryption key verification result.
 * Call this function after calling decryptTitleKey().
 * @return Encryption key verification result.
 */
KeyManager::VerifyResult WiiTicket::verifyResult(void) const
{
	RP_D(const WiiTicket);
	return d->verifyResult;
}

/**
 * Encryption key in use.
 * Call this function after calling decryptTitleKey().
 * @return Encryption key in use.
 */
WiiTicket::EncryptionKeys WiiTicket::encKey(void) const
{
	RP_D(const WiiTicket);
	if (d->encKey == WiiTicket::EncryptionKeys::Unknown) {
		// Try to determine the encryption key.
		const_cast<WiiTicketPrivate*>(d)->getEncKey();
	}
	return d->encKey;
}

/**
 * Get a user-friendly name for the specified encryption key.
 * NOTE: EncryptionKeys::Unknown will return nullptr.
 *
 * @param encKey Encryption key index
 * @return User-friendly name, or nullptr if not found.
 */
const char *WiiTicket::encKeyName_static(EncryptionKeys encKey)
{
	static const std::array<const char*, (int)EncryptionKeys::Max> wii_key_tbl = {{
		// tr: Key_RVL_Common - Retail Wii encryption key
		NOP_C_("Wii|EncKey", "Retail"),
		// tr: Key_RVL_Korean - Korean Wii encryption key
		NOP_C_("Wii|EncKey", "Korean"),
		// tr: Key_WUP_vWii - vWii-specific Wii encryption key
		NOP_C_("Wii|EncKey", "vWii"),

		// tr: Key_RVT_Debug - Debug Wii encryption key
		NOP_C_("Wii|EncKey", "Debug"),
		// tr: Key_RVT_Korean - Korean (debug) Wii encryption key
		NOP_C_("Wii|EncKey", "Korean (debug)"),
		// tr: Key_CAT_vWii - vWii (debug) Wii encryption key
		NOP_C_("Wii|EncKey", "vWii (debug)"),

		// SD card encryption keys (unlikely!)

		// tr: Key_RVL_SD_AES - SD card encryption key
		NOP_C_("Wii|EncKey", "SD card AES"),
		// tr: Key_RVL_SD_IV - SD card IV
		NOP_C_("Wii|EncKey", "SD card IV"),
		// tr: Key_RVL_SD_MD5 - SD card MD5 blanker
		NOP_C_("Wii|EncKey", "SD card MD5 blanker"),

		// Wii U mode keys

		// tr: Key_WUP_Starbuck_WiiU_Common - Retail Wii U encryption key
		NOP_C_("Wii|EncKey", "Retail (Wii U)"),
		// tr: Key_CAT_Starbuck_WiiU_Common - Debug Wii U encryption key
		NOP_C_("Wii|EncKey", "Debug (Wii U)"),
	}};

	const char *s_key_name;
	if ((int)encKey >= 0 && encKey < EncryptionKeys::Max) {
		s_key_name = pgettext_expr("Wii|KeyIdx", wii_key_tbl[(int)encKey]);
	} else if (encKey == WiiTicket::EncryptionKeys::None) {
		// tr: EncryptionKeys::None - No encryption.
		s_key_name = C_("Wii|EncKey", "None");
	} else {
		// Returning nullptr for Unknown.
		// The caller will have to handle this.
		s_key_name = nullptr;
	}

	return s_key_name;
}

/**
 * Get a user-friendly name for this ticket's encryption key.
 * NOTE: EncryptionKeys::Unknown will return nullptr.
 *
 * @return User-friendly name, or nullptr if not found.
 */
const char *WiiTicket::encKeyName(void) const
{
	RP_D(const WiiTicket);
	if (d->encKey == WiiTicket::EncryptionKeys::Unknown) {
		// Try to determine the encryption key.
		const_cast<WiiTicketPrivate*>(d)->getEncKey();
	}
	return encKeyName_static(d->encKey);
}

#ifdef ENABLE_DECRYPTION
/** Encryption keys. **/

/**
 * Get the total number of encryption key names.
 * @return Number of encryption key names.
 */
int WiiTicket::encryptionKeyCount_static(void)
{
	return static_cast<int>(EncryptionKeys::Max);
}

/**
 * Get an encryption key name.
 * @param keyIdx Encryption key index.
 * @return Encryption key name (in ASCII), or nullptr on error.
 */
const char *WiiTicket::encryptionKeyName_static(int keyIdx)
{
	assert(keyIdx >= 0);
	assert(keyIdx < static_cast<int>(EncryptionKeys::Max));
	if (keyIdx < 0 || keyIdx >= static_cast<int>(EncryptionKeys::Max))
		return nullptr;
	return WiiTicketPrivate::EncryptionKeyNames[keyIdx];
}

/**
 * Get the verification data for a given encryption key index.
 * @param keyIdx Encryption key index.
 * @return Verification data. (16 bytes)
 */
const uint8_t *WiiTicket::encryptionVerifyData_static(int keyIdx)
{
	assert(keyIdx >= 0);
	assert(keyIdx < static_cast<int>(EncryptionKeys::Max));
	if (keyIdx < 0 || keyIdx >= static_cast<int>(EncryptionKeys::Max))
		return nullptr;
	return WiiTicketPrivate::EncryptionKeyVerifyData[keyIdx];
}
#endif /* ENABLE_DECRYPTION */

} // namespace LibRomData
