/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiPartition.hpp: Wii partition reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "WiiPartition.hpp"
#include "Console/wii_structs.h"

// librpbase
#include "librpbase/crypto/KeyManager.hpp"
#ifdef ENABLE_DECRYPTION
#  include "librpbase/crypto/IAesCipher.hpp"
#  include "librpbase/crypto/AesCipherFactory.hpp"
#endif /* ENABLE_DECRYPTION */
using namespace LibRpBase;

// WiiTicket for title key decryption
#include "../Console/WiiTicket.hpp"
#include "librpfile/MemFile.hpp"
using namespace LibRpFile;

// C++ STL classes
using std::array;
using std::unique_ptr;

#include "GcnPartition_p.hpp"
namespace LibRomData {

#define SECTOR_SIZE_ENCRYPTED 0x8000
#define SECTOR_SIZE_DECRYPTED 0x7C00
#define SECTOR_SIZE_DECRYPTED_OFFSET 0x400

class WiiPartitionPrivate final : public GcnPartitionPrivate
{
public:
	WiiPartitionPrivate(WiiPartition *q, off64_t data_size,
		off64_t partition_offset, WiiPartition::CryptoMethod cryptoMethod);
	~WiiPartitionPrivate() final = default;

private:
	typedef GcnPartitionPrivate super;
	RP_DISABLE_COPY(WiiPartitionPrivate)

public:
	// Partition header
	RVL_PartitionHeader partitionHeader;

	// WiiTicket
	unique_ptr<WiiTicket> wiiTicket;

	// Encryption key verification result
	KeyManager::VerifyResult verifyResult;

public:
	// Encryption key in use
	WiiTicket::EncryptionKeys encKey;
	// Encryption key that would be used if the partition was encrypted
	WiiTicket::EncryptionKeys encKeyReal;

	// Crypto method
	WiiPartition::CryptoMethod cryptoMethod;

public:
	// Incrementing values. Occasionally found in the
	// update partition of debug-signed discs.
	static const array<uint8_t, 32> incr_vals;

	// Decrypted read position. (0x7C00 bytes out of 0x8000)
	// NOTE: Actual read position if ((cryptoMethod & CM_MASK_SECTOR) == CM_32K).
	off64_t pos_7C00;

	// Decrypted sector cache.
	// NOTE: Actual data starts at 0x400.
	// Hashes and the sector IV are stored first.
	uint32_t sector_num;				// Sector number.
	union EncSector_t {
		struct {
			// NOTE: &hashes.H2[7][4], when encrypted, is the sector IV.
			struct {
				uint8_t H0[31][20];
				uint8_t pad_H0[20];
				uint8_t H1[8][20];
				uint8_t pad_H1[32];
				uint8_t H2[8][20];
				uint8_t pad_H2[32];
			} hashes;
			uint8_t data[SECTOR_SIZE_DECRYPTED];
		};
		uint8_t fulldata[SECTOR_SIZE_ENCRYPTED];
	};
	ASSERT_STRUCT(EncSector_t, SECTOR_SIZE_ENCRYPTED);
	static_assert(offsetof(EncSector_t, hashes.H2) + (7*20) + 4 == 0x3D0, "IV location is wrong");
	EncSector_t sector_buf;	// Decrypted sector data.

