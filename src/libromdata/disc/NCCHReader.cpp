/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NCCHReader.cpp: Nintendo 3DS NCCH reader.                               *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include "NCCHReader.hpp"
#include "config.libromdata.h"

#include "file/IRpFile.hpp"
#include "PartitionFile.hpp"

#ifdef ENABLE_DECRYPTION
#include "crypto/AesCipherFactory.hpp"
#include "crypto/IAesCipher.hpp"
#endif /* ENABLE_DECRYPTION */

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <memory>
#include <vector>
using std::vector;
using std::unique_ptr;

#include "NCCHReader_p.hpp"
namespace LibRomData {

/** NCCHReaderPrivate **/

NCCHReaderPrivate::NCCHReaderPrivate(NCCHReader *q, IRpFile *file,
	uint8_t media_unit_shift,
	int64_t ncch_offset, uint32_t ncch_length,
	const N3DS_Ticket_t *ticket,
	uint16_t tmd_content_index)
	: q_ptr(q)
	, file(file)
	, ncch_offset(ncch_offset)
	, ncch_length(ncch_length)
	, media_unit_shift(media_unit_shift)
	, pos(0)
	, headers_loaded(0)
	, verifyResult(KeyManager::VERIFY_UNKNOWN)
#ifdef ENABLE_DECRYPTION
	, tid_be(0)
	, cipher_ncch(nullptr)
	, cipher_cia(nullptr)
	, titleKeyEncIdx(0)
	, tmd_content_index(tmd_content_index)
#endif /* ENABLE_DECRYPTION */
{
#ifdef ENABLE_DECRYPTION
	// Check if this is a CIA with title key encryption.
	if (ticket) {
		// Check the ticket issuer.
		const char *keyPrefix;
		const uint8_t *keyX_verify = nullptr;
		const uint8_t *keyY_verify = nullptr;
		const uint8_t *keyNormal_verify = nullptr;
		if (!strncmp(ticket->issuer, N3DS_TICKET_ISSUER_RETAIL, sizeof(ticket->issuer))) {
			// Retail issuer..
			keyPrefix = "ctr";
			titleKeyEncIdx = N3DS_TICKET_TITLEKEY_ISSUER_RETAIL;
		} else if (!strncmp(ticket->issuer, N3DS_TICKET_ISSUER_DEBUG, sizeof(ticket->issuer))) {
			// Debug issuer.
			keyPrefix = "ctr-dev";
			titleKeyEncIdx = N3DS_TICKET_TITLEKEY_ISSUER_DEBUG;
			if (ticket->keyY_index < 6) {
				// Verification data is available.
				keyX_verify = EncryptionKeyVerifyData[Key_Debug_Slot0x3DKeyX];
				keyY_verify = EncryptionKeyVerifyData[Key_Debug_Slot0x3DKeyY_0 + ticket->keyY_index];
				keyNormal_verify = EncryptionKeyVerifyData[Key_Debug_Slot0x3DKeyNormal_0 + ticket->keyY_index];
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
		KeyManager::VerifyResult res = loadKeyNormal(&keyNormal,
			keyNormal_name, keyX_name, keyY_name,
			keyNormal_verify, keyX_verify, keyY_verify);
		if (res == KeyManager::VERIFY_OK) {
			// Create the cipher.
			cipher_cia = AesCipherFactory::create();

			// Initialize parameters for title key decryption.
			// TODO: Error checking.
			// Parameters:
			// - Keyslot: 0x3D
			// - Chaining mode: CBC
			// - IV: Title ID (little-endian)
			cipher_cia->setChainingMode(IAesCipher::CM_CBC);
			cipher_cia->setKey(keyNormal.u8, sizeof(keyNormal.u8));
			tid_be = ticket->title_id.id;	// already in BE
			ctr_t cia_iv;
			memcpy(cia_iv.u8, &tid_be, sizeof(tid_be));
			memset(&cia_iv.u8[8], 0, 8);
			cipher_cia->setIV(cia_iv.u8, sizeof(cia_iv.u8));

			// Decrypt the title key.
			memcpy(title_key, ticket->title_key, sizeof(title_key));
			cipher_cia->decrypt(title_key, sizeof(title_key));

			// Initialize parameters for CIA decryption.
			// Parameters:
			// - Key: Decrypted title key.
			// - Chaining mode: CBC
			// - IV: Content index from the TMD.
			cipher_cia->setKey(title_key, sizeof(title_key));
		} else {
			// Unable to get the CIA encryption keys.
			memset(title_key, 0, sizeof(title_key));
		}
	} else {
		// No CIA encryption.
		memset(title_key, 0, sizeof(title_key));
	}
#endif /* ENABLE_DECRYPTION */

	// Read the NCCH header.
	// We're including the signature, since the first 16 bytes
	// are used for encryption in certain cases.
	size_t size = readFromROM(0, &ncch_header, sizeof(ncch_header));
	if (size != sizeof(ncch_header)) {
		// Read error.
		// NOTE: readFromROM() sets q->m_lastError.
		this->file = nullptr;
		return;
	}

	// Verify the NCCH magic number.
	if (memcmp(ncch_header.hdr.magic, N3DS_NCCH_HEADER_MAGIC, sizeof(ncch_header.hdr.magic)) != 0) {
		// NCCH magic number is incorrect.
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		this->file = nullptr;
		return;
	}
	headers_loaded |= HEADER_NCCH;

#ifdef ENABLE_DECRYPTION
	// Byteswap the title ID. (It's used for the AES counter.)
	if (tid_be == 0) {
		tid_be = __swab64(ncch_header.hdr.program_id.id);
	}
#endif /* ENABLE_DECRYPTION */

#ifdef ENABLE_DECRYPTION
	// Determine the keyset to use.
	verifyResult = loadNCCHKeys(ncch_keys, &ncch_header, titleKeyEncIdx);
	if (verifyResult != KeyManager::VERIFY_OK) {
		// Failed to load the keyset.
		// Zero out the keys.
		// TODO: Disable anything that requires reading encrypted data.
		// TODO: Determine which key couldn't be loaded.
	}
#else /* !ENABLE_DECRYPTION */
	// Decryption is not available, so only NoCrypto is allowed.
	if (!(ncch_header.hdr.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto)) {
		// Unsupported.
		verifyResult = KeyManager::VERIFY_NO_SUPPORT;
		q->m_lastError = EIO;
		this->file = nullptr;
		return;
	}
	// No decryption is required.
	verifyResult = KeyManager::VERIFY_OK;
#endif /* ENABLE_DECRYPTION */

	// Load the ExeFS header.
	// NOTE: Checking for >= 16, since it has to be
	// non-zero and on a multiple of 16 for AES.
	// TODO: Verify length.
	const uint32_t exefs_offset = (le32_to_cpu(ncch_header.hdr.exefs_offset) << media_unit_shift);
	if (exefs_offset >= 16) {
		// Read the ExeFS header.
		size = readFromROM(exefs_offset, &exefs_header, sizeof(exefs_header));
		if (size != sizeof(exefs_header)) {
			// Read error.
			// NOTE: readFromROM() sets q->m_lastError.
			this->file = nullptr;
			return;
		}

		headers_loaded |= HEADER_EXEFS;
	}

#ifdef ENABLE_DECRYPTION
	if (!(ncch_header.hdr.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto)) {
		// Initialize the AES cipher.
		// TODO: Check for errors.
		cipher_ncch = AesCipherFactory::create();
		cipher_ncch->setChainingMode(IAesCipher::CM_CTR);
		ctr_t ctr;

		if (headers_loaded & HEADER_EXEFS) {
			// Decrypt the ExeFS header.
			// ExeFS header uses ncchKey0.
			cipher_ncch->setKey(ncch_keys[0].u8, sizeof(ncch_keys[0].u8));
			init_ctr(&ctr, N3DS_NCCH_SECTION_EXEFS, 0);
			cipher_ncch->setIV(ctr.u8, sizeof(ctr.u8));
			cipher_ncch->decrypt(reinterpret_cast<uint8_t*>(&exefs_header), sizeof(exefs_header));
		}

		// Initialize encrypted section handling.
		// Reference: https://github.com/profi200/Project_CTR/blob/master/makerom/ncch.c
		// Encryption details:
		// - ExHeader: ncchKey0, N3DS_NCCH_SECTION_EXHEADER
		// - acexDesc (TODO): ncchKey0, N3DS_NCCH_SECTION_EXHEADER
		// - ExeFS:
		//   - Header, "icon" and "banner": ncchKey0, N3DS_NCCH_SECTION_EXEFS
		//   - Other files: ncchKey1, N3DS_NCCH_SECTION_EXEFS
		// - RomFS (TODO): ncchKey1, N3DS_NCCH_SECTION_ROMFS

		// ExHeader
		encSections.push_back(EncSection(
			sizeof(N3DS_NCCH_Header_t),	// Address within NCCH.
			sizeof(N3DS_NCCH_Header_t),	// Counter base address.
			le32_to_cpu(ncch_header.hdr.exheader_size),
			0, N3DS_NCCH_SECTION_EXHEADER));

		if (headers_loaded & HEADER_EXEFS) {
			// ExeFS header
			encSections.push_back(EncSection(
				exefs_offset,	// Address within NCCH.
				exefs_offset,	// Counter base address.
				sizeof(N3DS_ExeFS_Header_t),
				0, N3DS_NCCH_SECTION_EXEFS));

			// ExeFS files
			for (int i = 0; i < ARRAY_SIZE(exefs_header.files); i++) {
				const N3DS_ExeFS_File_Header_t *file_header = &exefs_header.files[i];
				if (file_header->name[0] == 0)
					continue;	// or break?

				uint8_t keyIdx;
				if (!strncmp(file_header->name, "icon", sizeof(file_header->name)) ||
				    !strncmp(file_header->name, "banner", sizeof(file_header->name))) {
					// Icon and Banner use key 0.
					keyIdx = 0;
				} else {
					// All other files use key 1.
					keyIdx = 1;
				}

				encSections.push_back(EncSection(
					exefs_offset + sizeof(exefs_header) +	// Address within NCCH.
						le32_to_cpu(file_header->offset),
					exefs_offset,				// Counter base address.
					le32_to_cpu(file_header->size),
					keyIdx, N3DS_NCCH_SECTION_EXEFS));
			}
		}

		// RomFS
		if (le32_to_cpu(ncch_header.hdr.romfs_size) != 0) {
			const uint32_t romfs_offset = (le32_to_cpu(ncch_header.hdr.romfs_offset) << media_unit_shift);
			encSections.push_back(EncSection(
				romfs_offset,	// Address within NCCH.
				romfs_offset,	// Counter base address.
				(le32_to_cpu(ncch_header.hdr.romfs_size) << media_unit_shift),
				0, N3DS_NCCH_SECTION_ROMFS));
		}

		// Sort encSections by NCCH-relative address.
		// TODO: Check for overlap?
		std::sort(encSections.begin(), encSections.end());
	}
#endif /* ENABLE_DECRYPTION */
}

NCCHReaderPrivate::~NCCHReaderPrivate()
{
#ifdef ENABLE_DECRYPTION
	delete cipher_ncch;
	delete cipher_cia;
#endif /* ENABLE_DECRYPTION */
}

#ifdef ENABLE_DECRYPTION
/**
 * Find the encrypted section containing a given address.
 * @param address Address.
 * @return Index in encSections, or -1 if not encrypted.
 */
int NCCHReaderPrivate::findEncSection(uint32_t address) const
{
	for (int i = 0; i < (int)encSections.size(); i++) {
		const auto &section = encSections.at(i);
		if (address >= section.address &&
		    address < section.address + section.length)
		{
			// Found the section.
			return i;
		}
	}

	// Not an encrypted section.
	return -1;
}
#endif /* ENABLE_DECRYPTION */

/**
 * Read data from the underlying ROM image.
 * CIA decryption is automatically handled if set up properly.
 *
 * NOTE: Offset and size must both be multiples of 16.
 *
 * @param offset	[in] Starting address, relative to the beginning of the NCCH.
 * @param ptr		[out] Output buffer.
 * @param size		[in] Amount of data to read.
 * @return Number of bytes read, or 0 on error.
 */
size_t NCCHReaderPrivate::readFromROM(uint32_t offset, void *ptr, size_t size)
{
	assert(ptr != nullptr);
	assert(offset % 16 == 0);
	assert(size % 16 == 0);
	RP_Q(NCCHReader);
	if (!ptr || offset % 16 != 0 || size % 16 != 0) {
		// Invalid parameters.
		q->m_lastError = EINVAL;
		return 0;
	} else if (size == 0) {
		// Nothing to do.
		return 0;
	}

	int64_t phys_addr = ncch_offset + offset;

#ifdef ENABLE_DECRYPTION
	ctr_t cia_iv;
	if (cipher_cia) {
		// CIA decryption is required.
		if (offset >= 16) {
			// Subtract 16 in order to read the IV.
			offset -= 16;
		}
		int ret = file->seek(phys_addr);
		if (ret != 0) {
			// Seek error.
			q->m_lastError = file->lastError();
			if (q->m_lastError == 0) {
				q->m_lastError = EIO;
			}
			return 0;
		}

		if (offset == 0) {
			// Start of CIA content.
			// IV is the TMD content index.
			cia_iv.u8[0] = tmd_content_index >> 8;
			cia_iv.u8[1] = tmd_content_index & 0xFF;
			memset(&cia_iv.u8[2], 0, sizeof(cia_iv.u8)-2);
		} else {
			// IV is the previous 16 bytes.
			// TODO: Cache this?
			size_t sz_read = file->read(&cia_iv.u8, sizeof(cia_iv.u8));
			if (sz_read != sizeof(cia_iv.u8)) {
				// Read error.
				q->m_lastError = file->lastError();
				if (q->m_lastError == 0) {
					q->m_lastError = EIO;
				}
				return 0;
			}
		}
	} else
#endif /* ENABLE_DECRYPTION */
	{
		// No CIA decryption is needed.
		// Seek directly to the start of the data.
		int ret = file->seek(phys_addr);
		if (ret != 0) {
			// Seek error.
			q->m_lastError = file->lastError();
			if (q->m_lastError == 0) {
				q->m_lastError = EIO;
			}
			return 0;
		}
	}

	// Read the data.
	size_t sz_read = file->read(ptr, size);
	if (sz_read != size) {
		// Short read.
		q->m_lastError = file->lastError();
#ifdef ENABLE_DECRYPTION
		if (cipher_cia) {
			// Cannot decrypt with a short read.
			if (q->m_lastError == 0) {
				q->m_lastError = EIO;
			}
			return 0;
		}
#endif /* ENABLE_DECRYPTION */
	}

#ifdef ENABLE_DECRYPTION
	if (cipher_cia) {
		// Decrypt the data.
		int ret = cipher_cia->setIV(cia_iv.u8, sizeof(cia_iv.u8));
		if (ret != 0) {
			// setIV() failed.
			q->m_lastError = EIO;
			return -EIO;
		}
		unsigned int sz_dec = cipher_cia->decrypt(
			static_cast<uint8_t*>(ptr), (unsigned int)size);
		if (sz_dec != size) {
			// decrypt() failed.
			q->m_lastError = EIO;
			return 0;
		}
	}
#endif /* ENABLE_DECRYPTION */

	// Data read successfully.
	return sz_read;
}

/**
 * Load the NCCH Extended Header.
 * @return 0 on success; non-zero on error.
 */
int NCCHReaderPrivate::loadExHeader(void)
{
	if (headers_loaded & HEADER_EXHEADER) {
		// NCCH Extended Header is already loaded.
		return 0;
	}

	// TODO: Load the NCCH header if it isn't loaded?
	if (!(headers_loaded & HEADER_NCCH)) {
		// NCCH header wasn't loaded.
		return -1;
	}

	RP_Q(NCCHReader);
	if (!file || !file->isOpen()) {
		// File isn't open...
		q->m_lastError = EBADF;
		return -2;
	}

	// NOTE: Using NCCHReader functions instead of direct
	// file access, so all addresses are relative to the
	// start of the NCCH.

	// Check the ExHeader length.
	uint32_t exheader_length = le32_to_cpu(ncch_header.hdr.exheader_size);
	if (exheader_length < N3DS_NCCH_EXHEADER_MIN_SIZE ||
	    exheader_length > sizeof(ncch_exheader))
	{
		// ExHeader is either too small or too big.
		q->m_lastError = EIO;
		return -3;
	}

	// Round up exheader_length to the nearest 16 bytes for decryption purposes.
	exheader_length = (exheader_length + 15) & ~15;

	// Load the ExHeader.
	// ExHeader is stored immediately after the main header.
	int64_t prev_pos = q->tell();
	int ret = q->seek(sizeof(N3DS_NCCH_Header_t));
	if (ret != 0) {
		// Seek error.
		q->m_lastError = file->lastError();
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		q->seek(prev_pos);
		return -4;
	}
	size_t size = q->read(&ncch_exheader, exheader_length);
	if (size != exheader_length) {
		// Read error.
		q->m_lastError = file->lastError();
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		q->seek(prev_pos);
		return -5;
	}

	// If the ExHeader size is smaller than the maximum size,
	// clear the rest of the ExHeader.
	// TODO: Possible strict aliasing violation...
	if (exheader_length < sizeof(ncch_exheader)) {
		uint8_t *exzero = reinterpret_cast<uint8_t*>(&ncch_exheader) + exheader_length;
		memset(exzero, 0, sizeof(ncch_exheader) - exheader_length);
	}

	// TODO: Verify the ExHeader SHA256.
	// For now, reject it if some fields are invalid, since this
	// usually means it's encrypted with a key that isn't available.
	if (ncch_exheader.aci.arm11_local.res_limit_category > N3DS_NCCH_EXHEADER_ACI_ResLimit_Categry_OTHER) {
		// Invalid application type.
		return -6;
	}
	const uint8_t old3ds_sys_mode = (ncch_exheader.aci.arm11_local.flags[2] &
		N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Mask) >> 4;
	if (old3ds_sys_mode > N3DS_NCCH_EXHEADER_ACI_FLAG2_Old3DS_SysMode_Dev4) {
		// Invalid Old3DS system mode.
		return -7;
	}
	const uint8_t new3ds_sys_mode = ncch_exheader.aci.arm11_local.flags[1] &
		N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Mask;
	if (new3ds_sys_mode > N3DS_NCCH_EXHEADER_ACI_FLAG1_New3DS_SysMode_Dev2) {
		// Invalid New3DS system mode.
		return -8;
	}
	
	// ExHeader loaded.
	q->seek(prev_pos);
	headers_loaded |= HEADER_EXHEADER;
	return 0;
}

/** NCCHReader **/

/**
 * Construct an NCCHReader with the specified IRpFile.
 *
 * NOTE: The IRpFile *must* remain valid while this
 * NCCHReader is open.
 *
 * @param file 			[in] IRpFile.
 * @param media_unit_shift	[in] Media unit shift.
 * @param ncch_offset		[in] NCCH start offset, in bytes.
 * @param ncch_length		[in] NCCH length, in bytes.
 * @param ticket		[in,opt] Ticket for CIA decryption. (nullptr if NoCrypto)
 * @param tmd_content_index	[in,opt] TMD content index for CIA decryption.
 */
NCCHReader::NCCHReader(IRpFile *file, uint8_t media_unit_shift,
		int64_t ncch_offset, uint32_t ncch_length,
		const N3DS_Ticket_t *ticket,
		uint16_t tmd_content_index)
	: d_ptr(new NCCHReaderPrivate(this, file, media_unit_shift, ncch_offset, ncch_length, ticket, tmd_content_index))
{ }

NCCHReader::~NCCHReader()
{
	delete d_ptr;
}

/** IDiscReader **/

/**
 * Is the partition open?
 * This usually only returns false if an error occurred.
 * @return True if the partition is open; false if it isn't.
 */
bool NCCHReader::isOpen(void) const
{
	RP_D(NCCHReader);
	return (d->file && d->file->isOpen());
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t NCCHReader::read(void *ptr, size_t size)
{
	RP_D(NCCHReader);
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
	if (d->pos >= d->ncch_length)
		return 0;

	// Make sure d->pos + size <= d->ncch_length.
	// If it isn't, we'll do a short read.
	if (d->pos + (int64_t)size >= d->ncch_length) {
		size = (size_t)(d->ncch_length - d->pos);
	}

	if (d->ncch_header.hdr.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto) {
		// No NCCH encryption.
		// NOTE: readFromROM() sets q->m_lastError, so we
		// don't need to check if a short read occurred.
		return d->readFromROM(d->pos, ptr, size);
	}

#ifdef ENABLE_DECRYPTION
	// TODO: Handle reads that aren't a multiple of 16 bytes.
	assert(d->pos % 16 == 0);
	assert(size % 16 == 0);
	if (d->pos % 16 != 0 || size % 16 != 0) {
		// Cannot read now.
		return 0;
	}

	// Seek to the starting position.
	int ret = d->file->seek(d->ncch_offset + d->pos);
	if (ret != 0) {
		// Seek error.
		m_lastError = d->file->lastError();
		return 0;
	}

	uint8_t *ptr8 = static_cast<uint8_t*>(ptr);
	size_t sz_total_read = 0;
	while (size > 0) {
		// Determine what section we're in.
		int sectIdx = d->findEncSection(d->pos);
		const NCCHReaderPrivate::EncSection *section = (
			sectIdx >= 0 ? &d->encSections.at(sectIdx) : nullptr);

		size_t sz_to_read = 0;
		if (!section) {
			// Not in an encrypted section.
			// TODO: Find the next encrypted section.
			// For now, assuming the rest is plaintext.
			sz_to_read = size;
			assert(!"Reading in an unencrypted section...");
		} else {
			// We're in an encrypted section.
			uint32_t section_offset = (uint32_t)(d->pos - section->address);
			if (section_offset + size <= section->length) {
				// Remainder of reading is in this section.
				sz_to_read = size;
			} else {
				// We're reading past the end of this section.
				sz_to_read = section->length - section_offset;
			}
		}

		// Read from the ROM image.
		// This automatically removes the outer CIA
		// title key encryption if it's present.
		size_t ret_sz = d->readFromROM(d->pos, ptr8, sz_to_read);

		if (section) {
			// Set the required key.
			// TODO: Don't set the key if it hasn't changed.
			d->cipher_ncch->setKey(d->ncch_keys[section->keyIdx].u8, sizeof(d->ncch_keys[section->keyIdx].u8));

			// Initialize the counter based on section and offset.
			NCCHReaderPrivate::ctr_t ctr;
			d->init_ctr(&ctr, section->section, d->pos - section->ctr_base);
			d->cipher_ncch->setIV(ctr.u8, sizeof(ctr.u8));

			// Decrypt the data.
			// FIXME: Round up to 16 if a short read occurred?
			ret_sz = d->cipher_ncch->decrypt(static_cast<uint8_t*>(ptr), (unsigned int)ret_sz);
		}

		d->pos += (uint32_t)ret_sz;
		ptr8 += ret_sz;
		sz_total_read += ret_sz;
		size -= ret_sz;
		if (d->pos > d->ncch_length) {
			d->pos = d->ncch_length;
			break;
		}
		if (ret_sz != sz_to_read) {
			// Short read.
			break;
		}
	}

	return sz_total_read;
#else /* !ENABLE_DECRYPTION */
	// Decryption is not enabled.
	return 0;
#endif /* ENABLE_DECRYPTION */
}

/**
 * Set the partition position.
 * @param pos Partition position.
 * @return 0 on success; -1 on error.
 */
int NCCHReader::seek(int64_t pos)
{
	RP_D(NCCHReader);
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
	else if (pos >= d->ncch_length)
		d->pos = d->ncch_length;
	else
		d->pos = (uint32_t)pos;
	return 0;
}

/**
 * Seek to the beginning of the partition.
 */
void NCCHReader::rewind(void)
{
	seek(0);
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
int64_t NCCHReader::tell(void)
{
	RP_D(NCCHReader);
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
int64_t NCCHReader::size(void)
{
	// TODO: Errors?
	RP_D(const NCCHReader);
	const int64_t ret = d->ncch_length - sizeof(N3DS_NCCH_Header_t);
	return (ret >= 0 ? ret : 0);
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
int64_t NCCHReader::partition_size(void) const
{
	// TODO: Errors?
	RP_D(const NCCHReader);
	return d->ncch_length;
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
int64_t NCCHReader::partition_size_used(void) const
{
	// TODO: Errors?
	// NOTE: For NCCHReader, this is the same as partition_size().
	RP_D(const NCCHReader);
	return d->ncch_length;
}

/** NCCHReader **/

/**
 * Get the NCCH header.
 * @return NCCH header, or nullptr if it couldn't be loaded.
 */
const N3DS_NCCH_Header_NoSig_t *NCCHReader::ncchHeader(void) const
{
	RP_D(const NCCHReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
		//m_lastError = EBADF;
		return nullptr;
	}

	if (!(d->headers_loaded & NCCHReaderPrivate::HEADER_NCCH)) {
		// NCCH header wasn't loaded.
		// TODO: Try to load it here?
		return nullptr;
	}

	return &d->ncch_header.hdr;
}

/**
 * Get the NCCH extended header.
 * @return NCCH extended header, or nullptr if it couldn't be loaded.
 */
const N3DS_NCCH_ExHeader_t *NCCHReader::ncchExHeader(void) const
{
	RP_D(const NCCHReader);
	if (!(d->headers_loaded & NCCHReaderPrivate::HEADER_EXHEADER)) {
		if (const_cast<NCCHReaderPrivate*>(d)->loadExHeader() != 0) {
			// Unable to load the ExHeader.
			return nullptr;
		}
	}

	return &d->ncch_exheader;
}

/**
 * Get the ExeFS header.
 * @return ExeFS header, or nullptr if it couldn't be loaded.
 */
const N3DS_ExeFS_Header_t *NCCHReader::exefsHeader(void) const
{
	RP_D(const NCCHReader);
	if (!(d->headers_loaded & NCCHReaderPrivate::HEADER_EXEFS)) {
		// ExeFS header wasn't loaded.
		// TODO: Try to load it here?
		return nullptr;
	}

	// TODO: Check if the ExeFS header was actually loaded.
	return &d->exefs_header;
}

/**
 * Get the NCCH crypto type.
 * @param pCryptoType	[out] Crypto type.
 * @param pNcchHeader	[in] NCCH header.
 * @return 0 on success; negative POSIX error code on error.
 */
int NCCHReader::cryptoType_static(CryptoType *pCryptoType, const N3DS_NCCH_Header_NoSig_t *pNcchHeader)
{
	assert(pCryptoType != nullptr);
	assert(pNcchHeader != nullptr);
	if (!pCryptoType || !pNcchHeader) {
		// Invalid argument(s).
		return -EINVAL;
	}

	// References:
	// - https://github.com/d0k3/GodMode9/blob/master/source/game/ncch.c
	// - https://github.com/d0k3/GodMode9/blob/master/source/game/ncch.h
	if (pNcchHeader->flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto) {
		// No encryption.
		pCryptoType->name = "NoCrypto";
		pCryptoType->encrypted = false;
		pCryptoType->keyslot = 0xFF;
		pCryptoType->seed = false;
		return 0;
	}

	// Encryption is enabled.
	pCryptoType->encrypted = true;
	if (pNcchHeader->flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_FixedCryptoKey) {
		// Fixed key encryption.
		// NOTE: Keyslot 0x11 is used, but that keyslot
		// isn't permanently assigned, so we're not setting
		// it here.
		pCryptoType->keyslot = 0xFF;
		pCryptoType->seed = false;
		// NOTE: Using GodMode9's fixed keyset determination.
		if (le32_to_cpu(pNcchHeader->program_id.hi) & 0x10) {
			// Using the fixed debug key.
			pCryptoType->name = "Fixed (Debug)";
		} else {
			// Using zero-key.
			pCryptoType->name = "Fixed (Zero)";
		}
		return 0;
	}

	// Check ncchflag[3].
	switch (pNcchHeader->flags[N3DS_NCCH_FLAG_CRYPTO_METHOD]) {
		case 0x00:
			pCryptoType->name = "Standard";
			pCryptoType->keyslot = 0x2C;
			break;
		case 0x01:
			pCryptoType->name = "v7.x";
			pCryptoType->keyslot = 0x25;
			break;
		case 0x0A:
			pCryptoType->name = "Secure3";
			pCryptoType->keyslot = 0x18;
			break;
		case 0x0B:
			pCryptoType->name = "Secure4";
			pCryptoType->keyslot = 0x1B;
			break;
		default:
			// TODO: Unknown encryption method...
			assert(!"Unknown NCCH encryption method.");
			pCryptoType->name = nullptr;
			pCryptoType->keyslot = 0xFF;
			break;
	}

	// Is SEED encryption in use?
	pCryptoType->seed = !!(pNcchHeader->flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_Fw96KeyY);

	// We're done here.
	return 0;
}

/**
 * Get the NCCH crypto type.
 * @param pCryptoType	[out] Crypto type.
 * @return 0 on success; negative POSIX error code on error.
 */
int NCCHReader::cryptoType(CryptoType *pCryptoType) const
{
	assert(pCryptoType != nullptr);
	if (!pCryptoType) {
		// Invalid argument.
		return -EINVAL;
	}

	RP_D(const NCCHReader);
	if (!(d->headers_loaded & NCCHReaderPrivate::HEADER_NCCH)) {
		// NCCH header wasn't loaded.
		// TODO: Try to load it here?
		return -EIO;
	}
	return cryptoType_static(pCryptoType, &d->ncch_header.hdr);
}

/**
 * Encryption key verification result.
 * @return Encryption key verification result.
 */
KeyManager::VerifyResult NCCHReader::verifyResult(void) const
{
	RP_D(const NCCHReader);
	return d->verifyResult;
}

/**
 * Get the content type as a string.
 * @return Content type, or nullptr on error.
 */
const rp_char *NCCHReader::contentType(void) const
{
	const N3DS_NCCH_Header_NoSig_t *ncch_header = ncchHeader();
	if (!ncch_header) {
		// NCCH header is not loaded.
		return nullptr;
	}

	const rp_char *content_type;
	const uint8_t ctype_flag = ncch_header->flags[N3DS_NCCH_FLAG_CONTENT_TYPE];
	if ((ctype_flag & N3DS_NCCH_CONTENT_TYPE_Child) == N3DS_NCCH_CONTENT_TYPE_Child) {
		// DLP child
		content_type = _RP("Download Play");
	} else if (ctype_flag & N3DS_NCCH_CONTENT_TYPE_Trial) {
		// Demo
		content_type = _RP("Demo");
	} else if (ctype_flag & N3DS_NCCH_CONTENT_TYPE_Executable) {
		// CXI
		content_type = _RP("CXI");
	} else if (ctype_flag & N3DS_NCCH_CONTENT_TYPE_Manual) {
		// Manual
		content_type = _RP("Manual");
	} else if (ctype_flag & N3DS_NCCH_CONTENT_TYPE_SystemUpdate) {
		// System Update
		content_type = _RP("Update");
	} else if (ctype_flag & N3DS_NCCH_CONTENT_TYPE_Data) {
		// CFA
		content_type = _RP("CFA");
	} else {
		// Unknown.
		content_type = nullptr;
	}
	return content_type;
}

/**
 * Open a file. (read-only)
 *
 * NOTE: Only ExeFS is currently supported.
 *
 * @param section NCCH section.
 * @param filename Filename. (ASCII)
 * @return IRpFile*, or nullptr on error.
 */
IRpFile *NCCHReader::open(int section, const char *filename)
{
	RP_D(NCCHReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	assert(section == N3DS_NCCH_SECTION_EXEFS);
	assert(filename != nullptr);
	if (!d->file || !d->file->isOpen()) {
		m_lastError = EBADF;
		return nullptr;
	} else if (section != N3DS_NCCH_SECTION_EXEFS) {
		// Only ExeFS is currently supported.
		m_lastError = ENOTSUP;
		return nullptr;
	} else if (!filename) {
		// Invalid filename.
		m_lastError = EINVAL;
		return nullptr;
	}

	// Get the ExeFS header.
	const N3DS_ExeFS_Header_t *const exefs_header = exefsHeader();
	if (!exefs_header) {
		// Unable to get the ExeFS header.
		return nullptr;
	}

	const N3DS_ExeFS_File_Header_t *file_header = nullptr;
	for (int i = 0; i < ARRAY_SIZE(exefs_header->files); i++) {
		if (!strncmp(exefs_header->files[i].name, filename, sizeof(exefs_header->files[i].name))) {
			// Found "icon".
			file_header = &exefs_header->files[i];
			break;
		}
	}
	if (!file_header) {
		// File not found.
		m_lastError = ENOENT;
		return nullptr;
	}

	// Get the file offset.
	const uint32_t offset = (le32_to_cpu(d->ncch_header.hdr.exefs_offset) << d->media_unit_shift) +
		sizeof(d->exefs_header) +
		le32_to_cpu(file_header->offset);
	const uint32_t size = le32_to_cpu(file_header->size);
	if (offset >= d->ncch_length ||
	    ((int64_t)offset + size) > d->ncch_length)
	{
		// File offset/size is out of bounds.
		m_lastError = EIO;	// TODO: Better error code?
		return nullptr;
	}

	// TODO: Reference count opened PartitionFiles and
	// add assertions if they aren't closed correctly.

	// Create the PartitionFile.
	// This is an IRpFile implementation that uses an
	// IPartition as the reader and takes an offset
	// and size as the file parameters.
	return new PartitionFile(this, offset, size);
}

}
