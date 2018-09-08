/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * CBCReader.hpp: AES-128-CBC data reader class.                           *
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

#include "librpbase/config.librpbase.h"
#include "CBCReader.hpp"

// librpbase
#include "file/IRpFile.hpp"
#ifdef ENABLE_DECRYPTION
# include "crypto/AesCipherFactory.hpp"
# include "crypto/IAesCipher.hpp"
# include "crypto/KeyManager.hpp"
#endif

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>

namespace LibRpBase {

class CBCReaderPrivate
{
	public:
		CBCReaderPrivate(CBCReader *q, LibRpBase::IRpFile *file, int64_t offset, int64_t length,
			const uint8_t *key, const uint8_t *iv);
		~CBCReaderPrivate();

	private:
		RP_DISABLE_COPY(CBCReaderPrivate)
	protected:
		CBCReader *const q_ptr;

	public:
		LibRpBase::IRpFile *file;	// File with encrypted data.
		const int64_t offset;		// Encrypted data start offset, in bytes.
		const int64_t length;		// Encrypted data length, in bytes.

		// Current read position within the encrypted data.
		// pos = 0 indicates the beginning of the content.
		int64_t pos;

#ifdef ENABLE_DECRYPTION
		// Encryption cipher.
		uint8_t key[16];
		uint8_t iv[16];
		LibRpBase::IAesCipher *cipher;
#endif /* ENABLE_DECRYPTION */
};

/** CBCReaderPrivate **/

CBCReaderPrivate::CBCReaderPrivate(CBCReader *q, IRpFile *file,
	int64_t offset, int64_t length,
	const uint8_t *key, const uint8_t *iv)
	: q_ptr(q)
	, file(file)
	, offset(offset)
	, length(length)
	, pos(0)
#ifdef ENABLE_DECRYPTION
	, cipher(nullptr)
#endif
{
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
	cipher = AesCipherFactory::create();
	if (!cipher) {
		// Unable to initialize decryption.
		// TODO: Error code.
		this->file = nullptr;
		return;
	}

	// Initialize parameters for CBC decryption.
	cipher->setChainingMode(iv != nullptr ? IAesCipher::CM_CBC : IAesCipher::CM_ECB);
	cipher->setKey(this->key, sizeof(this->key));
	if (iv) {
		cipher->setIV(this->iv, sizeof(this->iv));
	}
#else /* !ENABLE_DECRYPTION */
	// Passthru only.
	assert(key == nullptr);
	assert(iv == nullptr);
#endif /* ENABLE_DECRYPTION */
}

CBCReaderPrivate::~CBCReaderPrivate()
{
#ifdef ENABLE_DECRYPTION
	delete cipher;
#endif /* ENABLE_DECRYPTION */
}

/** CBCReader **/

/**
 * Construct a CBCReader with the specified IRpFile.
 *
 * NOTE: The IRpFile *must* remain valid while this
 * CBCReader is open.
 *
 * @param file 		[in] IRpFile.
 * @param offset	[in] Encrypted data start offset, in bytes.
 * @param length	[in] Encrypted data length, in bytes.
 * @param key		[in] Encryption key. (Must be 128-bit) [If NULL, acts like no encryption.]
 * @param iv		[in] Initialization vector. (Must be 128-bit) [If NULL, uses ECB instead of CBC.]
 */
CBCReader::CBCReader(LibRpBase::IRpFile *file, int64_t offset, int64_t length,
		const uint8_t *key, const uint8_t *iv)
	: d_ptr(new CBCReaderPrivate(this, file, offset, length, key, iv))
{ }

CBCReader::~CBCReader()
{
	delete d_ptr;
}

/** IDiscReader **/

/**
 * Is the partition open?
 * This usually only returns false if an error occurred.
 * @return True if the partition is open; false if it isn't.
 */
bool CBCReader::isOpen(void) const
{
	RP_D(CBCReader);
	return (d->file && d->file->isOpen());
}

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
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!ptr) {
		m_lastError = EINVAL;
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
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
	if (d->pos + (int64_t)size >= d->length) {
		size = (size_t)(d->length - d->pos);
	}

#ifdef ENABLE_DECRYPTION
	// If d->cipher is nullptr, this means key was nullptr,
	// so pass it through as if it's not encrypted.
	if (!d->cipher)
#endif /* ENABLE_DECRYPTION */
	{
		// No encryption. Read directly from the file.
		size_t sz_read = d->file->seekAndRead(d->offset + d->pos, ptr, size);
		if (sz_read != size) {
			// Seek and/or read error.
			m_lastError = d->file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			return 0;
		}
		return sz_read;
	}

#ifdef ENABLE_DECRYPTION
	// TODO: Handle reads that aren't a multiple of 16 bytes.
	assert(d->pos % 16 == 0);
	assert(size % 16 == 0);
	if (d->pos % 16 != 0 || size % 16 != 0) {
		// Cannot read now.
		return 0;
	}

	// Physical address.
	int64_t phys_addr = d->offset + d->pos;

	// Determine the current IV.
	uint8_t cur_iv[16];
	if (d->pos >= 16) {
		// Subtract 16 in order to read the IV.
		phys_addr -= 16;
	}
	int ret = d->file->seek(phys_addr);
	if (ret != 0) {
		// Seek error.
		m_lastError = d->file->lastError();
		if (m_lastError == 0) {
			m_lastError = EIO;
		}
		return 0;
	}

	if (d->pos < 16) {
		// Start of encrypted data.
		// Use the specified IV.
		memcpy(cur_iv, d->iv, sizeof(cur_iv));
	} else {
		// IV is the previous 16 bytes.
		// TODO: Cache this?
		size_t sz_read = d->file->read(cur_iv, sizeof(cur_iv));
		if (sz_read != sizeof(cur_iv)) {
			// Read error.
			m_lastError = d->file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			return 0;
		}
	}

	// Read the data.
	size_t sz_read = d->file->read(ptr, size);
	if (sz_read != size) {
		// Short read.
		// Cannot decrypt with a short read.
		m_lastError = d->file->lastError();
		if (m_lastError == 0) {
			m_lastError = EIO;
		}
		return 0;
	}

	// Decrypt the data.
	ret = d->cipher->setIV(cur_iv, sizeof(cur_iv));
	if (ret != 0) {
		// setIV() failed.
		m_lastError = EIO;
		return 0;
	}
	unsigned int sz_dec = d->cipher->decrypt(
		static_cast<uint8_t*>(ptr), (unsigned int)size);
	if (sz_dec != size) {
		// decrypt() failed.
		m_lastError = EIO;
		return 0;
	}

	// Data read and decrypted successfully.
	return sz_read;
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
int CBCReader::seek(int64_t pos)
{
	RP_D(CBCReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file ||  !d->file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// Handle out-of-range cases.
	// TODO: How does POSIX behave?
	if (pos < 0)
		d->pos = 0;
	else if (pos >= d->length)
		d->pos = d->length;
	else
		d->pos = (uint32_t)pos;
	return 0;
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
int64_t CBCReader::tell(void)
{
	RP_D(CBCReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file ||  !d->file->isOpen()) {
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
int64_t CBCReader::size(void)
{
	RP_D(const CBCReader);
	return d->length;
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
int64_t CBCReader::partition_size(void) const
{
	// TODO: Errors?
	RP_D(const CBCReader);
	return d->length;
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
int64_t CBCReader::partition_size_used(void) const
{
	// TODO: Errors?
	// NOTE: For CBCReader, this is the same as partition_size().
	RP_D(const CBCReader);
	return d->length;
}

}
