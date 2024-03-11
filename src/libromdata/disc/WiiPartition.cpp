/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiPartition.hpp: Wii partition reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "WiiPartition.hpp"
#include "Console/wii_structs.h"

// librpbase
#include "librpbase/crypto/KeyManager.hpp"
#ifdef ENABLE_DECRYPTION
#  include "librpbase/crypto/IAesCipher.hpp"
#  include "librpbase/crypto/AesCipherFactory.hpp"
#endif /* ENABLE_DECRYPTION */
using namespace LibRpBase;

// C++ STL classes
using std::unique_ptr;

#include "GcnPartition_p.hpp"
namespace LibRomData {

#define SECTOR_SIZE_ENCRYPTED 0x8000
#define SECTOR_SIZE_DECRYPTED 0x7C00
#define SECTOR_SIZE_DECRYPTED_OFFSET 0x400

class WiiPartitionPrivate final : public GcnPartitionPrivate
{
public:
	WiiPartitionPrivate(WiiPartition *q, off64_t data_size,
		off64_t partition_offset, WiiPartition::CryptoMethod cryptoMethod);
	~WiiPartitionPrivate() final;

private:
	typedef GcnPartitionPrivate super;
	RP_DISABLE_COPY(WiiPartitionPrivate)

public:
	// Partition header
	RVL_PartitionHeader partitionHeader;

	// Encryption key verification result
	KeyManager::VerifyResult verifyResult;

public:
	/**
	 * Determine the encryption key used by this partition.
	 * This initializes encKey and encKeyReal.
	 */
	void getEncKey(void);

	// Encryption key in use
	WiiPartition::EncKey encKey;
	// Encryption key that would be used if the partition was encrypted
	WiiPartition::EncKey encKeyReal;

	// Crypto method
	WiiPartition::CryptoMethod cryptoMethod;

public:
	// Decrypted read position. (0x7C00 bytes out of 0x8000)
	// NOTE: Actual read position if ((cryptoMethod & CM_MASK_SECTOR) == CM_32K).
	off64_t pos_7C00;

	// Decrypted sector cache.
	// NOTE: Actual data starts at 0x400.
	// Hashes and the sector IV are stored first.
	uint32_t sector_num;				// Sector number.
	union EncSector_t {
		struct {
			// NOTE: &hashes.H2[7][4], when encrypted, is the sector IV.
			struct {
				uint8_t H0[31][20];
				uint8_t pad_H0[20];
				uint8_t H1[8][20];
				uint8_t pad_H1[32];
				uint8_t H2[8][20];
				uint8_t pad_H2[32];
			} hashes;
			uint8_t data[SECTOR_SIZE_DECRYPTED];
		};
		uint8_t fulldata[SECTOR_SIZE_ENCRYPTED];
	};
	ASSERT_STRUCT(EncSector_t, SECTOR_SIZE_ENCRYPTED);
	static_assert(offsetof(EncSector_t, hashes.H2) + (7*20) + 4 == 0x3D0, "IV location is wrong");
	EncSector_t sector_buf;	// Decrypted sector data.

	/**
	 * Read and decrypt a sector.
	 * The decrypted sector is stored in sector_buf.
	 *
	 * @param sector_num Sector number. (address / 0x7C00)
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int readSector(uint32_t sector_num);

#ifdef ENABLE_DECRYPTION
public:
	// AES cipher for this partition's title key
	IAesCipher *aes_title;
	// Decrypted title key
	uint8_t title_key[16];

	/**
	 * Initialize decryption.
	 * @return VerifyResult.
	 */
	KeyManager::VerifyResult initDecryption(void);

public:
	// Verification key names
	static const std::array<const char*, WiiPartition::Key_Max> EncryptionKeyNames;

