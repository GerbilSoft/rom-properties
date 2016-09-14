/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiPartition.hpp: Wii partition reader.                                 *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#ifdef ENABLE_DECRYPTION
#include "crypto/AesCipher.hpp"
#include "crypto/KeyManager.hpp"
#endif /* ENABLE_DECRYPTION */

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <memory>
using std::unique_ptr;

namespace LibRomData {

#define SECTOR_SIZE_ENCRYPTED 0x8000
#define SECTOR_SIZE_DECRYPTED 0x7C00
#define SECTOR_SIZE_DECRYPTED_OFFSET 0x400

class WiiPartitionPrivate {
	public:
		WiiPartitionPrivate(IDiscReader *discReader, int64_t partition_offset);
		~WiiPartitionPrivate();

	private:
		WiiPartitionPrivate(const WiiPartitionPrivate &other);
		WiiPartitionPrivate &operator=(const WiiPartitionPrivate &other);

	public:
		IDiscReader *discReader;
		int64_t partition_offset;	// Partition start offset.
		int64_t data_offset;		// Encrypted data start offset. (-1 == error)

		int64_t partition_size;		// Partition size, including header and hashes.
		int64_t data_size;		// Data size, excluding hashes.

		int64_t pos_7C00;		// Decrypted read position. (0x7C00 bytes out of 0x8000)

		// Partition header.
		RVL_PartitionHeader partitionHeader;
		uint8_t title_key[16];		// Decrypted title key.

		// Decrypted sector cache.
		// NOTE: Actual data starts at 0x400.
		// Hashes and the sector IV are stored first.
		uint32_t sector_num;				// Sector number.
		uint8_t sector_buf[SECTOR_SIZE_ENCRYPTED];	// Decrypted sector data.

		WiiPartition::EncInitStatus encInitStatus;
#ifdef ENABLE_DECRYPTION
		// AES cipher for the Common key.
		// - Index 0: rvl-common
		// - Index 1: rvl-korean
		// TODO: Dev keys?
		static AesCipher *aes_common[2];
		static int aes_common_refcnt;

		// AES cipher for this partition's title key.
		AesCipher *aes_title;

		/**
		 * Initialize decryption.
		 * @return EncInitStatus.
		 */
		WiiPartition::EncInitStatus initDecryption(void);
#endif

