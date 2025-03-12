/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUH3Reader.hpp: Wii U H3 content reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// TODO: Hash validation?

#include "stdafx.h"
#include "config.librpbase.h"

#include "WiiUH3Reader.hpp"
#include "Console/wiiu_structs.h"

// librpbase
#ifdef ENABLE_DECRYPTION
#  include "librpbase/crypto/IAesCipher.hpp"
#  include "librpbase/crypto/AesCipherFactory.hpp"
#endif /* ENABLE_DECRYPTION */
using namespace LibRpBase;
using namespace LibRpFile;

#ifdef ENABLE_DECRYPTION
using std::array;
using std::unique_ptr;
#endif /* ENABLE_DECRYPTION */

namespace LibRomData {

class WiiUH3ReaderPrivate final
{
public:
	WiiUH3ReaderPrivate(WiiUH3Reader *q, const uint8_t *pKey, size_t keyLen);

private:
	RP_DISABLE_COPY(WiiUH3ReaderPrivate)
	WiiUH3Reader *const q_ptr;

public:
	// Decrypted read position (0xFC00 bytes out of 0x10000)
	off64_t pos_FC00;

	off64_t partition_size;		// Partition size, including header and hashes.
	off64_t data_size;		// Data size, excluding hashes.

	// Decrypted sector cache
	WUP_H3_Content_Block sector_buf;
	uint32_t sector_num;	// Sector number

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
	// AES cipher for this content file's encryption key
	unique_ptr<IAesCipher> cipher;
#endif
};

/** WiiUH3ReaderPrivate **/

WiiUH3ReaderPrivate::WiiUH3ReaderPrivate(WiiUH3Reader *q, const uint8_t *pKey, size_t keyLen)
	: q_ptr(q)
	, pos_FC00(-1)
	, partition_size(0)
	, data_size(0)
	, sector_num(~0)
{
	// Key must be 128-bit.
	assert(pKey != nullptr);
	assert(keyLen == 16);
	if (!pKey || keyLen != 16) {
		q->m_lastError = EINVAL;
		q->m_file.reset();
		return;
	}

#ifdef ENABLE_DECRYPTION
	// Initialize the cipher.
	cipher.reset(AesCipherFactory::create());
	if (!cipher || !cipher->isInit()) {
		// Error initializing the cipher.
		cipher.reset();
		q->m_lastError = EIO;
		q->m_file.reset();
		return;
	}

	// Set parameters.
	// NOTE: We don't need to save the key separately.
	int ret = cipher->setKey(pKey, keyLen);
	ret |= cipher->setChainingMode(IAesCipher::ChainingMode::CBC);
	if (ret != 0) {
		// Error initializing the cipher.
		cipher.reset();
		q->m_lastError = EIO;
		q->m_file.reset();
		return;
	}

	// Key is loaded.
#else /* !ENABLE_DECRYPTION */
	// FIXME: Some sort of error?
	q->m_lastError = ENOTSUP;
	q->m_file.reset();
#endif /* ENABLE_DECRYPTION */
}

WiiUH3Reader::~WiiUH3Reader()
{
	delete d_ptr;
}

/**
 * Read and decrypt a sector.
 * The decrypted sector is stored in sector_buf.
 *
 * @param sector_num Sector number. (address / 0x7C00)
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiUH3ReaderPrivate::readSector(uint32_t sector_num)
{
	if (this->sector_num == sector_num) {
		// Sector is already in memory.
		return 0;
	}

	// NOTE: This function doesn't check verifyResult,
	// since it's called by initDecryption() before
	// verifyResult is set.
	off64_t sector_addr = (static_cast<off64_t>(sector_num) * WUP_H3_SECTOR_SIZE_ENCRYPTED);

	RP_Q(WiiUH3Reader);
	int ret = q->m_file->seek(sector_addr);
	if (ret != 0) {
		q->m_lastError = q->m_file->lastError();
		return ret;
	}

	size_t sz = q->m_file->read(&sector_buf, sizeof(sector_buf));
	if (sz != sizeof(sector_buf)) {
		// Read failed.
		// NOTE: sector_buf may be invalid.
		this->sector_num = ~0;
		q->m_lastError = EIO;
		return -1;
	}

#ifdef ENABLE_DECRYPTION
	if (!cipher) {
		// Cipher was not initialized...
		q->m_lastError = EIO;
		return -1;
	}
	// Decrypt the hashes. (IV is zero)
	array<uint8_t, 16> iv;
	iv.fill(0);
	size_t size = cipher->decrypt(reinterpret_cast<uint8_t*>(&sector_buf.hashes), sizeof(sector_buf.hashes), iv.data(), iv.size());
	if (size != sizeof(sector_buf.hashes)) {
		// Decryption failed.
		// NOTE: sector_buf may be invalid.
		this->sector_num = ~0;
		q->m_lastError = EIO;
		return -1;
	}

	// Decrypt the sector. (IV is hashes.h0[sector_num % 16].)
	size = cipher->decrypt(sector_buf.data, sizeof(sector_buf.data), sector_buf.hashes.h0[sector_num % 16], 16);
	if (size != WUP_H3_SECTOR_SIZE_DECRYPTED) {
		// Decryption failed.
		// NOTE: sector_buf may be invalid.
		this->sector_num = ~0;
		q->m_lastError = EIO;
		return -1;
	}
#endif /* ENABLE_DECRYPTION */

