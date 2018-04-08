/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiPartition.hpp: Wii partition reader.                                 *
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

#include "config.librpbase.h"
#include "WiiPartition.hpp"

#include "Console/gcn_structs.h"
#include "GcnFst.hpp"

// librpbase
#include "librpbase/byteswap.h"
#include "librpbase/crypto/KeyManager.hpp"
#ifdef ENABLE_DECRYPTION
# include "librpbase/crypto/IAesCipher.hpp"
# include "librpbase/crypto/AesCipherFactory.hpp"
#endif /* ENABLE_DECRYPTION */
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <memory>
using std::unique_ptr;

#include "GcnPartitionPrivate.hpp"

namespace LibRomData {

#define SECTOR_SIZE_ENCRYPTED 0x8000
#define SECTOR_SIZE_DECRYPTED 0x7C00
#define SECTOR_SIZE_DECRYPTED_OFFSET 0x400

class WiiPartitionPrivate : public GcnPartitionPrivate
{
	public:
		WiiPartitionPrivate(WiiPartition *q, IDiscReader *discReader,
			int64_t partition_offset, int64_t partition_size, bool noCrypt);
		virtual ~WiiPartitionPrivate();

	private:
		typedef GcnPartitionPrivate super;
		RP_DISABLE_COPY(WiiPartitionPrivate)

	public:
		// Partition header.
		RVL_PartitionHeader partitionHeader;

		// Encryption key verification result.
		KeyManager::VerifyResult verifyResult;

	public:
		/**
		 * Determine the encryption key used by this partition.
		 * @return encKey.
		 */
		WiiPartition::EncKey getEncKey(void);

	private:
		// Encryption key in use.
		WiiPartition::EncKey m_encKey;

	public:
		// If true, the disc image is not encrypted. (RVT-H)
		bool noCrypt;

#ifdef ENABLE_DECRYPTION
	public:
		// AES cipher for this partition's title key.
		IAesCipher *aes_title;
		// Decrypted title key.
		uint8_t title_key[16];

		/**
		 * Initialize decryption.
		 * @return VerifyResult.
		 */
		KeyManager::VerifyResult initDecryption(void);

		// Decrypted read position. (0x7C00 bytes out of 0x8000)
		int64_t pos_7C00;

		// Decrypted sector cache.
		// NOTE: Actual data starts at 0x400.
		// Hashes and the sector IV are stored first.
		uint32_t sector_num;				// Sector number.
		uint8_t sector_buf[SECTOR_SIZE_ENCRYPTED];	// Decrypted sector data.

