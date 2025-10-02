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
using std::array;
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
		array<uint8_t, 16> key;
		array<uint8_t, 16> iv;
		bool usesIV;
#endif /* ENABLE_DECRYPTION */
};

/** CBCReaderPrivate **/

CBCReaderPrivate::CBCReaderPrivate(CBCReader *q, off64_t offset, off64_t length, const uint8_t *key, const uint8_t *iv)
	: offset(offset)
	, length(length)
	, pos(0)
#ifdef ENABLE_DECRYPTION
	, usesIV(iv != nullptr)
#endif /* ENABLE_DECRYPTION */
{
	assert(q->m_file);
	if (!q->m_file) {
		// No file...
		return;
	}

	// Length must be a multiple of 16.
	assert(length % 16LL == 0);
	length &= ~15LL;

#ifdef ENABLE_DECRYPTION
	if (!key) {
		// No key. Assuming passthru with no encryption.
		this->key.fill(0);
		this->iv.fill(0);
		return;
	}

	// Save the key and IV for later.
	memcpy(this->key.data(), key, 16);
	if (iv) {
		// IV specified. Using CBC.
		memcpy(this->iv.data(), iv, 16);
	} else {
		// No IV specified. Using ECB.
		this->iv.fill(0);
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
	cipher->setKey(this->key.data(), this->key.size());
	if (iv) {
		cipher->setIV(this->iv.data(), this->iv.size());
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

	// Read the first block.
	// NOTE: If we're in the middle of a block, round it down.
	const off64_t pos_block = d->pos & ~15LL;

	if (d->usesIV) {
		// Initialize the IV for this position.
		array<uint8_t, 16> iv;

		// Get the IV.
		if (pos_block == 0) {
			// Start of data.
			// Use the specified IV.
			iv = d->iv;
			m_file->seek(d->offset);
		} else {
			// Not start of data.
			// Read the IV from the previous 16 bytes.
			// TODO: Cache it!
			m_file->seek(d->offset + pos_block - 16);
			size_t sz_read = m_file->read(iv.data(), iv.size());
			if (sz_read != iv.size()) {
				// Read error.
				m_lastError = m_file->lastError();
				if (m_lastError == 0) {
					m_lastError = EIO;
				}
				return 0;
			}
		}

		// Set the IV.
		int ret = d->cipher->setIV(iv.data(), iv.size());
		if (ret != 0) {
			// setIV() failed.
			m_lastError = EIO;
			return 0;
		}
	} else {
		// No IV is needed. Seek directly to the data.
		m_file->seek(d->offset + pos_block);
	}

	// Total number of bytes read.
	size_t total_sz_read = 0;

	array<uint8_t, 16> block_tmp;
	if (d->pos != pos_block) {
		// We're in the middle of a block.
		// Read and decrypt the full block, and copy out
		// the necessary bytes.
		const size_t sz = std::min(16U - (static_cast<size_t>(d->pos) & 15U), size);
		size_t sz_read = m_file->read(block_tmp.data(), block_tmp.size());
		if (sz_read != block_tmp.size()) {
			// Read error.
			m_lastError = m_file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			return 0;
		}

		// Decrypt the data.
		size_t sz_dec = d->cipher->decrypt(block_tmp.data(), block_tmp.size());
		if (sz_dec != block_tmp.size()) {
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
		size_t sz_read = m_file->read(block_tmp.data(), block_tmp.size());
		if (sz_read != block_tmp.size()) {
			// Read error.
			m_lastError = m_file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			return 0;
		}

		// Decrypt the data.
		size_t sz_dec = d->cipher->decrypt(block_tmp.data(), block_tmp.size());
		if (sz_dec != block_tmp.size()) {
			// decrypt() failed.
			m_lastError = EIO;
			return 0;
		}

		memcpy(ptr8, block_tmp.data(), size);
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
 * @param pos		[in] Partition position
 * @param whence	[in] Where to seek from
 * @return 0 on success; -1 on error.
 */
int CBCReader::seek(off64_t pos, SeekWhence whence)
{
	RP_D(CBCReader);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	if (!m_file ||  !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	pos = adjust_file_pos_for_whence(pos, whence, d->pos, d->length);
	if (pos < 0) {
		// Negative is invalid.
		m_lastError = EINVAL;
		return -1;
	}
	d->pos = constrain_file_pos(pos, d->length);
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