	// Verification key data
	static const uint8_t EncryptionKeyVerifyData[WiiPartition::Key_Max][16];
#endif
};

/** WiiPartitionPrivate **/

#ifdef ENABLE_DECRYPTION

// Verification key names.
const std::array<const char*, WiiPartition::Key_Max> WiiPartitionPrivate::EncryptionKeyNames = {{
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
}};

const uint8_t WiiPartitionPrivate::EncryptionKeyVerifyData[WiiPartition::Key_Max][16] = {
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
};
#endif /* ENABLE_DECRYPTION */

WiiPartitionPrivate::WiiPartitionPrivate(WiiPartition *q,
		off64_t data_size, off64_t partition_offset,
		WiiPartition::CryptoMethod cryptoMethod)
	: super(q, partition_offset, data_size, 2)
#ifdef ENABLE_DECRYPTION
	, verifyResult(KeyManager::VerifyResult::Unknown)
	, encKey(WiiPartition::EncKey::Unknown)
	, encKeyReal(WiiPartition::EncKey::Unknown)
	, cryptoMethod(cryptoMethod)
	, pos_7C00(-1)
	, sector_num(~0)
	, aes_title(nullptr)
#else /* !ENABLE_DECRYPTION */
	, verifyResult(KeyManager::VerifyResult::NoSupport)
	, encKey(WiiPartition::EncKey::Unknown)
	, encKeyReal(WiiPartition::EncKey::Unknown)
	, cryptoMethod(cryptoMethod)
	, pos_7C00(-1)
	, sector_num(~0)
#endif /* ENABLE_DECRYPTION */
{
	// Clear data set by GcnPartition in case the
	// partition headers can't be read.
	this->data_offset = -1;
	this->data_size = -1;
	this->partition_size = -1;

	// Clear the partition header struct.
	memset(&partitionHeader, 0, sizeof(partitionHeader));

	// Partition header will be read in the WiiPartition constructor.
}

/**
 * Determine the encryption key used by this partition.
 * This initializes encKey and encKeyReal.
 */
void WiiPartitionPrivate::getEncKey(void)
{
	if (encKey > WiiPartition::EncKey::Unknown) {
		// Encryption key has already been determined.
		return;
	}

	encKey = WiiPartition::EncKey::Unknown;
	encKeyReal = WiiPartition::EncKey::Unknown;
	if (partition_size < 0) {
		// Error loading the partition header.
		return;
	}

	assert(partitionHeader.ticket.common_key_index <= 1);
	const uint8_t keyIdx = partitionHeader.ticket.common_key_index;

	// Check the issuer to determine Retail vs. Debug.
	static const char issuer_rvt[] = "Root-CA00000002-XS00000006";
	if (!memcmp(partitionHeader.ticket.signature_issuer, issuer_rvt, sizeof(issuer_rvt))) {
		// Debug issuer. Use the debug key for keyIdx == 0.
		// NOTE: vWii common key shouldn't be used for discs,
		// but we'll allow it anyway...
		if (keyIdx <= 2) {
			encKeyReal = static_cast<WiiPartition::EncKey>(
				(int)WiiPartition::EncKey::RVT_Debug + keyIdx);
		}
	} else {
		// Assuming Retail issuer.
		// NOTE: vWii common key shouldn't be used for discs,
		// but we'll allow it anyway...
		if (keyIdx <= 2) {
			// keyIdx maps to encKey directly for retail.
			encKeyReal = static_cast<WiiPartition::EncKey>(
				(int)WiiPartition::EncKey::RVL_Common + keyIdx);
		}
	}

	if ((cryptoMethod & WiiPartition::CM_MASK_ENCRYPTED) == WiiPartition::CM_UNENCRYPTED) {
		// Not encrypted.
		encKey = WiiPartition::EncKey::None;
	} else {
		// Encrypted.
		encKey = encKeyReal;
	}
}

#ifdef ENABLE_DECRYPTION
/**
 * Initialize decryption.
 * @return VerifyResult.
 */
KeyManager::VerifyResult WiiPartitionPrivate::initDecryption(void)
{
	if (verifyResult != KeyManager::VerifyResult::Unknown) {
		// Decryption has already been initialized.
		return verifyResult;
	}

	// If decryption is enabled, we can load the key and enable reading.
	// Otherwise, we can only get the partition size information.

	// Get the Key Manager instance.
	KeyManager *const keyManager = KeyManager::instance();
	assert(keyManager != nullptr);

	// Determine the required encryption key.
	getEncKey();
	if (encKey <= WiiPartition::EncKey::Unknown) {
		// Invalid encryption key index.
		// Use VerifyResult::KeyNotFound here.
		// This condition is indicated by VerifyResult::KeyNotFound
		// and a key index EncKey::Unknown.
		verifyResult = KeyManager::VerifyResult::KeyNotFound;
		return verifyResult;
	}

	// Determine the encryption key to use.
	WiiPartition::EncryptionKeys keyIdx;
	switch (encKey) {
		case WiiPartition::EncKey::RVL_Common:
			keyIdx = WiiPartition::Key_Rvl_Common;
			break;
		case WiiPartition::EncKey::RVL_Korean:
			keyIdx = WiiPartition::Key_Rvl_Korean;
			break;
		case WiiPartition::EncKey::WUP_vWii:
			keyIdx = WiiPartition::Key_Wup_Starbuck_vWii_Common;
			break;
		case WiiPartition::EncKey::RVT_Debug:
			keyIdx = WiiPartition::Key_Rvt_Debug;
			break;
		case WiiPartition::EncKey::RVT_Korean:
			keyIdx = WiiPartition::Key_Rvt_Korean;
			break;
		case WiiPartition::EncKey::CAT_vWii:
			keyIdx = WiiPartition::Key_Cat_Starbuck_vWii_Common;
			break;
		default:
			// Unknown key...
			verifyResult = KeyManager::VerifyResult::KeyNotFound;
			return verifyResult;
	}

	// Initialize the AES cipher.
	unique_ptr<IAesCipher> cipher(AesCipherFactory::create());
	if (!cipher || !cipher->isInit()) {
		// Error initializing the cipher.
		verifyResult = KeyManager::VerifyResult::IAesCipherInitErr;
		return verifyResult;
	}

	// Get the common key.
	KeyManager::KeyData_t keyData;
	verifyResult = keyManager->getAndVerify(
		WiiPartitionPrivate::EncryptionKeyNames[keyIdx], &keyData,
		WiiPartitionPrivate::EncryptionKeyVerifyData[keyIdx], 16);
	if (verifyResult != KeyManager::VerifyResult::OK) {
		// An error occurred loading while the common key.
		return verifyResult;
	}

	// Load the common key. (CBC mode)
	int ret = cipher->setKey(keyData.key, keyData.length);
	ret |= cipher->setChainingMode(IAesCipher::ChainingMode::CBC);
	if (ret != 0) {
		// Error initializing the cipher.
		verifyResult = KeyManager::VerifyResult::IAesCipherInitErr;
		return verifyResult;
	}

	// Get the IV.
	// First 8 bytes are the title ID.
	// Second 8 bytes are all 0.
	uint8_t iv[16];
	memcpy(iv, partitionHeader.ticket.title_id.u8, 8);
	memset(&iv[8], 0, 8);

	// Decrypt the title key.
	memcpy(title_key, partitionHeader.ticket.enc_title_key, sizeof(title_key));
	if (cipher->decrypt(title_key, sizeof(title_key), iv, sizeof(iv)) != sizeof(title_key)) {
		// Error decrypting the title key.
		verifyResult = KeyManager::VerifyResult::IAesCipherDecryptErr;
		return verifyResult;
	}

	// Set the title key in the AES cipher.
	if (cipher->setKey(title_key, sizeof(title_key)) != 0) {
		// Error initializing the cipher.
		verifyResult = KeyManager::VerifyResult::IAesCipherInitErr;
		return verifyResult;
	}

	// readSector() needs aes_title.
	delete aes_title;
	aes_title = cipher.release();

	// Read sector 0, which contains a disc header.
	// NOTE: readSector() doesn't check verifyResult.
	if (readSector(0) != 0) {
		// Error reading sector 0.
		delete aes_title;
		aes_title = nullptr;
		verifyResult = KeyManager::VerifyResult::IAesCipherDecryptErr;
		return verifyResult;
	}

	// Verify that this is a Wii partition.
	// If it isn't, the key is probably wrong.
	const GCN_DiscHeader *const discHeader =
		reinterpret_cast<const GCN_DiscHeader*>(sector_buf.data);
	if (discHeader->magic_wii != cpu_to_be32(WII_MAGIC)) {
		// Invalid disc header.

		// NOTE: Debug discs may have incrementing values in update partitions.
		static const uint8_t incr_vals[32] = {
			0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x04,
			0x00,0x00,0x00,0x08, 0x00,0x00,0x00,0x0C,
			0x00,0x00,0x00,0x10, 0x00,0x00,0x00,0x14,
			0x00,0x00,0x00,0x18, 0x00,0x00,0x00,0x1C,
		};
		if (!memcmp(sector_buf.data, incr_vals, sizeof(incr_vals))) {
			// Found incrementing values.
			verifyResult = KeyManager::VerifyResult::IncrementingValues;
		} else {
			// Not incrementing values.
			// We probably have the wrong key.
			verifyResult = KeyManager::VerifyResult::WrongKey;
		}
		return verifyResult;
	}

	// Cipher initialized.
	verifyResult = KeyManager::VerifyResult::OK;
	return verifyResult;
}
#endif /* ENABLE_DECRYPTION */

WiiPartitionPrivate::~WiiPartitionPrivate()
{
#ifdef ENABLE_DECRYPTION
	delete aes_title;
#endif /* ENABLE_DECRYPTION */
}

/**
 * Read and decrypt a sector.
 * The decrypted sector is stored in sector_buf.
 *
 * @param sector_num Sector number. (address / 0x7C00)
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiPartitionPrivate::readSector(uint32_t sector_num)
{
	if (this->sector_num == sector_num) {
		// Sector is already in memory.
		return 0;
	}

	RP_Q(WiiPartition);
	const bool isCrypted = ((cryptoMethod & WiiPartition::CM_MASK_ENCRYPTED) == WiiPartition::CM_ENCRYPTED);
#ifndef ENABLE_DECRYPTION
	if (isCrypted) {
		// Decryption is disabled.
		q->m_lastError = EIO;
		return -1;
	}
#endif /* !ENABLE_DECRYPTION */

