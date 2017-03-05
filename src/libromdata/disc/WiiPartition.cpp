/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiPartition.hpp: Wii partition reader.                                 *
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

#include "WiiPartition.hpp"
#include "config.libromdata.h"

#include "byteswap.h"
#include "gcn_structs.h"
#include "IDiscReader.hpp"
#include "GcnFst.hpp"

#ifdef ENABLE_DECRYPTION
#include "crypto/AesCipherFactory.hpp"
#include "crypto/IAesCipher.hpp"
#include "crypto/KeyManager.hpp"
#endif /* ENABLE_DECRYPTION */

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
		WiiPartitionPrivate(WiiPartition *q, IDiscReader *discReader, int64_t partition_offset);
		virtual ~WiiPartitionPrivate();

	private:
		typedef GcnPartitionPrivate super;
		RP_DISABLE_COPY(WiiPartitionPrivate)

	public:
		// Partition header.
		RVL_PartitionHeader partitionHeader;

		// Encryption status.
		WiiPartition::EncInitStatus encInitStatus;

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
#ifdef ENABLE_DECRYPTION
		// AES cipher for the Common key.
		// - Index 0: rvl-common (retail)
		// - Index 1: rvl-korean (Korean)
		// - Index 2: rvt-debug  (debug)
		// TODO: Dev keys?
		static IAesCipher *aes_common[3];
		static int aes_common_refcnt;

		// AES cipher for this partition's title key.
		IAesCipher *aes_title;
		// Decrypted title key.
		uint8_t title_key[16];

		/**
		 * Initialize decryption.
		 * @return EncInitStatus.
		 */
		WiiPartition::EncInitStatus initDecryption(void);

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
#endif
};

/** WiiPartitionPrivate **/

#ifdef ENABLE_DECRYPTION
IAesCipher *WiiPartitionPrivate::aes_common[3] = {nullptr, nullptr, nullptr};
int WiiPartitionPrivate::aes_common_refcnt = 0;
#endif /* ENABLE_DECRYPTION */

WiiPartitionPrivate::WiiPartitionPrivate(WiiPartition *q, IDiscReader *discReader, int64_t partition_offset)
	: super(q, discReader, partition_offset, 2)
#ifdef ENABLE_DECRYPTION
	, encInitStatus(WiiPartition::ENCINIT_UNKNOWN)
	, m_encKey(WiiPartition::ENCKEY_UNKNOWN)
	, aes_title(nullptr)
	, pos_7C00(-1)
	, sector_num(~0)
#else /* !ENABLE_DECRYPTION */
	, encInitStatus(WiiPartition::ENCINIT_DISABLED)
	, m_encKey(WiiPartition::ENCKEY_UNKNOWN)