		/**
		 * Read and decrypt a sector.
		 * The decrypted sector is stored in sector_buf.
		 *
		 * @param sector_num Sector number. (address / 0x7C00)
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int readSector(uint32_t sector_num);

	public:
		// Verification key names.
		static const char *const EncryptionKeyNames[WiiPartition::Key_Max];

		// Verification key data.
		static const uint8_t EncryptionKeyVerifyData[WiiPartition::Key_Max][16];
#endif
};

/** WiiPartitionPrivate **/

#ifdef ENABLE_DECRYPTION

// Verification key names.
const char *const WiiPartitionPrivate::EncryptionKeyNames[WiiPartition::Key_Max] = {
	// Retail
	"rvl-common",
	"rvl-korean",
	"wup-vwii-common",
	"rvl-sd-aes",
	"rvl-sd-iv",
	"rvl-sd-md5",

	// Debug
	"rvt-debug",
};

const uint8_t WiiPartitionPrivate::EncryptionKeyVerifyData[WiiPartition::Key_Max][16] = {
	/** Retail **/

	// rvl-common
	{0xCF,0xB7,0xFF,0xA0,0x64,0x0C,0x7A,0x7D,
	 0xA7,0x22,0xDC,0x16,0x40,0xFA,0x04,0x58},
	// rvl-korean
	{0x98,0x1C,0xD4,0x51,0x17,0xF2,0x23,0xB6,
	 0xC8,0x84,0x4A,0x97,0xA6,0x93,0xF2,0xE3},
	// wup-vwii-common
	{0x04,0xF1,0x33,0x3F,0xF8,0x05,0x7B,0x8F,
	 0xA7,0xF1,0xED,0x6E,0xAC,0x23,0x33,0xFA},
	// rvl-sd-aes
	{0x8C,0x1C,0xBA,0x01,0x02,0xB9,0x6F,0x65,
	 0x24,0x7C,0x85,0x3C,0x0F,0x3B,0x8C,0x37},
	// rvl-sd-iv
	{0xE3,0xEE,0xE5,0x0F,0xDC,0xFD,0xBE,0x89,
	 0x20,0x05,0xF2,0xB9,0xD8,0x1D,0xF1,0x27},
	// rvl-sd-md5
	{0xF8,0xE1,0x8D,0x89,0x06,0xC7,0x21,0x32,
	 0x9D,0xE0,0x14,0x19,0x30,0xC3,0x88,0x1F},

	/** Debug **/

	// rvt-debug
	{0x22,0xC4,0x2C,0x5B,0xCB,0xFE,0x75,0xAC,
	 0xEB,0xC3,0x6B,0xAF,0x90,0xB3,0xB4,0xF5},
};
#endif /* ENABLE_DECRYPTION */

WiiPartitionPrivate::WiiPartitionPrivate(WiiPartition *q,
		IDiscReader *discReader, int64_t partition_offset,
		int64_t partition_size, bool noCrypt)
	: super(q, discReader, partition_offset, 2)
	, noCrypt(noCrypt)
#ifdef ENABLE_DECRYPTION
	, verifyResult(KeyManager::VERIFY_UNKNOWN)
	, m_encKey(WiiPartition::ENCKEY_UNKNOWN)
	, aes_title(nullptr)
	, pos_7C00(-1)
	, sector_num(~0)
#else /* !ENABLE_DECRYPTION */
	, verifyResult(KeyManager::VERIFY_NO_SUPPORT)
	, m_encKey(WiiPartition::ENCKEY_UNKNOWN)
#endif /* ENABLE_DECRYPTION */
{
	if (noCrypt) {
		// No encryption. (RVT-H)
		verifyResult = KeyManager::VERIFY_OK;
		m_encKey = WiiPartition::ENCKEY_NONE;
	}

	// Clear data set by GcnPartition in case the
	// partition headers can't be read.
	this->data_offset = -1;
	this->data_size = -1;
	this->partition_size = -1;

	// Clear the partition header struct.
	memset(&partitionHeader, 0, sizeof(partitionHeader));

	if (!discReader->isOpen()) {
		q->m_lastError = discReader->lastError();
		return;
	}

	// Read the partition header.
	if (discReader->seek(partition_offset) != 0) {
		q->m_lastError = discReader->lastError();
		return;
	}
	size_t size = discReader->read(&partitionHeader, sizeof(partitionHeader));
	if (size != sizeof(partitionHeader)) {
		q->m_lastError = EIO;
		return;
	}

	// Make sure the signature type is correct.
	if (partitionHeader.ticket.signature_type != cpu_to_be32(RVL_SIGNATURE_TYPE_RSA2048)) {
		// TODO: Better error?
		q->m_lastError = EIO;
		return;
	}

	// Save important data.
	data_offset     = (int64_t)be32_to_cpu(partitionHeader.data_offset) << 2;
	data_size       = (int64_t)be32_to_cpu(partitionHeader.data_size) << 2;
	if (data_size == 0) {
		// NoCrypt RVT-H images sometimes have this set to 0.
		// Use the calculated partition size.
		data_size = partition_size - data_offset;
	}
	this->partition_size  = data_size + data_offset;
#ifdef ENABLE_DECRYPTION
	pos_7C00	= 0;
#endif /* ENABLE_DECRYPTION */

	// Encryption will not be initialized until
	// read() is called.
}

/**
 * Determine the encryption key used by this partition.
 * @return encKey.
 */
WiiPartition::EncKey WiiPartitionPrivate::getEncKey(void)
{
	if (m_encKey > WiiPartition::ENCKEY_UNKNOWN) {
		// Encryption key has already been determined.
		return m_encKey;
	}

	m_encKey = WiiPartition::ENCKEY_UNKNOWN;
	if (partition_size < 0) {
		// Error loading the partition header.
		return m_encKey;
	}

	assert(partitionHeader.ticket.common_key_index <= 1);
	const uint8_t keyIdx = partitionHeader.ticket.common_key_index;

	// Check the issuer to determine Retail vs. Debug.
	static const char issuer_rvt[] = "Root-CA00000002-XS00000006";
	if (!memcmp(partitionHeader.ticket.signature_issuer, issuer_rvt, sizeof(issuer_rvt))) {
		// Debug issuer. Use the debug key for keyIdx == 0.
		if (keyIdx == 0) {
			m_encKey = WiiPartition::ENCKEY_DEBUG;
		}
	} else {
		// Assuming Retail issuer.
		if (keyIdx <= 1) {
			// keyIdx maps to encKey directly for retail.
			m_encKey = (WiiPartition::EncKey)keyIdx;
		}
	}

	return m_encKey;
}

#ifdef ENABLE_DECRYPTION
/**
 * Initialize decryption.
 * @return VerifyResult.
 */
KeyManager::VerifyResult WiiPartitionPrivate::initDecryption(void)
{
	if (verifyResult != KeyManager::VERIFY_UNKNOWN) {
		// Decryption has already been initialized.
		return verifyResult;
	}

	// If decryption is enabled, we can load the key and enable reading.
	// Otherwise, we can only get the partition size information.

	// Get the Key Manager instance.
	KeyManager *keyManager = KeyManager::instance();
	assert(keyManager != nullptr);

	// Determine the required encryption key.
	const WiiPartition::EncKey encKey = getEncKey();
	if (encKey <= WiiPartition::ENCKEY_UNKNOWN) {
		// Invalid encryption key index.
		// Use VERIFY_KEY_NOT_FOUND here.
		// This condition is indicated by VERIFY_KEY_NOT_FOUND
		// and a key index ENCKEY_UNKNOWN.
		verifyResult = KeyManager::VERIFY_KEY_NOT_FOUND;
		return verifyResult;
	}

	// Determine the encryption key to use.
	WiiPartition::EncryptionKeys keyIdx;
	switch (encKey) {
		case WiiPartition::ENCKEY_COMMON:
			// Wii common key.
			keyIdx = WiiPartition::Key_Rvl_Common;
			break;
		case WiiPartition::ENCKEY_KOREAN:
			// Korean key.
			keyIdx = WiiPartition::Key_Rvl_Korean;
			break;
		case WiiPartition::ENCKEY_VWII:
			// vWii common key.
			keyIdx = WiiPartition::Key_Wup_vWii_Common;
			break;
		case WiiPartition::ENCKEY_DEBUG:
			// Debug key. (RVT-R)
			keyIdx = WiiPartition::Key_Rvt_Debug;
			break;
		default:
			// Unknown key...
			verifyResult = KeyManager::VERIFY_KEY_NOT_FOUND;
			return verifyResult;
	}

	// Initialize the AES cipher.
	unique_ptr<IAesCipher> cipher(AesCipherFactory::create());
	if (!cipher || !cipher->isInit()) {
		// Error initializing the cipher.
		verifyResult = KeyManager::VERFIY_IAESCIPHER_INIT_ERR;
		return verifyResult;
	}

	// Get the common key.
	KeyManager::KeyData_t keyData;
	verifyResult = keyManager->getAndVerify(
		WiiPartitionPrivate::EncryptionKeyNames[keyIdx], &keyData,
		WiiPartitionPrivate::EncryptionKeyVerifyData[keyIdx], 16);
	if (verifyResult != KeyManager::VERIFY_OK) {
		// An error occurred loading while the common key.
		return verifyResult;
	}

	// Load the common key. (CBC mode)
	int ret = cipher->setKey(keyData.key, keyData.length);
	ret |= cipher->setChainingMode(IAesCipher::CM_CBC);
	if (ret != 0) {
		// Error initializing the cipher.
		verifyResult = KeyManager::VERFIY_IAESCIPHER_INIT_ERR;
		return verifyResult;
	}

	// Get the IV.
	// First 8 bytes are the title ID.
	// Second 8 bytes are all 0.
	uint8_t iv[16];
	memcpy(iv, partitionHeader.ticket.title_id, 8);
	memset(&iv[8], 0, 8);

	// Decrypt the title key.
	memcpy(title_key, partitionHeader.ticket.enc_title_key, sizeof(title_key));
	if (cipher->decrypt(title_key, sizeof(title_key), iv, sizeof(iv)) != sizeof(title_key)) {
		// Error decrypting the title key.
		verifyResult = KeyManager::VERIFY_IAESCIPHER_DECRYPT_ERR;
		return verifyResult;
	}

	// Set the title key in the AES cipher.
	if (cipher->setKey(title_key, sizeof(title_key)) != 0) {
		// Error initializing the cipher.
		verifyResult = KeyManager::VERFIY_IAESCIPHER_INIT_ERR;
		return verifyResult;
	}

	// readSector() needs aes_title.
	if (aes_title) {
		delete aes_title;
	}
	aes_title = cipher.release();

	// Read sector 0, which contains a disc header.
	// NOTE: readSector() doesn't check verifyResult.
	if (readSector(0) != 0) {
		// Error reading sector 0.
		delete aes_title;
		aes_title = nullptr;
		verifyResult = KeyManager::VERIFY_IAESCIPHER_DECRYPT_ERR;
		return verifyResult;
	}

	// Verify that this is a Wii partition.
	// If it isn't, the key is probably wrong.
	const GCN_DiscHeader *discHeader =
		reinterpret_cast<const GCN_DiscHeader*>(&sector_buf[SECTOR_SIZE_DECRYPTED_OFFSET]);
	if (discHeader->magic_wii != cpu_to_be32(WII_MAGIC)) {
		// Invalid disc header.
		verifyResult = KeyManager::VERIFY_WRONG_KEY;
		return verifyResult;
	}

	// Cipher initialized.
	verifyResult = KeyManager::VERIFY_OK;
	return verifyResult;
}
#endif /* ENABLE_DECRYPTION */

WiiPartitionPrivate::~WiiPartitionPrivate()
{
#ifdef ENABLE_DECRYPTION
	delete aes_title;
#endif /* ENABLE_DECRYPTION */
}

#ifdef ENABLE_DECRYPTION
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