	// NOTE: This function doesn't check verifyResult,
	// since it's called by initDecryption() before
	// verifyResult is set.
	off64_t sector_addr = partition_offset + data_offset;
	sector_addr += (static_cast<off64_t>(sector_num) * SECTOR_SIZE_ENCRYPTED);

	int ret = q->m_file->seek(sector_addr);
	if (ret != 0) {
		q->m_lastError = q->m_file->lastError();
		return ret;
	}

	size_t sz = q->m_file->read(&sector_buf, sizeof(sector_buf));
	if (sz != sizeof(sector_buf)) {
		// sector_buf may be invalid.
		this->sector_num = ~0;
		q->m_lastError = EIO;
		return -1;
	}

#ifdef ENABLE_DECRYPTION
	if (isCrypted) {
		// Decrypt the sector.
		if (aes_title->decrypt(sector_buf.data, sizeof(sector_buf.data),
		    &sector_buf.hashes.H2[7][4], 16) != SECTOR_SIZE_DECRYPTED)
		{
			// sector_buf may be invalid.
			this->sector_num = ~0;
			q->m_lastError = EIO;
			return -1;
		}
	}
#endif /* ENABLE_DECRYPTION */

	// Sector read and decrypted.
	this->sector_num = sector_num;
	return 0;
}

