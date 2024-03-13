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
#include "librpbase/crypto/KeyManager.hpp"
#ifdef ENABLE_DECRYPTION
#  include "librpbase/crypto/IAesCipher.hpp"
#  include "librpbase/crypto/AesCipherFactory.hpp"

// for encryption key indexes
#  include "libromdata/disc/WiiPartition.hpp"
#endif /* ENABLE_DECRYPTION */
using namespace LibRpBase;

// Other rom-properties libraries
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::unique_ptr;

namespace LibRomData {

class WiiTicketPrivate final : public RomDataPrivate
{
public:
	WiiTicketPrivate(const IRpFilePtr &file);

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

WiiTicketPrivate::WiiTicketPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the ticket struct.
	memset(&ticket, 0, sizeof(ticket));
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
	static const char *const sysNames[8][4] = {
		{"Nintendo Wii", "Wii", "Wii", nullptr},	// Wii IOS
		{"Nintendo Wii", "Wii", "Wii", nullptr},	// Wii
		{"GBA NetCard", "NetCard", "NetCard", nullptr},	// GBA NetCard
		{"Nintendo DSi", "DSi", "DSi", nullptr},	// DSi
		{"Nintendo 3DS", "3DS", "3DS", nullptr},	// 3DS
		{"Nintendo Wii U", "Wii U", "Wii U", nullptr},	// Wii U
		{nullptr, nullptr, nullptr, nullptr},		// unused
		{"Nintendo Wii U", "Wii U", "Wii U", nullptr},	// Wii U (vWii)
	};

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
	d->fields.reserve(4);	// Maximum of 4 fields.

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
	// TODO: Convert to Retail/Korean/vWii for Wii titles?
	d->fields.addField_string_numeric(C_("Nintendo", "Key Index"), ticket->common_key_index);

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
 * @param pKeyBuf	[out] Pointer to key buffer
 * @param size		[in] Size of pKeyBuf (must be >= 16)
 * @return 0 on success; negative POSIX error code on error.
 */
ATTR_ACCESS_SIZE(write_only, 2, 3)
int WiiTicket::decryptTitleKey(uint8_t *pKeyBuf, size_t size) const
{
	RP_D(const WiiTicket);
	assert(d->isValid);
	assert(size >= 16);
	if (!d->isValid) {
		// Not valid...
		return -EIO;
	} else if (size < 16) {
		// Key buffer is too small.
		return -EINVAL;
	}

	// Determine the key in use by checking the issuer.
	// TODO: Common issuer check function in WiiPartition or WiiTicket?
	// TODO: WiiPartition probably isn't the best place for Wii U keys...
	char issuer[64];
	memcpy(issuer, d->ticket.v0.signature_issuer, sizeof(d->ticket.v0.signature_issuer));
	issuer[sizeof(issuer)-1] = '\0';

	uint32_t ca, xs;
	char c;
	int ret = sscanf(issuer, "Root-CA%08X-XS%08X%c", &ca, &xs, &c);
	if (ret != 2) {
		// Not a valid issuer...
		return -EINVAL;
	}

	uint8_t common_key_index = d->ticket.v0.common_key_index;
	if (common_key_index > 2) {
		// Out of range. Assume Wii common key.
		common_key_index = 0;
	}

	// Check CA and XS.
	WiiPartition::EncryptionKeys encKey;
	if (ca == 1 && xs == 3) {
		// RVL retail
		encKey = static_cast<WiiPartition::EncryptionKeys>(
			(int)WiiPartition::EncryptionKeys::Key_RVL_Common + common_key_index);
	} else if (ca == 2 && xs == 6) {
		// RVT debug (TODO: There's also XS00000004)
		encKey = static_cast<WiiPartition::EncryptionKeys>(
			(int)WiiPartition::EncryptionKeys::Key_RVT_Debug + common_key_index);
	} else if (ca == 3 && xs == 0xc) {
		// CTR/WUP retail
		encKey = WiiPartition::EncryptionKeys::Key_WUP_Starbuck_WiiU_Common;
	} else if (ca == 4 && xs == 0xf) {
		// CAT debug
		encKey = WiiPartition::EncryptionKeys::Key_CAT_Starbuck_WiiU_Common;
	} else {
		// Unsupported CA/XS combination.
		return -EINVAL;
	}

	// Get the Key Manager instance.
	KeyManager *const keyManager = KeyManager::instance();
	assert(keyManager != nullptr);

	// Initialize the AES cipher.
	unique_ptr<IAesCipher> cipher(AesCipherFactory::create());
	if (!cipher || !cipher->isInit()) {
		// Error initializing the cipher.
		// TODO: Return verifyResult?
		//verifyResult = KeyManager::VerifyResult::IAesCipherInitErr;
		return -EIO;
	}

	// Get the common key.
	KeyManager::KeyData_t keyData;
	KeyManager::VerifyResult verifyResult = keyManager->getAndVerify(
		WiiPartition::encryptionKeyName_static(static_cast<int>(encKey)), &keyData,
		WiiPartition::encryptionVerifyData_static(static_cast<int>(encKey)), 16);
	if (verifyResult != KeyManager::VerifyResult::OK) {
		// An error occurred while loading the common key.
		// TODO: Return verifyResult?
		return -EINVAL;
	}

	// Load the common key into the AES cipher. (CBC mode)
	ret = cipher->setKey(keyData.key, keyData.length);
	ret |= cipher->setChainingMode(IAesCipher::ChainingMode::CBC);
	if (ret != 0) {
		// Error initializing the cipher.
		// TODO: Return verifyResult?
		//verifyResult = KeyManager::VerifyResult::IAesCipherInitErr;
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
		//verifyResult = KeyManager::VerifyResult::IAesCipherDecryptErr;
		return -EIO;
	}

	// Title key decrypted.
	return 0;
}
#endif /* ENABLE_DECRYPTION */

}