#endif /* ENABLE_DECRYPTION */
{
	// Clear data set by GcnPartition in case the
	// partition headers can't be read.
	data_offset = -1;
	data_size = -1;
	partition_size = -1;

	// Clear the partition header struct.
	memset(&partitionHeader, 0, sizeof(partitionHeader));

#ifdef ENABLE_DECRYPTION
	// Increment the AES common key reference counter.
	// TODO: Atomic reference count?
	++aes_common_refcnt;
#endif /* ENABLE_DECRYPTION */

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
	if (be32_to_cpu(partitionHeader.ticket.signature_type) != RVL_SIGNATURE_TYPE_RSA2048) {
		// TODO: Better error?
		q->m_lastError = EIO;
		return;
	}

	// Save important data.
	data_offset     = (int64_t)be32_to_cpu(partitionHeader.data_offset) << 2;
	data_size       = (int64_t)be32_to_cpu(partitionHeader.data_size) << 2;
	partition_size  = data_size + ((int64_t)be32_to_cpu(partitionHeader.data_offset) << 2);
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
 * @return EncInitStatus.
 */
WiiPartition::EncInitStatus WiiPartitionPrivate::initDecryption(void)
{
	if (encInitStatus != WiiPartition::ENCINIT_UNKNOWN) {
		// Decryption has already been initialized.
		return encInitStatus;
	}

	// If decryption is enabled, we can load the key and enable reading.
	// Otherwise, we can only get the partition size information.

	// Initialize the Key Manager.
	KeyManager keyManager;

	// Determine the required encryption key.
	const WiiPartition::EncKey encKey = getEncKey();
	if (encKey <= WiiPartition::ENCKEY_UNKNOWN) {
		// Invalid encryption key.
		encInitStatus = WiiPartition::ENCINIT_INVALID_KEY_IDX;
		return encInitStatus;
	}

	// Determine the encryption key name.
	const char *key_name;
	switch (encKey) {
		case WiiPartition::ENCKEY_COMMON:
			// Wii common key.
			key_name = "rvl-common";
			break;
		case WiiPartition::ENCKEY_KOREAN:
			// Korean key.
			key_name = "rvl-korean";
			break;
		case WiiPartition::ENCKEY_DEBUG:
			// Debug key. (RVT-R)
			key_name = "rvt-debug";
			break;
		default:
			// Unknown key...
			encInitStatus = WiiPartition::ENCINIT_INVALID_KEY_IDX;
			return encInitStatus;
	}

	// TODO: Mutex?
	if (!aes_common[encKey]) {
		// Initialize this key.
		// TODO: Dev keys?
		unique_ptr<IAesCipher> cipher(AesCipherFactory::getInstance());
		if (!cipher || !cipher->isInit()) {
			// Error initializing the cipher.
			encInitStatus = WiiPartition::ENCINIT_CIPHER_ERROR;
			return encInitStatus;
		}

		// Get the common key.
		KeyManager::KeyData_t keyData;
		if (keyManager.get(key_name, &keyData) != 0) {
			// Common key was not found.
			if (keyManager.areKeysLoaded()) {
				// Keys were loaded, but this key is missing.
				encInitStatus = WiiPartition::ENCINIT_MISSING_KEY;
			} else {
				// Keys were not loaded. keys.conf is missing.
				encInitStatus = WiiPartition::ENCINIT_NO_KEYFILE;
			}
			return encInitStatus;
		}

		// Load the common key. (CBC mode)
		int ret = cipher->setKey(keyData.key, keyData.length);
		ret |= cipher->setChainingMode(IAesCipher::CM_CBC);
		if (ret != 0) {
			encInitStatus = WiiPartition::ENCINIT_CIPHER_ERROR;
			return encInitStatus;
		}

		// Save the cipher as the common instance.
		aes_common[encKey] = cipher.release();
	}

	// Initialize the title key AES cipher.
	unique_ptr<IAesCipher> cipher(AesCipherFactory::getInstance());
	if (!cipher || !cipher->isInit()) {
		// Error initializing the cipher.
		encInitStatus = WiiPartition::ENCINIT_CIPHER_ERROR;
		return encInitStatus;
	}

	// Get the IV.
	// First 8 bytes are the title ID.
	// Second 8 bytes are all 0.
	uint8_t iv[16];
	memcpy(iv, partitionHeader.ticket.title_id, 8);
	memset(&iv[8], 0, 8);

	// Decrypt the title key.
	memcpy(title_key, partitionHeader.ticket.enc_title_key, sizeof(title_key));
	if (aes_common[encKey]->decrypt(title_key, sizeof(title_key), iv, sizeof(iv)) != sizeof(title_key)) {
		encInitStatus = WiiPartition::ENCINIT_CIPHER_ERROR;
		return encInitStatus;
	}

	// Load the title key. (CBC mode)
	if (cipher->setKey(title_key, sizeof(title_key)) != 0) {
		encInitStatus = WiiPartition::ENCINIT_CIPHER_ERROR;
		return encInitStatus;
	}
	if (cipher->setChainingMode(IAesCipher::CM_CBC) != 0) {
		encInitStatus = WiiPartition::ENCINIT_CIPHER_ERROR;
		return encInitStatus;
	}

	// readSector() needs aes_title.
	if (aes_title) {
		delete aes_title;
	}
	aes_title = cipher.release();

	// Read sector 0, which contains a disc header.
	// NOTE: readSector() doesn't check encInitStatus.
	if (readSector(0) != 0) {
		// Error reading sector 0.
		delete aes_title;
		aes_title = nullptr;
		encInitStatus = WiiPartition::ENCINIT_CIPHER_ERROR;
		return encInitStatus;
	}

	// Verify that this is a Wii partition.
	// If it isn't, the key is probably wrong.
	const GCN_DiscHeader *discHeader =
		reinterpret_cast<const GCN_DiscHeader*>(&sector_buf[SECTOR_SIZE_DECRYPTED_OFFSET]);
	if (be32_to_cpu(discHeader->magic_wii) != WII_MAGIC) {
		// Invalid disc header.
		encInitStatus = WiiPartition::ENCINIT_INCORRECT_KEY;
		return encInitStatus;
	}

	// Cipher initialized.
	encInitStatus = WiiPartition::ENCINIT_OK;
	return encInitStatus;
}
#endif /* ENABLE_DECRYPTION */

WiiPartitionPrivate::~WiiPartitionPrivate()
{
#ifdef ENABLE_DECRYPTION
	delete aes_title;

	// Decrement the reference counter for the common keys.
	// TODO: Atomic reference count?
	if (--aes_common_refcnt == 0) {
		// Delete the common keys.
		delete aes_common[0];
		aes_common[0] = nullptr;
		delete aes_common[1];
		aes_common[1] = nullptr;
		delete aes_common[2];
		aes_common[2] = nullptr;
	}
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

	// NOTE: This function doesn't check encInitStatus,
	// since it's called by initDecryption() before
	// encInitStatus is set.

	// Read the first encrypted sector of the partition.
	int64_t sector_addr = partition_offset + data_offset;
	sector_addr += (sector_num * SECTOR_SIZE_ENCRYPTED);

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

	// Decrypt the sector.
	if (aes_title->decrypt(&sector_buf[SECTOR_SIZE_DECRYPTED_OFFSET], SECTOR_SIZE_DECRYPTED,
	    &sector_buf[0x3D0], 16) != SECTOR_SIZE_DECRYPTED)
	{
		// sector_buf may be invalid.
		this->sector_num = ~0;
		q->m_lastError = EIO;
		return -1;
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
WiiPartition::WiiPartition(IDiscReader *discReader, int64_t partition_offset)
	: super(new WiiPartitionPrivate(this, discReader, partition_offset))
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

#ifdef ENABLE_DECRYPTION
	// Make sure decryption is initialized.
	switch (d->encInitStatus) {
		case ENCINIT_UNKNOWN:
			// Attempt to initialize decryption.
			if (d->initDecryption() != WiiPartition::ENCINIT_OK) {
				// Decryption could not be initialized.
				// TODO: Better error?
				m_lastError = EIO;
				return -m_lastError;
			}
			break;

		case ENCINIT_OK:
			// Decryption is initialized.
			break;

		default:
			// Decryption failed to initialize.
			// TODO: Better error?
			m_lastError = EIO;
			return -m_lastError;
	}

	uint8_t *ptr8 = static_cast<uint8_t*>(ptr);
	size_t ret = 0;

	// Are we already at the end of the file?
	if (d->pos_7C00 >= d->data_size)
		return 0;

	// Make sure d->pos_7C00 + size <= d->data_size.
	// If it isn't, we'll do a short read.
	if (d->pos_7C00 + (int64_t)size >= d->data_size) {
		size = (size_t)(d->data_size - d->pos_7C00);
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
	return ret;
#else /* !ENABLE_DECRYPTION */
	// Decryption is not enabled.
	((void)ptr);
	((void)size);
	m_lastError = EIO;
	return 0;
#endif /* ENABLE_DECRYPTION */
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

#ifdef ENABLE_DECRYPTION
	// Handle out-of-range cases.
	// TODO: How does POSIX behave?
	if (pos < 0)
		d->pos_7C00 = 0;
	else if (pos >= d->data_size)
		d->pos_7C00 = d->data_size;
	else
		d->pos_7C00 = pos;
	return 0;
#else /* !ENABLE_DECRYPTION */
	// Decryption is not enabled.
	((void)pos);
	m_lastError = EIO;
	return -1;
#endif /* ENABLE_DECRYPTION */
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

#ifdef ENABLE_DECRYPTION
	return d->pos_7C00;
#else /* !ENABLE_DECRYPTION */
	// Decryption is not enabled.
	m_lastError = EIO;
	return -1;
#endif /* ENABLE_DECRYPTION */
}

/** WiiPartition **/

/**
 * Encryption initialization status.
 * @return Encryption initialization status.
 */
WiiPartition::EncInitStatus WiiPartition::encInitStatus(void) const
{
	// TODO: Errors?
	RP_D(const WiiPartition);
	return d->encInitStatus;
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

}