/** WiiPartition **/

/**
 * Construct a WiiPartition with the specified IDiscReader.
 *
 * NOTE: The IDiscReader *must* remain valid while this
 * WiiPartition is open.
 *
 * @param discReader		[in] IDiscReader.
 * @param partition_offset	[in] Partition start offset.
 * @param partition_size	[in] Calculated partition size. Used if the size in the header is 0.
 * @param cryptoMethod		[in] Crypto method.
 */
WiiPartition::WiiPartition(const IDiscReaderPtr &discReader, off64_t partition_offset,
		off64_t partition_size, CryptoMethod cryptoMethod)
	: super(new WiiPartitionPrivate(this, discReader->size(),
		partition_offset, cryptoMethod), discReader)
{
	// q->m_lastError is handled by GcnPartitionPrivate's constructor.
	if (!m_file || !m_file->isOpen()) {
		m_file.reset();
		return;
	}

	// Read the partition header.
	RP_D(WiiPartition);
	if (m_file->seek(partition_offset) != 0) {
		m_lastError = m_file->lastError();
		m_file.reset();
		return;
	}
	size_t size = m_file->read(&d->partitionHeader, sizeof(d->partitionHeader));
	if (size != sizeof(d->partitionHeader)) {
		m_lastError = EIO;
		m_file.reset();
		return;
	}

	// Make sure the signature type is correct.
	if (d->partitionHeader.ticket.signature_type != cpu_to_be32(RVL_CERT_SIGTYPE_RSA2048_SHA1)) {
		// TODO: Better error?
		m_lastError = EIO;
		m_file.reset();
		return;
	}

	// Save important data.
	d->data_offset = d->partitionHeader.data_offset.geto_be();
	d->data_size   = d->partitionHeader.data_size.geto_be();
	if (d->data_size == 0) {
		// NoCrypto RVT-H images sometimes have this set to 0.
		// Use the calculated partition size.
		d->data_size = partition_size - d->data_offset;
	}

	d->partition_size = d->data_size + d->data_offset;
#ifdef ENABLE_DECRYPTION
	d->pos_7C00 = 0;
#endif /* ENABLE_DECRYPTION */

	// Unencrypted discs don't need decryption.
	if ((cryptoMethod & WiiPartition::CM_MASK_ENCRYPTED) == WiiPartition::CM_UNENCRYPTED) {
		// No encryption. (RVT-H)

		// NOTE: Debug discs may have incrementing values in update partitions.
		static const uint8_t incr_vals[32] = {
			0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x04,
			0x00,0x00,0x00,0x08, 0x00,0x00,0x00,0x0C,
			0x00,0x00,0x00,0x10, 0x00,0x00,0x00,0x14,
			0x00,0x00,0x00,0x18, 0x00,0x00,0x00,0x1C,
		};

		uint8_t data[32];
		size_t size = m_file->seekAndRead(d->partition_offset + d->data_offset, data, sizeof(data));
		if (size == sizeof(incr_vals) && !memcmp(data, incr_vals, sizeof(incr_vals))) {
			// Found incrementing values.
			d->verifyResult = KeyManager::VerifyResult::IncrementingValues;
		} else {
			// Assume the disc is OK.
			// TODO: Check for Wii magic?
			d->verifyResult = KeyManager::VerifyResult::OK;
		}
	}

	// Encryption will not be initialized until
	// read() is called.
}

