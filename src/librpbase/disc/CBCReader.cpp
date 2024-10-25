/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * CBCReader.hpp: AES-128-CBC data reader class.                           *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"
#include "CBCReader.hpp"

// librpbase
#ifdef ENABLE_DECRYPTION
#  include "crypto/AesCipherFactory.hpp"
#  include "crypto/IAesCipher.hpp"
#endif

// Other rom-properties libraries
#include "librpfile/IRpFile.hpp"
using namespace LibRpFile;

#ifdef ENABLE_DECRYPTION
using std::unique_ptr;
#endif /* ENABLE_DECRYPTION */

namespace LibRpBase {

class CBCReaderPrivate
{
	public:
		CBCReaderPrivate(CBCReader *q, off64_t offset, off64_t length, const uint8_t *key, const uint8_t *iv);

	private:
		RP_DISABLE_COPY(CBCReaderPrivate)

	public:
		const off64_t offset;		// Encrypted data start offset, in bytes.
		const off64_t length;		// Encrypted data length, in bytes.

		// Current read position within the encrypted data.
		// pos = 0 indicates the beginning of the content.
		off64_t pos;

#ifdef ENABLE_DECRYPTION
		// Encryption cipher
		unique_ptr<IAesCipher> cipher;
		uint8_t key[16];
		uint8_t iv[16];
#endif /* ENABLE_DECRYPTION */
};

/** CBCReaderPrivate **/

CBCReaderPrivate::CBCReaderPrivate(CBCReader *q, off64_t offset, off64_t length, const uint8_t *key, const uint8_t *iv)
	: offset(offset)
	, length(length)
	, pos(0)
{
	assert((bool)q->m_file);
	if (!q->m_file) {
		// No file...
		return;
	}

#ifdef ENABLE_DECRYPTION
	if (!key) {
		// No key. Assuming passthru with no encryption.
		memset(this->key, 0, sizeof(this->key));
		memset(this->iv, 0, sizeof(this->iv));
		return;
	}

	// Save the key and IV for later.
	memcpy(this->key, key, 16);
	if (iv) {
		// IV specified. Using CBC.
		memcpy(this->iv, iv, 16);
	} else {
		// No IV specified. Using ECB.
		memset(this->iv, 0, sizeof(this->iv));
	}

	// Create the cipher.
	cipher.reset(AesCipherFactory::create());
	if (!cipher) {
		// Unable to initialize decryption.
		// TODO: Error code.
		q->m_file.reset();
		return;
	}

	// Initialize parameters for CBC decryption.
	cipher->setChainingMode(iv != nullptr ? IAesCipher::ChainingMode::CBC : IAesCipher::ChainingMode::ECB);
	cipher->setKey(this->key, sizeof(this->key));
	if (iv) {
		cipher->setIV(this->iv, sizeof(this->iv));
	}
#else /* !ENABLE_DECRYPTION */
	// Passthru only.
	assert(key == nullptr);
	assert(iv == nullptr);
	RP_UNUSED(key);
	RP_UNUSED(iv);
#endif /* ENABLE_DECRYPTION */
}

/** CBCReader **/

/**
 * Construct a CBCReader with the specified IRpFile.
 *
 * NOTE: The IRpFile *must* remain valid while this
 * CBCReader is open.
 *
 * @param file 		[in] IRpFile
 * @param offset	[in] Encrypted data start offset, in bytes.
 * @param length	[in] Encrypted data length, in bytes.
 * @param key		[in] Encryption key. (Must be 128-bit) [If NULL, acts like no encryption.]
 * @param iv		[in] Initialization vector. (Must be 128-bit) [If NULL, uses ECB instead of CBC.]
 */
CBCReader::CBCReader(const IRpFilePtr &file, off64_t offset, off64_t length,
		const uint8_t *key, const uint8_t *iv)
	: super(file)
	, d_ptr(new CBCReaderPrivate(this, offset, length, key, iv))
{ }

CBCReader::~CBCReader()
{
	delete d_ptr;
}

/** IDiscReader **/

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t CBCReader::read(void *ptr, size_t size)
{
	RP_D(CBCReader);
	assert(ptr != nullptr);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	if (!ptr) {
		m_lastError = EINVAL;
		return 0;
	} else if (!m_file || !m_file->isOpen()) {
		m_lastError = EBADF;
		return 0;
	} else if (size == 0) {
		// Nothing to do...
		return 0;
	}

	// Are we already at the end of the file?
	if (d->pos >= d->length)
		return 0;

	// Make sure d->pos + size <= d->length.
	// If it isn't, we'll do a short read.
	if (d->pos + (off64_t)size >= d->length) {
		size = (size_t)(d->length - d->pos);
	}

#ifdef ENABLE_DECRYPTION
	// If d->cipher is nullptr, this means key was nullptr,
	// so pass it through as if it's not encrypted.
	if (!d->cipher)
#endif /* ENABLE_DECRYPTION */
	{
		// No encryption. Read directly from the file.
		size_t sz_read = m_file->seekAndRead(d->offset + d->pos, ptr, size);
		if (sz_read != size) {
			// Seek and/or read error.
			m_lastError = m_file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			return 0;
		}
		d->pos += size;
		return sz_read;
	}

#ifdef ENABLE_DECRYPTION
	uint8_t *ptr8 = static_cast<uint8_t*>(ptr);

	// TODO: Check for overflow.
	if (d->pos + (off64_t)size > d->length) {
		// Reduce size so it doesn't go out of bounds.
		size = static_cast<size_t>(d->length - d->pos);
	}

	uint8_t iv[16];

	// Read the first block.
	// NOTE: If we're in the middle of a block, round it down.
	const off64_t pos_block = d->pos & ~15LL;

	// Total number of bytes read.
	size_t total_sz_read = 0;

	// Get the IV.
	if (pos_block == 0) {
		// Start of data.
		// Use the specified IV.
		memcpy(iv, d->iv, sizeof(iv));
		m_file->seek(d->offset);
	} else {
		// Not start of data.
		// Read the IV from the previous 16 bytes.
		// TODO: Cache it!
		m_file->seek(d->offset + pos_block - 16);
		size_t sz_read = m_file->read(iv, sizeof(iv));
		if (sz_read != sizeof(iv)) {
			// Read error.
			m_lastError = m_file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			return 0;
		}
	}

	// Set the IV.
	int ret = d->cipher->setIV(iv, sizeof(iv));
	if (ret != 0) {
		// setIV() failed.
		m_lastError = EIO;
		return 0;
	}

	uint8_t block_tmp[16];
	if (d->pos != pos_block) {
		// We're in the middle of a block.
		// Read and decrypt the full block, and copy out
		// the necessary bytes.
		const size_t sz = std::min(16U - (static_cast<size_t>(d->pos) & 15U), size);
		size_t sz_read = m_file->read(block_tmp, sizeof(block_tmp));
		if (sz_read != sizeof(block_tmp)) {
			// Read error.
			m_lastError = m_file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			return 0;
		}

		// Decrypt the data.
		size_t sz_dec = d->cipher->decrypt(block_tmp, sizeof(block_tmp));
		if (sz_dec != sizeof(block_tmp)) {
			// decrypt() failed.
			m_lastError = EIO;
			return 0;
		}

		memcpy(ptr8, &block_tmp[d->pos & 15], sz);
		ptr8 += sz;
		size -= sz;
		total_sz_read += sz;
		d->pos += sz;
	}

	// Read full blocks.
	const size_t full_block_sz = (size & ~15ULL);
	if (full_block_sz > 0) {
		size_t sz_read = m_file->read(ptr8, full_block_sz);
		if (sz_read != full_block_sz) {
			// Short read.
			// Cannot decrypt with a short read.
			m_lastError = m_file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			return 0;
		}

		// Decrypt the data.
		size_t sz_dec = d->cipher->decrypt(ptr8, full_block_sz);
		if (sz_dec != full_block_sz) {
			// decrypt() failed.
			m_lastError = EIO;
			return 0;
		}

		ptr8 += sz_read;
		size -= sz_read;
		total_sz_read += sz_read;
		d->pos += sz_read;
	}

	if (size > 0) {
		// We need to decrypt a partial block at the end.
		// Read and decrypt the full block, and copy out
		// the necessary bytes.
		size_t sz_read = m_file->read(block_tmp, sizeof(block_tmp));
		if (sz_read != sizeof(block_tmp)) {
			// Read error.
			m_lastError = m_file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			return 0;
		}

		// Decrypt the data.
		size_t sz_dec = d->cipher->decrypt(block_tmp, sizeof(block_tmp));
		if (sz_dec != sizeof(block_tmp)) {
			// decrypt() failed.
			m_lastError = EIO;
			return 0;
		}

		memcpy(ptr8, block_tmp, size);
		ptr8 += size;
		total_sz_read += size;
		d->pos += size;
		size = 0;
	}

	// Data read and decrypted successfully.
	return total_sz_read;
#else
	// Cannot decrypt data if decryption is disabled.
	return 0;
#endif /* ENABLE_DECRYPTION */
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int CBCReader::seek(off64_t pos)
{
	RP_D(CBCReader);
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
	} else if (pos >= d->length) {
		d->pos = d->length;
	} else {
		d->pos = static_cast<uint32_t>(pos);
	}
	return 0;
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
off64_t CBCReader::tell(void)
{
	RP_D(CBCReader);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	if (!m_file || !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	return d->pos;
}

/**
 * Get the data size.
 * This size does not include the NCCH header,
 * and it's adjusted to exclude hashes.
 * @return Data size, or -1 on error.
 */
off64_t CBCReader::size(void)
{
	RP_D(const CBCReader);
	return d->length;
}

}
