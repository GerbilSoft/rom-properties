/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CIAReader.cpp: Nintendo 3DS CIA reader.                                 *
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
#include "CIAReader.hpp"

// librpbase
#include "librpbase/file/IRpFile.hpp"
#ifdef ENABLE_DECRYPTION
#include "librpbase/crypto/AesCipherFactory.hpp"
#include "librpbase/crypto/IAesCipher.hpp"
#include "librpbase/crypto/KeyManager.hpp"
#include "../crypto/N3DSVerifyKeys.hpp"
#endif /* ENABLE_DECRYPTION */
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>

namespace LibRomData {

class CIAReaderPrivate
{
	public:
		CIAReaderPrivate(CIAReader *q, LibRpBase::IRpFile *file,
			int64_t content_offset, uint32_t content_length,
			const N3DS_Ticket_t *ticket,
			uint16_t tmd_content_index);
		~CIAReaderPrivate();

	private:
		RP_DISABLE_COPY(CIAReaderPrivate)
	protected:
		CIAReader *const q_ptr;

	public:
		LibRpBase::IRpFile *file;	// 3DS CIA file.

		// Content offsets.
		const int64_t content_offset;	// Content start offset, in bytes.
		const uint32_t content_length;	// Content length, in bytes.

		// Current read position within the CIA content.
		// pos = 0 indicates the beginning of the content.
		// NOTE: This cannot be more than 4 GB,
		// so we're using uint32_t.
		uint32_t pos;

#ifdef ENABLE_DECRYPTION
		// KeyY index for title key encryption.
		uint8_t titleKeyEncIdx;
		// TMD content index.
		uint16_t tmd_content_index;