	// NOTE: This function doesn't check verifyResult,
	// since it's called by initDecryption() before
	// verifyResult is set.
	int64_t sector_addr = partition_offset + data_offset;
	sector_addr += ((int64_t)sector_num * SECTOR_SIZE_ENCRYPTED);

	RP_Q(WiiPartition);
	int ret = discReader->seek(sector_addr);
	if (ret != 0) {
		q->m_lastError = discReader->lastError();
		return ret;
	}

	size_t sz = discReader->read(sector_buf, sizeof(sector_buf));
	if (sz != SECTOR_SIZE_ENCRYPTED) {
		// sector_buf may be invalid.
		this->sector_num = ~0;
		q->m_lastError = EIO;
		return -1;
	}

	if (!noCrypt) {
		// Decrypt the sector.
		if (aes_title->decrypt(&sector_buf[SECTOR_SIZE_DECRYPTED_OFFSET], SECTOR_SIZE_DECRYPTED,
		    &sector_buf[0x3D0], 16) != SECTOR_SIZE_DECRYPTED)
		{
			// sector_buf may be invalid.
			this->sector_num = ~0;
			q->m_lastError = EIO;
			return -1;
		}
	}

	// Sector read and decrypted.
	this->sector_num = sector_num;
	return 0;
}
#endif /* ENABLE_DECRYPTION */

/** WiiPartition **/

/**
 * Construct a WiiPartition with the specified IDiscReader.
 *
 * NOTE: The IDiscReader *must* remain valid while this
 * WiiPartition is open.
 *
 * @param discReader IDiscReader.
 * @param partition_offset Partition start offset.
 */
WiiPartition::WiiPartition(IDiscReader *discReader, int64_t partition_offset,
		int64_t partition_size, bool noCrypt)
	: super(new WiiPartitionPrivate(this, discReader, partition_offset, partition_size, noCrypt))
{ }

WiiPartition::~WiiPartition()
{ }

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
	assert(d->discReader != nullptr);
	assert(d->discReader->isOpen());
	if (!d->discReader || !d->discReader->isOpen()) {
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
	if (d->pos_7C00 + (int64_t)size >= d->data_size) {
		size = (size_t)(d->data_size - d->pos_7C00);
	}

	if (d->noCrypt) {
		// No encryption.

		// Check if we're not starting on a block boundary.
		const uint32_t blockStartOffset = d->pos_7C00 % SECTOR_SIZE_ENCRYPTED;
		if (blockStartOffset != 0) {
			// Not a block boundary.
			// Read the end of the block.
			uint32_t read_sz = SECTOR_SIZE_ENCRYPTED - blockStartOffset;
			if (size < (size_t)read_sz) {
				read_sz = (uint32_t)size;
			}

			// Read and decrypt the sector.
			uint32_t blockStart = (uint32_t)(d->pos_7C00 / SECTOR_SIZE_ENCRYPTED);
			d->readSector(blockStart);

			// Copy data from the sector.
			memcpy(ptr8, &d->sector_buf[blockStartOffset], read_sz);

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
			uint32_t blockStart = (uint32_t)(d->pos_7C00 / SECTOR_SIZE_ENCRYPTED);
			d->readSector(blockStart);

			// Copy data from the sector.
			memcpy(ptr8, d->sector_buf, SECTOR_SIZE_ENCRYPTED);
		}

		// Check if we still have data left. (not a full block)
		if (size > 0) {
			// Not a full block.

			// Read the sector.
			assert(d->pos_7C00 % SECTOR_SIZE_ENCRYPTED == 0);
			uint32_t blockEnd = (uint32_t)(d->pos_7C00 / SECTOR_SIZE_ENCRYPTED);
			d->readSector(blockEnd);

			// Copy data from the sector.
			memcpy(ptr8, &d->sector_buf[blockStartOffset], size);

			ret += size;
			d->pos_7C00 += size;
		}
	} else {
#ifdef ENABLE_DECRYPTION
	// Make sure decryption is initialized.
	switch (d->verifyResult) {
		case KeyManager::VERIFY_UNKNOWN:
			// Attempt to initialize decryption.
			if (d->initDecryption() != KeyManager::VERIFY_OK) {
				// Decryption could not be initialized.
				// TODO: Better error?
				m_lastError = EIO;
				return -m_lastError;
			}
			break;

		case KeyManager::VERIFY_OK:
			// Decryption is initialized.
			break;

		default:
			// Decryption failed to initialize.
			// TODO: Better error?
			m_lastError = EIO;
			return -m_lastError;
	}

	// Check if we're not starting on a block boundary.
	const uint32_t blockStartOffset = d->pos_7C00 % SECTOR_SIZE_DECRYPTED;
	if (blockStartOffset != 0) {
		// Not a block boundary.
		// Read the end of the block.
		uint32_t read_sz = SECTOR_SIZE_DECRYPTED - blockStartOffset;
		if (size < (size_t)read_sz) {
			read_sz = (uint32_t)size;
		}

		// Read and decrypt the sector.
		uint32_t blockStart = (uint32_t)(d->pos_7C00 / SECTOR_SIZE_DECRYPTED);
		d->readSector(blockStart);

		// Copy data from the sector.
		memcpy(ptr8, &d->sector_buf[SECTOR_SIZE_DECRYPTED_OFFSET + blockStartOffset], read_sz);

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
		uint32_t blockStart = (uint32_t)(d->pos_7C00 / SECTOR_SIZE_DECRYPTED);
		d->readSector(blockStart);

		// Copy data from the sector.
		memcpy(ptr8, &d->sector_buf[SECTOR_SIZE_DECRYPTED_OFFSET], SECTOR_SIZE_DECRYPTED);
	}

	// Check if we still have data left. (not a full block)
	if (size > 0) {
		// Not a full block.

		// Read and decrypt the sector.
		assert(d->pos_7C00 % SECTOR_SIZE_DECRYPTED == 0);
		uint32_t blockEnd = (uint32_t)(d->pos_7C00 / SECTOR_SIZE_DECRYPTED);
		d->readSector(blockEnd);

		// Copy data from the sector.
		memcpy(ptr8, &d->sector_buf[SECTOR_SIZE_DECRYPTED_OFFSET], size);

		ret += size;
		d->pos_7C00 += size;
	}

	// Finished reading the data.
#else /* !ENABLE_DECRYPTION */
	// Decryption is not enabled.
	m_lastError = EIO;
	ret = 0;
#endif /* ENABLE_DECRYPTION */
	}

	return ret;
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int WiiPartition::seek(int64_t pos)
{
	RP_D(WiiPartition);
	assert(d->discReader != nullptr);
	assert(d->discReader->isOpen());
	if (!d->discReader ||  !d->discReader->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// Handle out-of-range cases.
	// TODO: How does POSIX behave?
	if (pos < 0)
		d->pos_7C00 = 0;
	else if (pos >= d->data_size)
		d->pos_7C00 = d->data_size;
	else
		d->pos_7C00 = pos;
	return 0;
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
int64_t WiiPartition::tell(void)
{
	RP_D(WiiPartition);
	assert(d->discReader != nullptr);
	assert(d->discReader->isOpen());
	if (!d->discReader ||  !d->discReader->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	return d->pos_7C00;
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
	return d->getEncKey();
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