		/**
		 * Read and decrypt a sector.
		 * The decrypted sector is stored in sector_buf.
		 *
		 * @param sector_num Sector number. (address / 0x7C00)
		 * @return 0 on success; non-zero on error.
		 */
		int readSector(uint32_t sector_num);
};

/** WiiPartitionPrivate **/

AesCipher *WiiPartitionPrivate::aes_common[2] = {nullptr, nullptr};
int WiiPartitionPrivate::aes_common_refcnt = 0;

WiiPartitionPrivate::WiiPartitionPrivate(IDiscReader *discReader, int64_t partition_offset)
	: discReader(discReader)
	, partition_offset(partition_offset)
	, data_offset(-1)	// -1 == invalid
	, partition_size(-1)
	, data_size(-1)
	, pos_7C00(-1)
	, sector_num(-1)
#ifdef ENABLE_DECRYPTION
	, encInitStatus(WiiPartition::ENCINIT_UNKNOWN)
	, aes_title(nullptr)
#else /* !ENABLE_DECRYPTION */
	, encInitStatus(WiiPartition::ENCINIT_DISABLED)
#endif /* ENABLE_DECRYPTION */
{
	static_assert(sizeof(RVL_TimeLimit) == RVL_TimeLimit_SIZE,
		"sizeof(RVL_TimeLimit) is incorrect. (Should be 8)");
	static_assert(sizeof(RVL_Ticket) == RVL_Ticket_SIZE,
		"sizeof(RVL_Ticket) is incorrect. (Should be 0x2A4)");
	static_assert(sizeof(RVL_PartitionHeader) == RVL_PartitionHeader_SIZE,
		"sizeof(RVL_PartitionHeader) is incorrect. (Should be 0x20000)");

	// Increment the AES common key reference counter.
	// TODO: Atomic reference count?
	++aes_common_refcnt;

	if (!discReader->isOpen())
		return;

	// Read the partition header.
	if (discReader->seek(partition_offset) != 0)
		return;
	size_t size = discReader->read(&partitionHeader, sizeof(partitionHeader));
	if (size != sizeof(partitionHeader))
		return;

	// Make sure the signature type is correct.
	if (be32_to_cpu(partitionHeader.ticket.signature_type) != RVL_SIGNATURE_TYPE_RSA2048)
		return;

	// Save important data.
	data_offset     = (int64_t)be32_to_cpu(partitionHeader.data_offset) << 2;
	data_size       = (int64_t)be32_to_cpu(partitionHeader.data_size) << 2;
	partition_size  = data_size + ((int64_t)be32_to_cpu(partitionHeader.data_offset) << 2);

#ifdef ENABLE_DECRYPTION
	// Initialize decryption.
	// TODO: Only do this when required?
	initDecryption();
#endif
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
	KeyManager *keyManager = KeyManager::instance();

	// Initialize the common key cipher required for this disc.
	assert(partitionHeader.ticket.common_key_index <= 1);
	const uint8_t keyIdx = partitionHeader.ticket.common_key_index;
	if (keyIdx > 1) {
		// Invalid common key index.
		encInitStatus = WiiPartition::ENCINIT_INVALID_KEY_IDX;
		return encInitStatus;
	}

	// TODO: Mutex?
	if (!aes_common[keyIdx]) {
		// Initialize this key.
		// TODO: Dev keys?
		unique_ptr<AesCipher> cipher(new AesCipher());
		if (!cipher->isInit()) {
			// Error initializing the cipher.
			encInitStatus = WiiPartition::ENCINIT_CIPHER_ERROR;
			return encInitStatus;
		}

		// Get the common key.
		KeyManager::KeyData_t keyData;
		if (keyManager->get("rvl-common", &keyData) != 0) {
			// "rvl-common" key was not found.
			// TODO: NO_KEYFILE if the key file is missing?
			encInitStatus = WiiPartition::ENCINIT_MISSING_KEY;
			return encInitStatus;
		}

		// Load the common key. (CBC mode)
		int ret = cipher->setKey(keyData.key, keyData.length);
		ret |= cipher->setChainingMode(AesCipher::CM_CBC);
		if (ret != 0) {
			encInitStatus = WiiPartition::ENCINIT_CIPHER_ERROR;
			return encInitStatus;
		}

		// Save the cipher as the common instance.
		aes_common[keyIdx] = cipher.release();
	}

	// Initialize the title key AES cipher.
	unique_ptr<AesCipher> cipher(new AesCipher());
	if (!cipher->isInit()) {
		// Error initializing the cipher.
		encInitStatus = WiiPartition::ENCINIT_CIPHER_ERROR;
		return encInitStatus;
	}

	// Get the Wii common key.
	KeyManager::KeyData_t keyData;
	if (keyManager->get("rvl-common", &keyData) != 0) {
		// "rvl-common" key was not found.
		// TODO: NO_KEYFILE if the key file is missing?
		encInitStatus = WiiPartition::ENCINIT_MISSING_KEY;
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
	if (aes_common[keyIdx]->decrypt(title_key, sizeof(title_key), iv, sizeof(iv)) != sizeof(title_key)) {
		encInitStatus = WiiPartition::ENCINIT_CIPHER_ERROR;
		return encInitStatus;
	}

	// Load the title key. (CBC mode)
	if (cipher->setKey(title_key, sizeof(title_key)) != 0) {
		encInitStatus = WiiPartition::ENCINIT_CIPHER_ERROR;
		return encInitStatus;
	}
	if (cipher->setChainingMode(AesCipher::CM_CBC) != 0) {
		encInitStatus = WiiPartition::ENCINIT_CIPHER_ERROR;
		return encInitStatus;
	}

	// Cipher initialized.
	encInitStatus = WiiPartition::ENCINIT_OK;
	aes_title = cipher.release();

	/** TODO: This is debug code. **/

	// Read the first encrypted sector of the partition.
	int ret = discReader->seek(partition_offset + data_offset);
	if (ret != 0)
		return encInitStatus;
	size_t sz = discReader->read(sector_buf, sizeof(sector_buf));
	if (sz != SECTOR_SIZE_ENCRYPTED)
		return encInitStatus;

	// Decrypt the sector.
	if (aes_title->decrypt(&sector_buf[SECTOR_SIZE_DECRYPTED_OFFSET], SECTOR_SIZE_DECRYPTED,
	    &sector_buf[0x3D0], 16) != SECTOR_SIZE_DECRYPTED)
	{
		return encInitStatus;
	}

	// Read the first encrypted sector of the partition.
	ret = discReader->seek(partition_offset + data_offset);
	sz = discReader->read(sector_buf, sizeof(sector_buf));
	if (sz != SECTOR_SIZE_ENCRYPTED)
		return encInitStatus;

	// Decrypt the sector.
	if (aes_title->decrypt(&sector_buf[SECTOR_SIZE_DECRYPTED_OFFSET], SECTOR_SIZE_DECRYPTED,
	    &sector_buf[0x3D0], 16) != SECTOR_SIZE_DECRYPTED)
	{
		return encInitStatus;
	}

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
	}
#endif /* ENABLE_DECRYPTION */
}

/** WiiPartition **/

/**
 * Construct a WiiPartition with the specified IDiscReader.
 * NOTE: The IDiscReader *must* remain valid while this
 * WiiPartition is open.
 * @param discReader IDiscReader.
 * @param partition_offset Partition start offset.
 */
WiiPartition::WiiPartition(IDiscReader *discReader, int64_t partition_offset)
	: d(new WiiPartitionPrivate(discReader, partition_offset))
{ }

WiiPartition::~WiiPartition()
{
	delete d;
}

/** IDiscReader **/

/**
 * Is the partition open?
 * This usually only returns false if an error occurred.
 * @return True if the partition is open; false if it isn't.
 */
bool WiiPartition::isOpen(void) const
{
	return (d->discReader && d->discReader->isOpen());
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t WiiPartition::read(void *ptr, size_t size)
{
#ifdef ENABLE_DECRYPTION
	// TODO
	return 0;
#else /* !ENABLE_DECRYPTION */
	// Decryption is not enabled in this build.
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
	assert(d->discReader != nullptr);
	assert(d->discReader->isOpen());
	if (!d->discReader || !d->discReader->isOpen())
		return -1;

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
 * Seek to the beginning of the partition.
 */
void WiiPartition::rewind(void)
{
	seek(0);
}

/**
 * Get the data size.
 * This size does not include the partition header,
 * and it's adjusted to exclude hashes.
 * @return Data size, or -1 on error.
 */
int64_t WiiPartition::size(void) const
{
	return d->data_size;
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
int64_t WiiPartition::partition_size(void) const
{
	return d->partition_size;
}

/** WiiPartition **/

/**
 * Encryption initialization status.
 * @return Encryption initialization status.
 */
WiiPartition::EncInitStatus WiiPartition::encInitStatus(void) const
{
	return d->encInitStatus;
}

}
