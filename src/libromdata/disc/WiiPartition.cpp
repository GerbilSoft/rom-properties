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
#include "GcnFst.hpp"

#ifdef ENABLE_DECRYPTION
#include "crypto/AesCipher.hpp"
#include "crypto/KeyManager.hpp"
#endif /* ENABLE_DECRYPTION */

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
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
		WiiPartitionPrivate(WiiPartition *q, IDiscReader *discReader, int64_t partition_offset);
		~WiiPartitionPrivate();

	private:
		WiiPartitionPrivate(const WiiPartitionPrivate &other);
		WiiPartitionPrivate &operator=(const WiiPartitionPrivate &other);
	private:
		WiiPartition *const q;

	public:
		int lastError;

		IDiscReader *discReader;
		int64_t partition_offset;	// Partition start offset.
		int64_t data_offset;		// Encrypted data start offset. (-1 == error)

		int64_t partition_size;		// Partition size, including header and hashes.
		int64_t data_size;		// Data size, excluding hashes.

		int64_t pos_7C00;		// Decrypted read position. (0x7C00 bytes out of 0x8000)

		// Partition header.
		RVL_PartitionHeader partitionHeader;
		uint8_t title_key[16];		// Decrypted title key.

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
		 * @return 0 on success; non-zero on error.
		 */
		int readSector(uint32_t sector_num);

		// Filesystem table.
		GcnFst *fst;
		GCN_FST_Info fstInfo;

		/**
		 * Load the FST.
		 * @return 0 on success; non-zero on error.
		 */
		int loadFst(void);
#endif
};

/** WiiPartitionPrivate **/

AesCipher *WiiPartitionPrivate::aes_common[2] = {nullptr, nullptr};
int WiiPartitionPrivate::aes_common_refcnt = 0;

WiiPartitionPrivate::WiiPartitionPrivate(WiiPartition *q, IDiscReader *discReader, int64_t partition_offset)
	: q(q)
	, lastError(0)
	, discReader(discReader)
	, partition_offset(partition_offset)
	, data_offset(-1)	// -1 == invalid
	, partition_size(-1)
	, data_size(-1)
	, pos_7C00(-1)
#ifdef ENABLE_DECRYPTION
	, encInitStatus(WiiPartition::ENCINIT_UNKNOWN)
	, aes_title(nullptr)
	, sector_num(~0)
	, fst(nullptr)
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
	static_assert(sizeof(GCN_FST_Info) == GCN_FST_Info_SIZE,
		"sizeof(GCN_FST_Info) is incorrect. (Should be 12)");

	// Increment the AES common key reference counter.
	// TODO: Atomic reference count?
	++aes_common_refcnt;

	if (!discReader->isOpen()) {
		lastError = discReader->lastError();
		return;
	}

	// Read the partition header.
	if (discReader->seek(partition_offset) != 0) {
		lastError = discReader->lastError();
		return;
	}
	size_t size = discReader->read(&partitionHeader, sizeof(partitionHeader));
	if (size != sizeof(partitionHeader)) {
		lastError = EIO;
		return;
	}

	// Make sure the signature type is correct.
	if (be32_to_cpu(partitionHeader.ticket.signature_type) != RVL_SIGNATURE_TYPE_RSA2048) {
		// TODO: Better error?
		lastError = EIO;
		return;
	}

	// Save important data.
	data_offset     = (int64_t)be32_to_cpu(partitionHeader.data_offset) << 2;
	data_size       = (int64_t)be32_to_cpu(partitionHeader.data_size) << 2;
	partition_size  = data_size + ((int64_t)be32_to_cpu(partitionHeader.data_offset) << 2);
	pos_7C00	= 0;

	// Encryption will not be initialized until
	// read() is called.
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
	return encInitStatus;
}
#endif /* ENABLE_DECRYPTION */

