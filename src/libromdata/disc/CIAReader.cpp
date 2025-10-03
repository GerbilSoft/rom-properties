/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CIAReader.cpp: Nintendo 3DS CIA reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
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

// C++ STL classes
using std::array;
using std::string;

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
			keyX_verify = N3DSVerifyKeys::encryptionVerifyData_static(static_cast<int>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyX));
			keyY_verify = N3DSVerifyKeys::encryptionVerifyData_static(static_cast<int>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyY_0) + ticket->keyY_index);
			keyNormal_verify = N3DSVerifyKeys::encryptionVerifyData_static(static_cast<int>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyNormal_0) + ticket->keyY_index);
		}
	} else if (!strncmp(ticket->issuer, N3DS_TICKET_ISSUER_DEBUG, sizeof(ticket->issuer))) {
		// Debug issuer.
		keyPrefix = "ctr-dev";
		titleKeyEncIdx = N3DS_TICKET_TITLEKEY_ISSUER_DEBUG;
		if (ticket->keyY_index < 6) {
			// Verification data is available.
			keyX_verify = N3DSVerifyKeys::encryptionVerifyData_static(static_cast<int>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyX));
			keyY_verify = N3DSVerifyKeys::encryptionVerifyData_static(static_cast<int>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyY_0) + ticket->keyY_index);
			keyNormal_verify = N3DSVerifyKeys::encryptionVerifyData_static(static_cast<int>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyNormal_0) + ticket->keyY_index);
		}
	} else {
		// Unknown issuer.
		keyPrefix = "ctr";
		titleKeyEncIdx = N3DS_TICKET_TITLEKEY_ISSUER_UNKNOWN;
	}

	// Check the KeyY index.
	// TODO: Handle invalid KeyY indexes?
	titleKeyEncIdx |= (ticket->keyY_index << 2);

	// Keyslot names
	const string keyX_name = fmt::format(FSTR("{:s}-Slot0x3DKeyX"), keyPrefix);
	const string keyY_name = fmt::format(FSTR("{:s}-Slot0x3DKeyY-{:d}"), keyPrefix, ticket->keyY_index);
	const string keyNormal_name = fmt::format(FSTR("{:s}-Slot0x3DKeyNormal-{:d}"), keyPrefix, ticket->keyY_index);

	// Get the KeyNormal. If that fails, get KeyX and KeyY,
	// then use CtrKeyScrambler to generate KeyNormal.
	u128_t keyNormal;
	KeyManager::VerifyResult res = N3DSVerifyKeys::loadKeyNormal(keyNormal,
		keyNormal_name.c_str(), keyX_name.c_str(), keyY_name.c_str(),
		keyNormal_verify, keyX_verify, keyY_verify);
	if (res != KeyManager::VerifyResult::OK) {
		// Unable to get the CIA encryption keys.
		// TODO: Set an error.
		//verifyResult = res;
		q->m_file.reset();
		return;
	}

	// Create a cipher to decrypt the title key.
	IAesCipher *const cipher = AesCipherFactory::create();

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
	array<uint8_t, 16> cia_iv;
	memcpy(cia_iv.data(), &ticket->title_id.id, sizeof(ticket->title_id.id));
	memset(&cia_iv[8], 0, 8);
	cipher->setIV(cia_iv.data(), cia_iv.size());

	// Decrypt the title key.
	array<uint8_t, 16> title_key;
	memcpy(title_key.data(), ticket->title_key, title_key.size());
	cipher->decrypt(title_key.data(), title_key.size());
	delete cipher;

	// Data area: IV is the TMD content index.
	cia_iv.fill(0);
	cia_iv[0] = tmd_content_index >> 8;
	cia_iv[1] = tmd_content_index & 0xFF;

	// Create a CBC reader to decrypt the CIA.
	cbcReader = std::make_shared<CBCReader>(q->m_file, content_offset, content_length, title_key.data(), cia_iv.data());
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
 * @param pos		[in] Partition position
 * @param whence	[in] Where to seek from
 * @return 0 on success; -1 on error.
 */
int CIAReader::seek(off64_t pos, SeekWhence whence)
{
	RP_D(CIAReader);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	assert(d->cbcReader != nullptr);
	if (!m_file ||  !m_file->isOpen() || !d->cbcReader) {
		m_lastError = EBADF;
		return -1;
	}

	int ret = d->cbcReader->seek(pos, whence);
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

}