/** IDiscReader **/

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t WiiPartition::read(void *ptr, size_t size)
{
	RP_D(WiiPartition);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	if (!m_file || !m_file->isOpen()) {
		m_lastError = EBADF;
		return 0;
	}

	// TODO: Consolidate this code and optimize it.
	size_t ret = 0;
	uint8_t *ptr8 = static_cast<uint8_t*>(ptr);

	// Are we already at the end of the file?
	if (d->pos_7C00 >= d->data_size)
		return 0;

	// Make sure d->pos_7C00 + size <= d->data_size.
	// If it isn't, we'll do a short read.
	if (d->pos_7C00 + static_cast<off64_t>(size) >= d->data_size) {
		size = static_cast<size_t>(d->data_size - d->pos_7C00);
	}

	if ((d->cryptoMethod & CM_MASK_SECTOR) == CM_32K) {
		// Full 32K sectors. (implies no encryption)

		// Check if we're not starting on a block boundary.
		const uint32_t blockStartOffset = d->pos_7C00 % SECTOR_SIZE_ENCRYPTED;
		if (blockStartOffset != 0) {
			// Not a block boundary.
			// Read the end of the block.
			uint32_t read_sz = SECTOR_SIZE_ENCRYPTED - blockStartOffset;
			if (size < static_cast<size_t>(read_sz)) {
				read_sz = static_cast<uint32_t>(size);
			}

			// Read and decrypt the sector.
			const uint32_t blockStart = static_cast<uint32_t>(d->pos_7C00 / SECTOR_SIZE_ENCRYPTED);
			d->readSector(blockStart);

			// Copy data from the sector.
			memcpy(ptr8, &d->sector_buf.fulldata[blockStartOffset], read_sz);

			// Starting block read.
			size -= read_sz;
			ptr8 += read_sz;
			ret += read_sz;
			d->pos_7C00 += read_sz;
		}

		// Read entire blocks.
		for (; size >= SECTOR_SIZE_ENCRYPTED;
		     size -= SECTOR_SIZE_ENCRYPTED, ptr8 += SECTOR_SIZE_ENCRYPTED,
		     ret += SECTOR_SIZE_ENCRYPTED, d->pos_7C00 += SECTOR_SIZE_ENCRYPTED)
		{
			assert(d->pos_7C00 % SECTOR_SIZE_ENCRYPTED == 0);

			// Read the sector.
			const uint32_t blockStart = static_cast<uint32_t>(d->pos_7C00 / SECTOR_SIZE_ENCRYPTED);
			d->readSector(blockStart);

			// Copy data from the sector.
			memcpy(ptr8, d->sector_buf.fulldata, SECTOR_SIZE_ENCRYPTED);
		}

		// Check if we still have data left. (not a full block)
		if (size > 0) {
			// Not a full block.

			// Read the sector.
			assert(d->pos_7C00 % SECTOR_SIZE_ENCRYPTED == 0);
			const uint32_t blockEnd = static_cast<uint32_t>(d->pos_7C00 / SECTOR_SIZE_ENCRYPTED);
			d->readSector(blockEnd);

			// Copy data from the sector.
			memcpy(ptr8, d->sector_buf.fulldata, size);

			ret += size;
			d->pos_7C00 += size;
		}
	} else {
		if ((d->cryptoMethod & CM_MASK_ENCRYPTED) == CM_ENCRYPTED) {
#ifdef ENABLE_DECRYPTION
			// Make sure decryption is initialized.
			switch (d->verifyResult) {
				case KeyManager::VerifyResult::Unknown:
					// Attempt to initialize decryption.
					if (d->initDecryption() != KeyManager::VerifyResult::OK) {
						// Decryption could not be initialized.
						// TODO: Better error?
						m_lastError = EIO;
						return 0;
					}
					break;

				case KeyManager::VerifyResult::OK:
					// Decryption is initialized.
					break;

				default:
					// Decryption failed to initialize.
					// TODO: Better error?
					m_lastError = EIO;
					return 0;
			}
#else /* !ENABLE_DECRYPTION */
			// Decryption is not enabled.
			m_lastError = EIO;
			ret = 0;
#endif /* ENABLE_DECRYPTION */
		}

		// Check if we're not starting on a block boundary.
		const uint32_t blockStartOffset = d->pos_7C00 % SECTOR_SIZE_DECRYPTED;
		if (blockStartOffset != 0) {
			// Not a block boundary.
			// Read the end of the block.
			uint32_t read_sz = SECTOR_SIZE_DECRYPTED - blockStartOffset;
			if (size < static_cast<size_t>(read_sz)) {
				read_sz = static_cast<uint32_t>(size);
			}

			// Read and decrypt the sector.
			const uint32_t blockStart = static_cast<uint32_t>(d->pos_7C00 / SECTOR_SIZE_DECRYPTED);
			d->readSector(blockStart);

			// Copy data from the sector.
			memcpy(ptr8, &d->sector_buf.data[blockStartOffset], read_sz);

			// Starting block read.
			size -= read_sz;
			ptr8 += read_sz;
			ret += read_sz;
			d->pos_7C00 += read_sz;
		}

		// Read entire blocks.
		for (; size >= SECTOR_SIZE_DECRYPTED;
		size -= SECTOR_SIZE_DECRYPTED, ptr8 += SECTOR_SIZE_DECRYPTED,
		ret += SECTOR_SIZE_DECRYPTED, d->pos_7C00 += SECTOR_SIZE_DECRYPTED)
		{
			assert(d->pos_7C00 % SECTOR_SIZE_DECRYPTED == 0);

			// Read and decrypt the sector.
			const uint32_t blockStart = static_cast<uint32_t>(d->pos_7C00 / SECTOR_SIZE_DECRYPTED);
			d->readSector(blockStart);

			// Copy data from the sector.
			memcpy(ptr8, d->sector_buf.data, SECTOR_SIZE_DECRYPTED);
		}

		// Check if we still have data left. (not a full block)
		if (size > 0) {
			// Not a full block.

			// Read and decrypt the sector.
			assert(d->pos_7C00 % SECTOR_SIZE_DECRYPTED == 0);
			const uint32_t blockEnd = static_cast<uint32_t>(d->pos_7C00 / SECTOR_SIZE_DECRYPTED);
			d->readSector(blockEnd);

			// Copy data from the sector.
			memcpy(ptr8, d->sector_buf.data, size);

			ret += size;
			d->pos_7C00 += size;
		}
	}

	// Finished reading the data.
	return ret;
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int WiiPartition::seek(off64_t pos)
{
	RP_D(WiiPartition);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	if (!m_file ||  !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// Handle out-of-range cases.
	if (pos < 0) {
		// Negative is invalid.
		m_lastError = EINVAL;
		return -1;
	} else if (pos >= d->data_size) {
		d->pos_7C00 = d->data_size;
	} else {
		d->pos_7C00 = pos;
	}
	return 0;
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
off64_t WiiPartition::tell(void)
{
	RP_D(const WiiPartition);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	if (!m_file ||  !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	return d->pos_7C00;
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
off64_t WiiPartition::partition_size_used(void) const
{
	// Get the FST used size from GcnPartition.
	off64_t size = super::partition_size_used();
	if (size <= 0) {
		// Error retrieving the FST used size.
		return size;
	}

	// Add the data offset from the partition header.
	RP_D(const WiiPartition);
	size += d->partitionHeader.data_offset.geto_be();

	// Are sectors hashed?
	if ((d->cryptoMethod & WiiPartition::CM_MASK_SECTOR) == CM_1K_31K) {
		// Partition has hashed sectors.
		// Multiply the FST used size by 32/31 to adjust for the difference.
		size = (size * 32) / 31;
	}

	// We're done here.
	return size;
}

/** WiiPartition **/

/**
 * Encryption key verification result.
 * @return Encryption key verification result.
 */
KeyManager::VerifyResult WiiPartition::verifyResult(void) const
{
	// TODO: Errors?
	RP_D(const WiiPartition);
	return d->verifyResult;
}

/**
 * Get the encryption key in use.
 * @return Encryption key in use.
 */
WiiPartition::EncKey WiiPartition::encKey(void) const
{
	// TODO: Errors?
	RP_D(WiiPartition);
	d->getEncKey();
	return d->encKey;
}

/**
 * Get the encryption key that would be in use if the partition was encrypted.
 * This is only needed for NASOS images.
 * @return "Real" encryption key in use.
 */
WiiPartition::EncKey WiiPartition::encKeyReal(void) const
{
	// TODO: Errors?
	RP_D(WiiPartition);
	d->getEncKey();
	return d->encKeyReal;
}

/**
 * Get the ticket.
 * @return Ticket, or nullptr if unavailable.
 */
const RVL_Ticket *WiiPartition::ticket(void) const
{
	RP_D(const WiiPartition);
	return (d->partitionHeader.ticket.signature_type != 0
		? &d->partitionHeader.ticket
		: nullptr);
}

/**
 * Get the TMD header.
 * @return TMD header, or nullptr if unavailable.
 */
const RVL_TMD_Header *WiiPartition::tmdHeader(void) const
{
	RP_D(const WiiPartition);
	const RVL_TMD_Header *const tmdHeader =
		reinterpret_cast<const RVL_TMD_Header*>(d->partitionHeader.tmd);
	return (tmdHeader->signature_type != 0
		? tmdHeader
		: nullptr);
}

/**
 * Get the title ID. (NOT BYTESWAPPED)
 * @return Title ID. (0-0 if unavailable)
 */
Nintendo_TitleID_BE_t WiiPartition::titleID(void) const
{
	RP_D(const WiiPartition);
	return d->partitionHeader.ticket.title_id;
}

#ifdef ENABLE_DECRYPTION
/** Encryption keys. **/

/**
 * Get the total number of encryption key names.
 * @return Number of encryption key names.
 */
int WiiPartition::encryptionKeyCount_static(void)
{
	return Key_Max;
}

/**
 * Get an encryption key name.
 * @param keyIdx Encryption key index.
 * @return Encryption key name (in ASCII), or nullptr on error.
 */
const char *WiiPartition::encryptionKeyName_static(int keyIdx)
{
	assert(keyIdx >= 0);
	assert(keyIdx < Key_Max);
	if (keyIdx < 0 || keyIdx >= Key_Max)
		return nullptr;
	return WiiPartitionPrivate::EncryptionKeyNames[keyIdx];
}

/**
 * Get the verification data for a given encryption key index.
 * @param keyIdx Encryption key index.
 * @return Verification data. (16 bytes)
 */
const uint8_t *WiiPartition::encryptionVerifyData_static(int keyIdx)
{
	assert(keyIdx >= 0);
	assert(keyIdx < Key_Max);
	if (keyIdx < 0 || keyIdx >= Key_Max)
		return nullptr;
	return WiiPartitionPrivate::EncryptionKeyVerifyData[keyIdx];
}
#endif /* ENABLE_DECRYPTION */

}
