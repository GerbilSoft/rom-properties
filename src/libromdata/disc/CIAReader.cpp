/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CIAReader.cpp: Nintendo 3DS CIA reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"
#include "CIAReader.hpp"

// Other rom-properties libraries
#include "librpbase/disc/CBCReader.hpp"
#ifdef ENABLE_DECRYPTION
#  include "librpbase/crypto/AesCipherFactory.hpp"
#  include "librpbase/crypto/IAesCipher.hpp"
#  include "librpbase/crypto/KeyManager.hpp"
#  include "../crypto/N3DSVerifyKeys.hpp"
#endif /* ENABLE_DECRYPTION */
using namespace LibRpBase;
using namespace LibRpFile;

namespace LibRomData {

class CIAReaderPrivate
{
public:
	CIAReaderPrivate(CIAReader *q,
		off64_t content_offset, uint32_t content_length,
		const N3DS_Ticket_t *ticket,
		uint16_t tmd_content_index);

private:
	RP_DISABLE_COPY(CIAReaderPrivate)
protected:
	CIAReader *const q_ptr;

public:
	CBCReaderPtr cbcReader;

#ifdef ENABLE_DECRYPTION
	// KeyY index for title key encryption
	uint8_t titleKeyEncIdx;
	// TMD content index
	uint16_t tmd_content_index;
#endif /* ENABLE_DECRYPTION */
};

/** CIAReaderPrivate **/

CIAReaderPrivate::CIAReaderPrivate(CIAReader *q,
	off64_t content_offset, uint32_t content_length,
	const N3DS_Ticket_t *ticket, uint16_t tmd_content_index)
	: q_ptr(q)
#ifdef ENABLE_DECRYPTION
	, titleKeyEncIdx(0)
	, tmd_content_index(tmd_content_index)
#endif /* ENABLE_DECRYPTION */
{
#ifndef ENABLE_DECRYPTION
	RP_UNUSED(tmd_content_index);
#endif /* ENABLE_DECRYPTION */

	assert(q->m_file != nullptr);
	if (!q->m_file) {
		// No file...
		return;
	}

	assert(ticket != nullptr);
	if (!ticket) {
		// No ticket. Assuming no encryption.
		// Create a passthru CBCReader anyway.
		cbcReader = std::make_shared<CBCReader>(q->m_file, content_offset, content_length, nullptr, nullptr);
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
			keyX_verify = N3DSVerifyKeys::encryptionVerifyData_static(N3DSVerifyKeys::Key_Retail_Slot0x3DKeyX);
			keyY_verify = N3DSVerifyKeys::encryptionVerifyData_static(N3DSVerifyKeys::Key_Retail_Slot0x3DKeyY_0 + ticket->keyY_index);
			keyNormal_verify = N3DSVerifyKeys::encryptionVerifyData_static(N3DSVerifyKeys::Key_Retail_Slot0x3DKeyNormal_0 + ticket->keyY_index);
		}
	} else if (!strncmp(ticket->issuer, N3DS_TICKET_ISSUER_DEBUG, sizeof(ticket->issuer))) {
		// Debug issuer.
		keyPrefix = "ctr-dev";
		titleKeyEncIdx = N3DS_TICKET_TITLEKEY_ISSUER_DEBUG;
		if (ticket->keyY_index < 6) {
			// Verification data is available.
			keyX_verify = N3DSVerifyKeys::encryptionVerifyData_static(N3DSVerifyKeys::Key_Debug_Slot0x3DKeyX);
			keyY_verify = N3DSVerifyKeys::encryptionVerifyData_static(N3DSVerifyKeys::Key_Debug_Slot0x3DKeyY_0 + ticket->keyY_index);
			keyNormal_verify = N3DSVerifyKeys::encryptionVerifyData_static(N3DSVerifyKeys::Key_Debug_Slot0x3DKeyNormal_0 + ticket->keyY_index);
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
	snprintf(keyY_name, sizeof(keyY_name), "%s-Slot0x3DKeyY-%u",
		keyPrefix, ticket->keyY_index);
	snprintf(keyNormal_name, sizeof(keyNormal_name), "%s-Slot0x3DKeyNormal-%u",
		keyPrefix, ticket->keyY_index);

	// Get the KeyNormal. If that fails, get KeyX and KeyY,
	// then use CtrKeyScrambler to generate KeyNormal.
	u128_t keyNormal;
	KeyManager::VerifyResult res = N3DSVerifyKeys::loadKeyNormal(&keyNormal,
		keyNormal_name, keyX_name, keyY_name,
		keyNormal_verify, keyX_verify, keyY_verify);
	if (res != KeyManager::VerifyResult::OK) {
		// Unable to get the CIA encryption keys.
		// TODO: Set an error.
		//verifyResult = res;
		q->m_file.reset();
		return;
	}

	// Create a cipher to decrypt the title key.
	IAesCipher *cipher = AesCipherFactory::create();

	// Initialize parameters for title key decryption.
	// TODO: Error checking.
	// Parameters:
	// - Keyslot: 0x3D
	// - Chaining mode: CBC
	// - IV: Title ID (little-endian)
	cipher->setChainingMode(IAesCipher::ChainingMode::CBC);
	cipher->setKey(keyNormal.u8, sizeof(keyNormal.u8));

	// CIA IV is the title ID in big-endian.
	// The ticket title ID is already in big-endian,
	// so copy it over directly.
	u128_t cia_iv;
	memcpy(cia_iv.u8, &ticket->title_id.id, sizeof(ticket->title_id.id));
	memset(&cia_iv.u8[8], 0, 8);
	cipher->setIV(cia_iv.u8, sizeof(cia_iv.u8));

	// Decrypt the title key.
	uint8_t title_key[16];
	memcpy(title_key, ticket->title_key, sizeof(title_key));
	cipher->decrypt(title_key, sizeof(title_key));
	delete cipher;

	// Data area: IV is the TMD content index.
	cia_iv.u8[0] = tmd_content_index >> 8;
	cia_iv.u8[1] = tmd_content_index & 0xFF;
	memset(&cia_iv.u8[2], 0, sizeof(cia_iv.u8)-2);

	// Create a CBC reader to decrypt the CIA.
	cbcReader = std::make_shared<CBCReader>(q->m_file, content_offset, content_length, title_key, cia_iv.u8);
#else /* !ENABLE_DECRYPTION */
	// Cannot decrypt the CIA.
	// TODO: Set an error.
	q->m_file.reset();
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
CIAReader::CIAReader(const IRpFilePtr &file,
		off64_t content_offset, uint32_t content_length,
		const N3DS_Ticket_t *ticket,
		uint16_t tmd_content_index)
	: super(file)
	, d_ptr(new CIAReaderPrivate(this, content_offset, content_length, ticket, tmd_content_index))
{ }

CIAReader::~CIAReader()
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
size_t CIAReader::read(void *ptr, size_t size)
{
	RP_D(CIAReader);
	assert(ptr != nullptr);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	assert((bool)d->cbcReader);
	if (!ptr) {
		m_lastError = EINVAL;
		return 0;
	} else if (!m_file || !m_file->isOpen() || !d->cbcReader) {
		m_lastError = EBADF;
		return 0;
	} else if (size == 0) {
		// Nothing to do...
		return 0;
	}

	size_t ret = d->cbcReader->read(ptr, size);
	m_lastError = d->cbcReader->lastError();
	return ret;
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int CIAReader::seek(off64_t pos)
{
	RP_D(CIAReader);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	assert(d->cbcReader != nullptr);
	if (!m_file ||  !m_file->isOpen() || !d->cbcReader) {
		m_lastError = EBADF;
		return -1;
	}

	int ret = d->cbcReader->seek(pos);
	if (ret != 0) {
		m_lastError = d->cbcReader->lastError();
	}
	return ret;
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
off64_t CIAReader::tell(void)
{
	RP_D(const CIAReader);
	assert(m_file != nullptr);
	assert(d->cbcReader != nullptr);
	if (!m_file ||  !m_file->isOpen() || !d->cbcReader) {
		m_lastError = EBADF;
		return -1;
	}

	off64_t ret = d->cbcReader->tell();
	m_lastError = d->cbcReader->lastError();
	return ret;
}

/**
 * Get the data size.
 * This size does not include the NCCH header,
 * and it's adjusted to exclude hashes.
 * @return Data size, or -1 on error.
 */
off64_t CIAReader::size(void)
{
	RP_D(const CIAReader);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	assert(d->cbcReader != nullptr);
	if (!m_file ||  !m_file->isOpen() || !d->cbcReader) {
		m_lastError = EBADF;
		return -1;
	}

	off64_t ret = d->cbcReader->size();
	m_lastError = d->cbcReader->lastError();
	return ret;
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
off64_t CIAReader::partition_size(void) const
{
	// TODO: Handle errors.
	RP_D(const CIAReader);
	return (d->cbcReader ? d->cbcReader->partition_size() : -1);
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
off64_t CIAReader::partition_size_used(void) const
{
	// NOTE: For CIAReader, this is the same as partition_size().
	// TODO: Handle errors.
	RP_D(const CIAReader);
	return (d->cbcReader ? d->cbcReader->partition_size_used() : -1);
}

}