	/**
	 * Read and decrypt a sector.
	 * The decrypted sector is stored in sector_buf.
	 *
	 * @param sector_num Sector number. (address / 0x7C00)
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int readSector(uint32_t sector_num);

public:
	/**
	 * Initialize decryption.
	 * @return VerifyResult.
	 */
	KeyManager::VerifyResult initDecryption(void);

#ifdef ENABLE_DECRYPTION
public:
	// AES cipher for this partition's title key
	unique_ptr<IAesCipher> aes_title;
#endif
};

/** WiiPartitionPrivate **/

// Incrementing values. Occasionally found in the
// update partition of debug-signed discs.
const array<uint8_t, 32> WiiPartitionPrivate::incr_vals = {
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x04,
	0x00,0x00,0x00,0x08, 0x00,0x00,0x00,0x0C,
	0x00,0x00,0x00,0x10, 0x00,0x00,0x00,0x14,
	0x00,0x00,0x00,0x18, 0x00,0x00,0x00,0x1C,
};

WiiPartitionPrivate::WiiPartitionPrivate(WiiPartition *q,
		off64_t data_size, off64_t partition_offset,
		WiiPartition::CryptoMethod cryptoMethod)
	: super(q, partition_offset, data_size, 2)
#ifdef ENABLE_DECRYPTION
	, verifyResult(KeyManager::VerifyResult::Unknown)
#else /* !ENABLE_DECRYPTION */
	, verifyResult(KeyManager::VerifyResult::NoSupport)
#endif /* ENABLE_DECRYPTION */
	, encKey(WiiTicket::EncryptionKeys::Unknown)
	, encKeyReal(WiiTicket::EncryptionKeys::Unknown)
	, cryptoMethod(cryptoMethod)
	, pos_7C00(-1)
	, sector_num(~0)
{
	// Clear data set by GcnPartition in case the
	// partition headers can't be read.
	this->data_offset = -1;
	this->data_size = -1;
	this->partition_size = -1;

	// Clear the partition header struct.
	memset(&partitionHeader, 0, sizeof(partitionHeader));

	// Partition header will be read in the WiiPartition constructor.
}

/**
 * Initialize decryption.
 * @return VerifyResult.
 */
KeyManager::VerifyResult WiiPartitionPrivate::initDecryption(void)
{
	if (verifyResult != KeyManager::VerifyResult::Unknown) {
		// Decryption has already been initialized.
		return verifyResult;
	}

	// If decryption is enabled, we can load the key and enable reading.
	// Otherwise, we can only get the partition size information.

	// Initialize the WiiTicket.
	if (!wiiTicket) {
		MemFilePtr memFile = std::make_shared<MemFile>(
			reinterpret_cast<const uint8_t*>(&partitionHeader.ticket), sizeof(partitionHeader.ticket));
		if (!memFile->isOpen()) {
			// Failed to open a MemFile.
			// TODO: Better verifyResult value?
			return verifyResult;
		}

		// NOTE: WiiTicket requires a ".tik" file extension.
		// TODO: Have WiiTicket use dynamic_cast<> to determine if this is a MemFile?
		memFile->setFilename("title.tik");

		WiiTicket *const wiiTicket = new WiiTicket(memFile);
		if (!wiiTicket->isValid()) {
			// Not a valid ticket?
			delete wiiTicket;
			// TODO: Better verifyResult value?
			return verifyResult;
		}
		this->wiiTicket.reset(wiiTicket);
	}

	// Get the encryption key in use.
	encKeyReal = wiiTicket->encKey();
	if ((int)encKeyReal < 0 || encKeyReal >= WiiTicket::EncryptionKeys::Max) {
		// Invalid key...
		// TODO: Correct verifyResult?
		verifyResult = KeyManager::VerifyResult::KeyInvalid;
		return verifyResult;
	}

	// Is this partition encrypted?
	if ((cryptoMethod & WiiPartition::CM_MASK_ENCRYPTED) == WiiPartition::CM_UNENCRYPTED) {
		// Not encrypted.
		encKey = WiiTicket::EncryptionKeys::None;
	} else {
		// Encrypted.
		encKey = encKeyReal;
	}

#ifdef ENABLE_DECRYPTION
	// Get the title key.
	uint8_t title_key[16];
	int ret = wiiTicket->decryptTitleKey(title_key, sizeof(title_key));
	if (ret != 0) {
		// Title key decryption failed.
		verifyResult = wiiTicket->verifyResult();
		return verifyResult;
	}

	// Initialize the AES cipher.
	unique_ptr<IAesCipher> cipher(AesCipherFactory::create());
	if (!cipher || !cipher->isInit()) {
		// Error initializing the cipher.
		verifyResult = KeyManager::VerifyResult::IAesCipherInitErr;
		return verifyResult;
	}

	// Load the decrypted title key. (CBC mode)
	ret = cipher->setKey(title_key, sizeof(title_key));
	ret |= cipher->setChainingMode(IAesCipher::ChainingMode::CBC);
	if (ret != 0) {
		// Error initializing the cipher.
		verifyResult = KeyManager::VerifyResult::IAesCipherInitErr;
		return verifyResult;
	}

	// readSector() needs aes_title.
	aes_title = std::move(cipher);

	// Read sector 0, which contains a disc header.
	// NOTE: readSector() doesn't check verifyResult.
	if (readSector(0) != 0) {
		// Error reading sector 0.
		aes_title.reset();
		verifyResult = KeyManager::VerifyResult::IAesCipherDecryptErr;
		return verifyResult;
	}

	// Verify that this is a Wii partition.
	// If it isn't, the key is probably wrong.
	const GCN_DiscHeader *const discHeader =
		reinterpret_cast<const GCN_DiscHeader*>(sector_buf.data);
	if (discHeader->magic_wii != cpu_to_be32(WII_MAGIC)) {
		// Invalid disc header.

		// NOTE: Debug discs may have incrementing values in update partitions.
		if (!memcmp(sector_buf.data, incr_vals.data(), incr_vals.size())) {
			// Found incrementing values.
			verifyResult = KeyManager::VerifyResult::IncrementingValues;
		} else {
			// Not incrementing values.
			// We probably have the wrong key.
			verifyResult = KeyManager::VerifyResult::WrongKey;
		}
		return verifyResult;
	}
#else /* !ENABLE_DECRYPTION */
	if (encKey != WiiTicket::EncryptionKeys::None) {
		// Can't read an encrypted disc image in a NoCrypto build.
		verifyResult = KeyManager::VerifyResult::NoSupport;
		return verifyResult;
	}
#endif /* ENABLE_DECRYPTION */

	// Cipher initialized.
	verifyResult = KeyManager::VerifyResult::OK;
	return verifyResult;
}

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