	// Sector read and decrypted.
	this->sector_num = sector_num;
	return 0;
}

/** WiiUH3Reader **/

/**
 * Construct a WiiUH3Reader with the specified IDiscReader.
 * @param file		[in] IRpFile
 * @param pKey		[in] Encryption key
 * @param keyLen	[in] Length of pKey  (should be 16)
 */
WiiUH3Reader::WiiUH3Reader(const IRpFilePtr &file, const uint8_t *pKey, size_t keyLen)
	: super(file)
	, d_ptr(new WiiUH3ReaderPrivate(this, pKey, keyLen))
{
	// q->m_lastError is handled by GcnPartitionPrivate's constructor.
	if (!m_file || !m_file->isOpen()) {
		m_file.reset();
		return;
	}

	// Partition size is the entire file.
	// NOTE: Need to convert from ENC size blocks to DEC size blocks.
	off64_t fileSize = m_file->size();
	assert(fileSize % WUP_H3_SECTOR_SIZE_ENCRYPTED == 0);

	d_ptr->partition_size = fileSize;
	d_ptr->data_size = (fileSize / WUP_H3_SECTOR_SIZE_ENCRYPTED) * WUP_H3_SECTOR_SIZE_DECRYPTED;
	d_ptr->pos_FC00 = 0;

	// Encryption will not be initialized until read() is called.
}

/** IDiscReader **/

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t WiiUH3Reader::read(void *ptr, size_t size)
{
	RP_D(WiiUH3Reader);
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
	if (d->pos_FC00 >= d->data_size)
		return 0;

	// Make sure d->pos_FC00 + size <= d->data_size.
	// If it isn't, we'll do a short read.
	if (d->pos_FC00 + static_cast<off64_t>(size) >= d->data_size) {
		size = static_cast<size_t>(d->data_size - d->pos_FC00);
	}

	// Check if we're not starting on a block boundary.
	const uint32_t blockStartOffset = d->pos_FC00 % WUP_H3_SECTOR_SIZE_DECRYPTED;
	if (blockStartOffset != 0) {
		// Not a block boundary.
		// Read the end of the block.
		uint32_t read_sz = WUP_H3_SECTOR_SIZE_DECRYPTED - blockStartOffset;
		if (size < static_cast<size_t>(read_sz)) {
			read_sz = static_cast<uint32_t>(size);
		}

		// Read and decrypt the sector.
		const uint32_t blockStart = static_cast<uint32_t>(d->pos_FC00 / WUP_H3_SECTOR_SIZE_DECRYPTED);
		d->readSector(blockStart);

		// Copy data from the sector.
		memcpy(ptr8, &d->sector_buf.data[blockStartOffset], read_sz);

		// Starting block read.
		size -= read_sz;
		ptr8 += read_sz;
		ret += read_sz;
		d->pos_FC00 += read_sz;
	}

	// Read entire blocks.
	for (; size >= WUP_H3_SECTOR_SIZE_DECRYPTED;
	     size -= WUP_H3_SECTOR_SIZE_DECRYPTED, ptr8 += WUP_H3_SECTOR_SIZE_DECRYPTED,
	     ret += WUP_H3_SECTOR_SIZE_DECRYPTED, d->pos_FC00 += WUP_H3_SECTOR_SIZE_DECRYPTED)
	{
		assert(d->pos_FC00 % WUP_H3_SECTOR_SIZE_DECRYPTED == 0);

		// Read and decrypt the sector.
		const uint32_t blockStart = static_cast<uint32_t>(d->pos_FC00 / WUP_H3_SECTOR_SIZE_DECRYPTED);
		d->readSector(blockStart);

		// Copy data from the sector.
		memcpy(ptr8, d->sector_buf.data, WUP_H3_SECTOR_SIZE_DECRYPTED);
	}

	// Check if we still have data left. (not a full block)
	if (size > 0) {
		// Not a full block.

		// Read and decrypt the sector.
		assert(d->pos_FC00 % WUP_H3_SECTOR_SIZE_DECRYPTED == 0);
		const uint32_t blockEnd = static_cast<uint32_t>(d->pos_FC00 / WUP_H3_SECTOR_SIZE_DECRYPTED);
		d->readSector(blockEnd);

		// Copy data from the sector.
		memcpy(ptr8, d->sector_buf.data, size);

		ret += size;
		d->pos_FC00 += size;
	}

	// Finished reading the data.
	return ret;
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int WiiUH3Reader::seek(off64_t pos)
{
	RP_D(WiiUH3Reader);
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
		d->pos_FC00 = d->data_size;
	} else {
		d->pos_FC00 = pos;
	}
	return 0;
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
off64_t WiiUH3Reader::tell(void)
{
	RP_D(const WiiUH3Reader);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	if (!m_file ||  !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	return d->pos_FC00;
}

/**
 * Get the data size.
 * This size does not include hashes.
 * @return Data size, or -1 on error.
 */
off64_t WiiUH3Reader::size(void)
{
	RP_D(const WiiUH3Reader);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	if (!m_file ||  !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	return d->data_size;
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
off64_t WiiUH3Reader::partition_size(void) const
{
	RP_D(const WiiUH3Reader);
	return d->partition_size;
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
off64_t WiiUH3Reader::partition_size_used(void) const
{
	// NOTE: Assuming the entire content is "used".
	RP_D(const WiiUH3Reader);
	return d->partition_size;
}

}