		// CIA cipher.
		uint8_t title_key[16];		// Decrypted title key.
		LibRpBase::IAesCipher *cipher;	// Cipher.
#endif /* ENABLE_DECRYPTION */
};

/** CIAReaderPrivate **/

CIAReaderPrivate::CIAReaderPrivate(CIAReader *q, IRpFile *file,
	int64_t content_offset, uint32_t content_length,
	const N3DS_Ticket_t *ticket,
	uint16_t tmd_content_index)
	: q_ptr(q)
	, file(file)
	, content_offset(content_offset)
	, content_length(content_length)
	, pos(0)
#ifdef ENABLE_DECRYPTION
	, titleKeyEncIdx(0)
	, tmd_content_index(tmd_content_index)
	, cipher(nullptr)
#endif /* ENABLE_DECRYPTION */
{
#ifndef ENABLE_DECRYPTION
	RP_UNUSED(tmd_content_index);
#endif /* ENABLE_DECRYPTION */

	assert(ticket != nullptr);
	if (!ticket) {
		// No ticket. Assuming no encryption.
		return;
	}

#ifdef ENABLE_DECRYPTION
	// Check the ticket issuer.
	const char *keyPrefix;
	const uint8_t *keyX_verify = nullptr;
	const uint8_t *keyY_verify = nullptr;
	const uint8_t *keyNormal_verify = nullptr;
	if (!strncmp(ticket->issuer, N3DS_TICKET_ISSUER_RETAIL, sizeof(ticket->issuer))) {
		// Retail issuer.
		keyPrefix = "ctr";
		titleKeyEncIdx = N3DS_TICKET_TITLEKEY_ISSUER_RETAIL;
		if (ticket->keyY_index < 6) {
			// Verification data is available.
			keyX_verify = N3DSVerifyKeys::EncryptionKeyVerifyData[N3DSVerifyKeys::Key_Retail_Slot0x3DKeyX];
			keyY_verify = N3DSVerifyKeys::EncryptionKeyVerifyData[N3DSVerifyKeys::Key_Retail_Slot0x3DKeyY_0 + ticket->keyY_index];
			keyNormal_verify = N3DSVerifyKeys::EncryptionKeyVerifyData[N3DSVerifyKeys::Key_Retail_Slot0x3DKeyNormal_0 + ticket->keyY_index];
		}
	} else if (!strncmp(ticket->issuer, N3DS_TICKET_ISSUER_DEBUG, sizeof(ticket->issuer))) {
		// Debug issuer.
		keyPrefix = "ctr-dev";
		titleKeyEncIdx = N3DS_TICKET_TITLEKEY_ISSUER_DEBUG;
		if (ticket->keyY_index < 6) {
			// Verification data is available.
			keyX_verify = N3DSVerifyKeys::EncryptionKeyVerifyData[N3DSVerifyKeys::Key_Debug_Slot0x3DKeyX];
			keyY_verify = N3DSVerifyKeys::EncryptionKeyVerifyData[N3DSVerifyKeys::Key_Debug_Slot0x3DKeyY_0 + ticket->keyY_index];
			keyNormal_verify = N3DSVerifyKeys::EncryptionKeyVerifyData[N3DSVerifyKeys::Key_Debug_Slot0x3DKeyNormal_0 + ticket->keyY_index];
		}
	} else {
		// Unknown issuer.
		keyPrefix = "ctr";
		titleKeyEncIdx = N3DS_TICKET_TITLEKEY_ISSUER_UNKNOWN;
	}

	// Check the KeyY index.
	// TODO: Handle invalid KeyY indexes?
	titleKeyEncIdx |= (ticket->keyY_index << 2);

	// Keyslot names.
	char keyX_name[40];
	char keyY_name[40];
	char keyNormal_name[40];
	snprintf(keyX_name, sizeof(keyNormal_name), "%s-Slot0x3DKeyX", keyPrefix);
	snprintf(keyY_name, sizeof(keyY_name), "%s-Slot0x3DKeyY-%d",
		keyPrefix, ticket->keyY_index);
	snprintf(keyNormal_name, sizeof(keyNormal_name), "%s-Slot0x3DKeyNormal-%d",
		keyPrefix, ticket->keyY_index);

	// Get the KeyNormal. If that fails, get KeyX and KeyY,
	// then use CtrKeyScrambler to generate KeyNormal.
	u128_t keyNormal;
	KeyManager::VerifyResult res = N3DSVerifyKeys::loadKeyNormal(&keyNormal,
		keyNormal_name, keyX_name, keyY_name,
		keyNormal_verify, keyX_verify, keyY_verify);
	if (res == KeyManager::VERIFY_OK) {
		// Create the cipher.
		cipher = AesCipherFactory::create();

		// Initialize parameters for title key decryption.
		// TODO: Error checking.
		// Parameters:
		// - Keyslot: 0x3D
		// - Chaining mode: CBC
		// - IV: Title ID (little-endian)
		cipher->setChainingMode(IAesCipher::CM_CBC);
		cipher->setKey(keyNormal.u8, sizeof(keyNormal.u8));
		// CIA IV is the title ID in big-endian.
		// The ticket title ID is already in big-endian,
		// so copy it over directly.
		u128_t cia_iv;
		memcpy(cia_iv.u8, &ticket->title_id.id, sizeof(ticket->title_id.id));
		memset(&cia_iv.u8[8], 0, 8);
		cipher->setIV(cia_iv.u8, sizeof(cia_iv.u8));

		// Decrypt the title key.
		memcpy(title_key, ticket->title_key, sizeof(title_key));
		cipher->decrypt(title_key, sizeof(title_key));

		// Initialize parameters for CIA decryption.
		// Parameters:
		// - Key: Decrypted title key.
		// - Chaining mode: CBC
		// - IV: Content index from the TMD.
		cipher->setKey(title_key, sizeof(title_key));
	} else {
		// Unable to get the CIA encryption keys.
		// TODO: Set an error.
		memset(title_key, 0, sizeof(title_key));
		//verifyResult = res;
		this->file = nullptr;
	}
#else /* !ENABLE_DECRYPTION */
	// Cannot decrypt the CIA.
	// TODO: Set an error.
	this->file = nullptr;
#endif /* ENABLE_DECRYPTION */
}

CIAReaderPrivate::~CIAReaderPrivate()
{
#ifdef ENABLE_DECRYPTION
	delete cipher;
#endif /* ENABLE_DECRYPTION */
}

/** CIAReader **/

/**
 * Construct a CIAReader with the specified IRpFile.
 *
 * NOTE: The IRpFile *must* remain valid while this
 * CIAReader is open.
 *
 * @param file 			[in] IRpFile.
 * @param content_offset	[in] Content start offset, in bytes.
 * @param content_length	[in] Content length, in bytes.
 * @param ticket		[in,opt] Ticket for decryption. (nullptr if NoCrypto)
 * @param tmd_content_index	[in,opt] TMD content index for decryption.
 */
CIAReader::CIAReader(IRpFile *file,
		int64_t content_offset, uint32_t content_length,
		const N3DS_Ticket_t *ticket,
		uint16_t tmd_content_index)
	: d_ptr(new CIAReaderPrivate(this, file, content_offset, content_length, ticket, tmd_content_index))
{ }

CIAReader::~CIAReader()
{
	delete d_ptr;
}

/** IDiscReader **/

/**
 * Is the partition open?
 * This usually only returns false if an error occurred.
 * @return True if the partition is open; false if it isn't.
 */
bool CIAReader::isOpen(void) const
{
	RP_D(CIAReader);
	return (d->file && d->file->isOpen());
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t CIAReader::read(void *ptr, size_t size)
{
	RP_D(CIAReader);
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
	if (d->pos >= d->content_length)
		return 0;

	// Make sure d->pos + size <= d->content_length.
	// If it isn't, we'll do a short read.
	if (d->pos + (int64_t)size >= d->content_length) {
		size = (size_t)(d->content_length - d->pos);
	}

#ifdef ENABLE_DECRYPTION
	// If decryption isn't available, CIAReader would have
	// NULLed out this->file if the CIA was encrypted.
	if (!d->cipher)
#endif /* ENABLE_DECRYPTION */
	{
		// No CIA encryption. Read directly from the file.
		size_t sz_read = d->file->seekAndRead(d->content_offset + d->pos, ptr, size);
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
	int64_t phys_addr = d->content_offset + d->pos;

	// Determine the CIA IV.
	u128_t cia_iv;
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
		// Start of CIA content.
		// IV is the TMD content index.
		cia_iv.u8[0] = d->tmd_content_index >> 8;
		cia_iv.u8[1] = d->tmd_content_index & 0xFF;
		memset(&cia_iv.u8[2], 0, sizeof(cia_iv.u8)-2);
	} else {
		// IV is the previous 16 bytes.
		// TODO: Cache this?
		size_t sz_read = d->file->read(&cia_iv.u8, sizeof(cia_iv.u8));
		if (sz_read != sizeof(cia_iv.u8)) {
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
	ret = d->cipher->setIV(cia_iv.u8, sizeof(cia_iv.u8));
	if (ret != 0) {
		// setIV() failed.
		m_lastError = EIO;
		return 0;
	}
	unsigned int sz_dec = d->cipher->decrypt(static_cast<uint8_t*>(ptr), size);
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
int CIAReader::seek(int64_t pos)
{
	RP_D(CIAReader);
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
	else if (pos >= d->content_length)
		d->pos = d->content_length;
	else
		d->pos = (uint32_t)pos;
	return 0;
}

/**
 * Seek to the beginning of the partition.
 */
void CIAReader::rewind(void)
{
	seek(0);
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
int64_t CIAReader::tell(void)
{
	RP_D(CIAReader);
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
int64_t CIAReader::size(void)
{
	RP_D(const CIAReader);
	return d->content_length;
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
int64_t CIAReader::partition_size(void) const
{
	// TODO: Errors?
	RP_D(const CIAReader);
	return d->content_length;
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
int64_t CIAReader::partition_size_used(void) const
{
	// TODO: Errors?
	// NOTE: For CIAReader, this is the same as partition_size().
	RP_D(const CIAReader);
	return d->content_length;
}

}