	RP_Q(WiiPartition);
	const bool isCrypted = ((cryptoMethod & WiiPartition::CM_MASK_ENCRYPTED) == WiiPartition::CM_ENCRYPTED);
#ifndef ENABLE_DECRYPTION
	if (isCrypted) {
		// Decryption is disabled.
		q->m_lastError = EIO;
		return -1;
	}
#endif /* !ENABLE_DECRYPTION */

	// NOTE: This function doesn't check verifyResult,
	// since it's called by initDecryption() before
	// verifyResult is set.
	off64_t sector_addr = partition_offset + data_offset;
	sector_addr += (static_cast<off64_t>(sector_num) * SECTOR_SIZE_ENCRYPTED);

	int ret = q->m_file->seek(sector_addr);
	if (ret != 0) {
		q->m_lastError = q->m_file->lastError();
		return ret;
	}

	size_t sz = q->m_file->read(&sector_buf, sizeof(sector_buf));
	if (sz != sizeof(sector_buf)) {
		// sector_buf may be invalid.
		this->sector_num = ~0;
		q->m_lastError = EIO;
		return -1;
	}

#ifdef ENABLE_DECRYPTION
	if (isCrypted) {
		// Decrypt the sector.
		if (aes_title->decrypt(sector_buf.data, sizeof(sector_buf.data),
		    &sector_buf.hashes.H2[7][4], 16) != SECTOR_SIZE_DECRYPTED)
		{
			// sector_buf may be invalid.
			this->sector_num = ~0;
			q->m_lastError = EIO;
			return -1;
		}
	}
#endif /* ENABLE_DECRYPTION */

	// Sector read and decrypted.
	this->sector_num = sector_num;
	return 0;
}

/** WiiPartition **/