WiiPartitionPrivate::~WiiPartitionPrivate()
{
#ifdef ENABLE_DECRYPTION
	delete fst;
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

#ifdef ENABLE_DECRYPTION
/**
 * Read and decrypt a sector.
 * The decrypted sector is stored in sector_buf.
 *
 * @param sector_num Sector number. (address / 0x7C00)
 * @return 0 on success; non-zero on error.
 */
int WiiPartitionPrivate::readSector(uint32_t sector_num)
{
	if (this->sector_num == sector_num) {
		// Sector is already in memory.
		return 0;
	}

	// Read the first encrypted sector of the partition.
	int64_t sector_addr = partition_offset + data_offset;
	sector_addr += (sector_num * SECTOR_SIZE_ENCRYPTED);

	int ret = discReader->seek(sector_addr);
	if (ret != 0) {
		lastError = discReader->lastError();
		return ret;
	}

	size_t sz = discReader->read(sector_buf, sizeof(sector_buf));
	if (sz != SECTOR_SIZE_ENCRYPTED) {
		// sector_buf may be invalid.
		this->sector_num = ~0;
		lastError = EIO;
		return -1;
	}

	// Decrypt the sector.
	if (aes_title->decrypt(&sector_buf[SECTOR_SIZE_DECRYPTED_OFFSET], SECTOR_SIZE_DECRYPTED,
	    &sector_buf[0x3D0], 16) != SECTOR_SIZE_DECRYPTED)
	{
		// sector_buf may be invalid.
		this->sector_num = ~0;
		lastError = EIO;
		return -1;
	}

	// Sector read and decrypted.
	this->sector_num = sector_num;
	return 0;
}
#endif /* ENABLE_DECRYPTION */

/**
 * Load the FST.
 * @return 0 on success; non-zero on error.
 */
int WiiPartitionPrivate::loadFst(void)
{
	if (fst) {
		// FST is already loaded.
		return 0;
	}

#ifdef ENABLE_DECRYPTION
	switch (encInitStatus) {
		case WiiPartition::ENCINIT_UNKNOWN:
			// Attempt to initialize decryption.
			if (initDecryption() != WiiPartition::ENCINIT_OK) {
				// Decryption could not be initialized.
				// TODO: Better error?
				lastError = ENOSYS;
				return 0;
			}
			break;

		case WiiPartition::ENCINIT_OK:
			// Decryption is initialized.
			break;

		default:
			// Decryption failed to initialize.
			return 0;
	}

	// Load the FST info.
	GCN_FST_Info fstInfo;
	int ret = q->seek(GCN_FST_Info_ADDRESS);
	if (ret != 0) {
		// Seek failed.
		return -lastError;
	}

	size_t size = q->read(&fstInfo, sizeof(fstInfo));
	if (size != sizeof(fstInfo)) {
		// fstInfo read failed.
		lastError = EIO;
		return -lastError;
	}

#if SYS_BYTEORDER != SYS_BIG_ENDIAN
	// Byteswap fstInfo.
	fstInfo.dol_offset	= be32_to_cpu(fstInfo.dol_offset);
	fstInfo.fst_offset	= be32_to_cpu(fstInfo.fst_offset);
	fstInfo.fst_size	= be32_to_cpu(fstInfo.fst_size);
	fstInfo.fst_max_size	= be32_to_cpu(fstInfo.fst_max_size);
#endif /* SYS_BYTEORDER != SYS_BIG_ENDIAN */

	// NOTE: fstInfo's values are all uint34_rsh2_t on Wii.
	if (fstInfo.fst_size > (1048576>>2) || fstInfo.fst_max_size > (1048576>>2)) {
		// Sanity check: FST larger than 1 MB is invalid.
		// TODO: What is the actual largest FST?
		lastError = EIO;
		return -lastError;
	} else if (fstInfo.fst_size > fstInfo.fst_max_size) {
		// FST is invalid.
		lastError = EIO;
		return -lastError;
	}

	// Seek to the beginning of the FST.
	ret = q->seek((int64_t)fstInfo.fst_offset << 2);
	if (ret != 0) {
		// Seek failed.
		return -lastError;
	}

	// Read the FST.
	// TODO: Eliminate the extra copy?
	size_t fstData_len = (size_t)fstInfo.fst_size << 2;
	uint8_t *fstData = reinterpret_cast<uint8_t*>(malloc(fstData_len));
	if (!fstData) {
		// malloc() failed.
		lastError = ENOMEM;
		return -lastError;
	}
	size = q->read(fstData, fstData_len);
	if (size != fstData_len) {
		// Short read.
		free(fstData);
		lastError = -EIO;
		return -lastError;
	}

	// Create the GcnFst.
	fst = new GcnFst(fstData, fstData_len, 2);
	free(fstData);	// TODO: Eliminate the extra copy?
	return 0;
#else /* !ENABLE_DECRYPTION */
	// Decryption is required in order to load the FST.
	return -EIO;
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
	: d(new WiiPartitionPrivate(this, discReader, partition_offset))
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
 * Get the last error.
 * @return Last POSIX error, or 0 if no error.
 */
int WiiPartition::lastError(void) const
{
	return d->lastError;
}

/**
 * Clear the last error.
 */
void WiiPartition::clearError(void)
{
	d->lastError = 0;
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t WiiPartition::read(void *ptr, size_t size)
{
	assert(d->discReader != nullptr);
	assert(d->discReader->isOpen());
	if (!d->discReader || !d->discReader->isOpen()) {
		d->lastError = EBADF;
		return 0;
	}

#ifdef ENABLE_DECRYPTION
	switch (d->encInitStatus) {
		case ENCINIT_UNKNOWN:
			// Attempt to initialize decryption.
			if (d->initDecryption() != ENCINIT_OK) {
				// Decryption could not be initialized.
				// TODO: Better error?
				d->lastError = ENOSYS;
				return 0;
			}
			break;

		case ENCINIT_OK:
			// Decryption is initialized.
			break;

		default:
			// Decryption failed to initialize.
			return 0;
	}

	uint8_t *ptr8 = reinterpret_cast<uint8_t*>(ptr);
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
		if (size < read_sz)
			read_sz = size;

		// Read and decrypt the sector.
		uint32_t blockStart = (d->pos_7C00 / SECTOR_SIZE_DECRYPTED);
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
		uint32_t blockStart = (d->pos_7C00 / SECTOR_SIZE_DECRYPTED);
		d->readSector(blockStart);

		// Copy data from the sector.
		memcpy(ptr8, &d->sector_buf[SECTOR_SIZE_DECRYPTED_OFFSET], SECTOR_SIZE_DECRYPTED);
	}

	// Check if we still have data left. (not a full block)
	if (size > 0) {
		// Not a full block.

		// Read and decrypt the sector.
		assert(d->pos_7C00 % SECTOR_SIZE_DECRYPTED == 0);
		uint32_t blockEnd = (d->pos_7C00 / SECTOR_SIZE_DECRYPTED);
		d->readSector(blockEnd);

		// Copy data from the sector.
		memcpy(ptr8, &d->sector_buf[SECTOR_SIZE_DECRYPTED_OFFSET], size);

		ret += size;
		d->pos_7C00 += size;
	}

	// Finished reading the data.
	return ret;
#else /* !ENABLE_DECRYPTION */
	// Decryption is not enabled in this build.
	d->lastError = ENOSYS;
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
	if (!d->discReader ||  !d->discReader->isOpen()) {
		d->lastError = EBADF;
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
	// TODO: Errors?
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
	// TODO: Errors?
	return d->partition_size;
}

/** WiiPartition **/

/**
 * Encryption initialization status.
 * @return Encryption initialization status.
 */
WiiPartition::EncInitStatus WiiPartition::encInitStatus(void) const
{
	// TODO: Errors?
	return d->encInitStatus;
}

		/** GcnFst wrapper functions. **/

/**
 * Open a directory.
 * @param path	[in] Directory path. [TODO; always reads "/" right now.]
 * @return FstDir*, or nullptr on error.
 */
GcnFst::FstDir *WiiPartition::opendir(const rp_char *path)
{
#ifdef ENABLE_DECRYPTION
	if (!d->fst) {
		// FST isn't loaded.
		if (d->loadFst() != 0) {
			// FST load failed.
			// TODO: Errors?
			return nullptr;
		}
	}

	return d->fst->opendir(path);
#else
	// TODO: Errors?
	return nullptr;
#endif
}

/**
 * Read a directory entry.
 * @param dirp FstDir pointer.
 * @return FstDirEntry*, or nullptr if end of directory or on error.
 * (TODO: Add lastError()?)
 */
GcnFst::FstDirEntry *WiiPartition::readdir(GcnFst::FstDir *dirp)
{
#ifdef ENABLE_DECRYPTION
	if (!d->fst) {
		// TODO: Errors?
		return nullptr;
	}

	return d->fst->readdir(dirp);
#else
	// TODO: Errors?
	return nullptr;
#endif
}

/**
 * Close an opened directory.
 * @param dirp FstDir pointer.
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiPartition::closedir(GcnFst::FstDir *dirp)
{
#ifdef ENABLE_DECRYPTION
	if (!d->fst) {
		// TODO: Errors?
		return -EBADF;
	}

	return d->fst->closedir(dirp);
#else
	// TODO: Errors?
	return -EBADF;
#endif
}

}