/**
 * Construct a WiiPartition with the specified IDiscReader.
 *
 * NOTE: The IDiscReader *must* remain valid while this
 * WiiPartition is open.
 *
 * @param discReader		[in] IDiscReader
 * @param partition_offset	[in] Partition start offset
 * @param partition_size	[in] Calculated partition size. Used if the size in the header is 0.
 * @param cryptoMethod		[in] Crypto method
 */
WiiPartition::WiiPartition(const IDiscReaderPtr &discReader, off64_t partition_offset,
		off64_t partition_size, CryptoMethod cryptoMethod)
	: super(new WiiPartitionPrivate(this, discReader->size(),
		partition_offset, cryptoMethod), discReader)
{
	// q->m_lastError is handled by GcnPartitionPrivate's constructor.
	if (!m_file || !m_file->isOpen()) {
		m_file.reset();
		return;
	}

	// Read the partition header.
	RP_D(WiiPartition);
	if (m_file->seek(partition_offset) != 0) {
		m_lastError = m_file->lastError();
		m_file.reset();
		return;
	}
	size_t size = m_file->read(&d->partitionHeader, sizeof(d->partitionHeader));
	if (size != sizeof(d->partitionHeader)) {
		m_lastError = EIO;
		m_file.reset();
		return;
	}

	// Make sure the signature type is correct.
	if (d->partitionHeader.ticket.signature_type != cpu_to_be32(RVL_CERT_SIGTYPE_RSA2048_SHA1)) {
		// TODO: Better error?
		m_lastError = EIO;
		m_file.reset();
		return;
	}

	// Save important data.
	d->data_offset = d->partitionHeader.data_offset.geto_be();
	d->data_size   = d->partitionHeader.data_size.geto_be();
	if (d->data_size == 0) {
		// NoCrypto RVT-H images sometimes have this set to 0.
		// Use the calculated partition size.
		d->data_size = partition_size - d->data_offset;
	}

	d->partition_size = d->data_size + d->data_offset;

	// Unencrypted discs don't need decryption.
	if ((cryptoMethod & WiiPartition::CM_MASK_ENCRYPTED) == WiiPartition::CM_UNENCRYPTED) {
		// No encryption. (RVT-H)
		// TODO: Get the "real" encryption key?
		d->encKey = WiiTicket::EncryptionKeys::None;
		d->pos_7C00 = 0;

		uint8_t data[32];
		size_t size = m_file->seekAndRead(d->partition_offset + d->data_offset, data, sizeof(data));
		if (size == d->incr_vals.size() && !memcmp(data, d->incr_vals.data(), d->incr_vals.size())) {
			// Found incrementing values.
			d->verifyResult = KeyManager::VerifyResult::IncrementingValues;
		} else {
			// Assume the disc is OK.
			// TODO: Check for Wii magic?
			d->verifyResult = KeyManager::VerifyResult::OK;
		}
	} else {
		// Disc is encrypted.
		// Only initialize d->pos_7C00 if decryption is supported.
#ifdef ENABLE_DECRYPTION
		d->pos_7C00 = 0;
#endif /* ENABLE_DECRYPTION */
	}

	// Encryption will not be initialized until read() is called.
}

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
	if (d->pos_7C00 >= d->data_size)
		return 0;

	// Make sure d->pos_7C00 + size <= d->data_size.
	// If it isn't, we'll do a short read.
	if (d->pos_7C00 + static_cast<off64_t>(size) >= d->data_size) {
		size = static_cast<size_t>(d->data_size - d->pos_7C00);
	}

	if ((d->cryptoMethod & CM_MASK_SECTOR) == CM_32K) {
		// Full 32K sectors. (implies no encryption)

		// Check if we're not starting on a block boundary.
		const uint32_t blockStartOffset = d->pos_7C00 % SECTOR_SIZE_ENCRYPTED;
		if (blockStartOffset != 0) {
			// Not a block boundary.
			// Read the end of the block.
			uint32_t read_sz = SECTOR_SIZE_ENCRYPTED - blockStartOffset;
			if (size < static_cast<size_t>(read_sz)) {
				read_sz = static_cast<uint32_t>(size);
			}

			// Read and decrypt the sector.
			const uint32_t blockStart = static_cast<uint32_t>(d->pos_7C00 / SECTOR_SIZE_ENCRYPTED);
			d->readSector(blockStart);

			// Copy data from the sector.
			memcpy(ptr8, &d->sector_buf.fulldata[blockStartOffset], read_sz);

			// Starting block read.
			size -= read_sz;
			ptr8 += read_sz;
			ret += read_sz;
			d->pos_7C00 += read_sz;
		}

		// Read entire blocks.
		for (; size >= SECTOR_SIZE_ENCRYPTED;
		     size -= SECTOR_SIZE_ENCRYPTED, ptr8 += SECTOR_SIZE_ENCRYPTED,
		     ret += SECTOR_SIZE_ENCRYPTED, d->pos_7C00 += SECTOR_SIZE_ENCRYPTED)
		{
			assert(d->pos_7C00 % SECTOR_SIZE_ENCRYPTED == 0);

			// Read the sector.
			const uint32_t blockStart = static_cast<uint32_t>(d->pos_7C00 / SECTOR_SIZE_ENCRYPTED);
			d->readSector(blockStart);

			// Copy data from the sector.
			memcpy(ptr8, d->sector_buf.fulldata, SECTOR_SIZE_ENCRYPTED);
		}

		// Check if we still have data left. (not a full block)
		if (size > 0) {
			// Not a full block.

			// Read the sector.
			assert(d->pos_7C00 % SECTOR_SIZE_ENCRYPTED == 0);
			const uint32_t blockEnd = static_cast<uint32_t>(d->pos_7C00 / SECTOR_SIZE_ENCRYPTED);
			d->readSector(blockEnd);

			// Copy data from the sector.
			memcpy(ptr8, d->sector_buf.fulldata, size);

			ret += size;
			d->pos_7C00 += size;
		}
	} else {
		if ((d->cryptoMethod & CM_MASK_ENCRYPTED) == CM_ENCRYPTED) {
#ifdef ENABLE_DECRYPTION
			// Make sure decryption is initialized.
			switch (d->verifyResult) {
				case KeyManager::VerifyResult::Unknown:
					// Attempt to initialize decryption.
					if (d->initDecryption() != KeyManager::VerifyResult::OK) {
						// Decryption could not be initialized.
						// TODO: Better error?
						m_lastError = EIO;
						return 0;
					}
					break;

				case KeyManager::VerifyResult::OK:
					// Decryption is initialized.
					break;

				default:
					// Decryption failed to initialize.
					// TODO: Better error?
					m_lastError = EIO;
					return 0;
			}
#else /* !ENABLE_DECRYPTION */
			// Decryption is not enabled.
			m_lastError = EIO;
			ret = 0;
#endif /* ENABLE_DECRYPTION */
		}

		// Check if we're not starting on a block boundary.
		const uint32_t blockStartOffset = d->pos_7C00 % SECTOR_SIZE_DECRYPTED;
		if (blockStartOffset != 0) {
			// Not a block boundary.
			// Read the end of the block.
			uint32_t read_sz = SECTOR_SIZE_DECRYPTED - blockStartOffset;
			if (size < static_cast<size_t>(read_sz)) {
				read_sz = static_cast<uint32_t>(size);
			}

			// Read and decrypt the sector.
			const uint32_t blockStart = static_cast<uint32_t>(d->pos_7C00 / SECTOR_SIZE_DECRYPTED);
			d->readSector(blockStart);

			// Copy data from the sector.
			memcpy(ptr8, &d->sector_buf.data[blockStartOffset], read_sz);

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
			const uint32_t blockStart = static_cast<uint32_t>(d->pos_7C00 / SECTOR_SIZE_DECRYPTED);
			d->readSector(blockStart);

			// Copy data from the sector.
			memcpy(ptr8, d->sector_buf.data, SECTOR_SIZE_DECRYPTED);
		}

		// Check if we still have data left. (not a full block)
		if (size > 0) {
			// Not a full block.

			// Read and decrypt the sector.
			assert(d->pos_7C00 % SECTOR_SIZE_DECRYPTED == 0);
			const uint32_t blockEnd = static_cast<uint32_t>(d->pos_7C00 / SECTOR_SIZE_DECRYPTED);
			d->readSector(blockEnd);

			// Copy data from the sector.
			memcpy(ptr8, d->sector_buf.data, size);

			ret += size;
			d->pos_7C00 += size;
		}
	}

	// Finished reading the data.
	return ret;
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int WiiPartition::seek(off64_t pos)
{
	RP_D(WiiPartition);
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
		d->pos_7C00 = d->data_size;
	} else {
		d->pos_7C00 = pos;
	}
	return 0;
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
off64_t WiiPartition::tell(void)
{
	RP_D(const WiiPartition);
	assert(m_file != nullptr);
	assert(m_file->isOpen());
	if (!m_file ||  !m_file->isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	return d->pos_7C00;
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
off64_t WiiPartition::partition_size_used(void) const
{
	// Get the FST used size from GcnPartition.
	off64_t size = super::partition_size_used();
	if (size <= 0) {
		// Error retrieving the FST used size.
		return size;
	}

	// Add the data offset from the partition header.
	RP_D(const WiiPartition);
	size += d->partitionHeader.data_offset.geto_be();

	// Are sectors hashed?
	if ((d->cryptoMethod & WiiPartition::CM_MASK_SECTOR) == CM_1K_31K) {
		// Partition has hashed sectors.
		// Multiply the FST used size by 32/31 to adjust for the difference.
		size = (size * 32) / 31;
	}

	// We're done here.
	return size;
}

/** WiiPartition **/

/**
 * Encryption key verification result.
 * @return Encryption key verification result.
 */
KeyManager::VerifyResult WiiPartition::verifyResult(void) const
{
	// TODO: Errors?
	RP_D(const WiiPartition);
	return d->verifyResult;
}

/**
 * Get the encryption key in use.
 * @return Encryption key in use.
 */
WiiTicket::EncryptionKeys WiiPartition::encKey(void) const
{
	// TODO: Errors?
	RP_D(WiiPartition);
	d->initDecryption();
	return d->encKey;
}

/**
 * Get the encryption key that would be in use if the partition was encrypted.
 * This is only needed for NASOS images.
 * @return "Real" encryption key in use.
 */
WiiTicket::EncryptionKeys WiiPartition::encKeyReal(void) const
{
	// TODO: Errors?
	RP_D(WiiPartition);
	d->initDecryption();
	return d->encKeyReal;
}

/**
 * Get the ticket.
 * @return Ticket, or nullptr if unavailable.
 */
const RVL_Ticket *WiiPartition::ticket(void) const
{
	RP_D(const WiiPartition);
	return (d->partitionHeader.ticket.signature_type != 0)
		? &d->partitionHeader.ticket
		: nullptr;
}

/**
 * Get the TMD header.
 * @return TMD header, or nullptr if unavailable.
 */
const RVL_TMD_Header *WiiPartition::tmdHeader(void) const
{
	RP_D(const WiiPartition);
	const RVL_TMD_Header *const tmdHeader =
		reinterpret_cast<const RVL_TMD_Header*>(d->partitionHeader.tmd);
	return (tmdHeader->signature_type != 0)
		? tmdHeader
		: nullptr;
}

/**
 * Get the title ID. (NOT BYTESWAPPED)
 * @return Title ID. (0-0 if unavailable)
 */
Nintendo_TitleID_BE_t WiiPartition::titleID(void) const
{
	RP_D(const WiiPartition);
	return d->partitionHeader.ticket.title_id;
}

}
