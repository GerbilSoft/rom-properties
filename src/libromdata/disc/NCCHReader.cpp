/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NCCHReader.cpp: Nintendo 3DS NCCH reader.                               *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "librpbase/config.librpbase.h"
#include "NCCHReader.hpp"

// librpbase
#include "librpbase/file/IRpFile.hpp"
#include "librpbase/disc/PartitionFile.hpp"
#ifdef ENABLE_DECRYPTION
#include "librpbase/crypto/AesCipherFactory.hpp"
#include "librpbase/crypto/IAesCipher.hpp"
#endif /* ENABLE_DECRYPTION */
using namespace LibRpBase;

// CIA Reader.
#include "disc/CIAReader.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <algorithm>

#include "NCCHReader_p.hpp"
namespace LibRomData {

/** NCCHReaderPrivate **/

NCCHReaderPrivate::NCCHReaderPrivate(NCCHReader *q,
	uint8_t media_unit_shift,
	int64_t ncch_offset, uint32_t ncch_length)
	: q_ptr(q)
	, ncch_offset(ncch_offset)
	, ncch_length(ncch_length)
	, media_unit_shift(media_unit_shift)
	, pos(0)
	, headers_loaded(0)
	, verifyResult(KeyManager::VERIFY_UNKNOWN)
#ifdef ENABLE_DECRYPTION
	, tid_be(0)
	, cipher(nullptr)
	, tmd_content_index(0)
	, isDebug(false)
#endif /* ENABLE_DECRYPTION */
{
	// Clear the various structs.
	memset(&ncch_header, 0, sizeof(ncch_header));
	memset(&ncch_exheader, 0, sizeof(ncch_exheader));
	memset(&exefs_header, 0, sizeof(exefs_header));

	// Read the NCCH header.
	// We're including the signature, since the first 16 bytes
	// are used for encryption in certain cases.
	size_t size = readFromROM(0, &ncch_header, sizeof(ncch_header));
	if (size != sizeof(ncch_header)) {
		// Read error.
		// NOTE: readFromROM() sets q->m_lastError.
		// TODO: Better verifyResult?
		verifyResult = KeyManager::VERIFY_WRONG_KEY;
		closeFileOrDiscReader();
		return;
	}

	// Verify the NCCH magic number.
	if (ncch_header.hdr.magic != cpu_to_be32(N3DS_NCCH_HEADER_MAGIC)) {
		// NCCH magic number is incorrect.
		// Check for non-NCCH types.
		if (ncch_header.hdr.magic == cpu_to_be32('NDHT')) {
			// NDHT. (DS Whitelist)
			// 0004800F-484E4841
			verifyResult = KeyManager::VERIFY_OK;
			nonNcchContentType = NONCCH_NDHT;
			closeFileOrDiscReader();
			return;
		}

		const uint32_t magic_narc = ((const uint32_t*)&ncch_header)[0x80/4];
		if (magic_narc == cpu_to_be32('NARC')) {
			// NARC. (TWL Version Data)
			// 0004800F-484E4C41
			verifyResult = KeyManager::VERIFY_OK;
			nonNcchContentType = NONCCH_NARC;
		} else {
			// TODO: Better verifyResult? (May be DSiWare...)
			verifyResult = KeyManager::VERIFY_WRONG_KEY;
			if (q->m_lastError == 0) {
				q->m_lastError = EIO;
			}
		}

		closeFileOrDiscReader();
		return;
	}
	headers_loaded |= HEADER_NCCH;

#ifdef ENABLE_DECRYPTION
	// Byteswap the title ID. (It's used for the AES counter.)
	// FIXME: Verify this on big-endian.
	tid_be = __swab64(ncch_header.hdr.program_id.id);

	// Determine the keyset to use.
	// NOTE: Assuming Retail by default. Will fall back to
	// Debug if ExeFS header decryption fails.
	verifyResult = N3DSVerifyKeys::loadNCCHKeys(ncch_keys, &ncch_header, N3DS_TICKET_TITLEKEY_ISSUER_RETAIL);
	if (verifyResult != KeyManager::VERIFY_OK) {
		// Failed to load the keyset.
		// Try debug keys instead.
		verifyResult = N3DSVerifyKeys::loadNCCHKeys(ncch_keys, &ncch_header,
			N3DS_TICKET_TITLEKEY_ISSUER_DEBUG);
		if (verifyResult != KeyManager::VERIFY_OK) {
			// Debug keys didn't work.
			// Zero out the keys.
			memset(ncch_keys, 0, sizeof(ncch_keys));
			q->m_lastError = EIO;
			closeFileOrDiscReader();
			return;
		}
		// Debug keys worked.
		isDebug = true;
	}
#else /* !ENABLE_DECRYPTION */
	// Decryption is not available, so only NoCrypto is allowed.
	if (!(ncch_header.hdr.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto)) {
		// Unsupported.
		verifyResult = KeyManager::VERIFY_NO_SUPPORT;
		q->m_lastError = EIO;
		closeFileOrDiscReader();
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
			closeFileOrDiscReader();
			return;
		}

		headers_loaded |= HEADER_EXEFS;
	}

#ifdef ENABLE_DECRYPTION
	if (!(ncch_header.hdr.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto)) {
		// Initialize the AES cipher.
		// TODO: Check for errors.
		cipher = AesCipherFactory::create();
		cipher->setChainingMode(IAesCipher::CM_CTR);
		u128_t ctr;

		if (headers_loaded & HEADER_EXEFS) {
			// Decrypt the ExeFS header.
			// ExeFS header uses ncchKey0.
			cipher->setKey(ncch_keys[0].u8, sizeof(ncch_keys[0].u8));
			ctr.init_ctr(tid_be, N3DS_NCCH_SECTION_EXEFS, 0);
			cipher->setIV(ctr.u8, sizeof(ctr.u8));
			cipher->decrypt(reinterpret_cast<uint8_t*>(&exefs_header), sizeof(exefs_header));

			// For CXI: First file should be ".code".
			// For CFA: First file should be "icon".
			const char *filename_chk = nullptr;
			const uint8_t ctype_flag = ncch_header.hdr.flags[N3DS_NCCH_FLAG_CONTENT_TYPE];
			if (ctype_flag & N3DS_NCCH_CONTENT_TYPE_Executable) {
				filename_chk = ".code";
			} else if (ctype_flag & N3DS_NCCH_CONTENT_TYPE_Data) {
				filename_chk = "icon";
			}
			if (!filename_chk) {
				// No filename to check...
				memset(ncch_keys, 0, sizeof(ncch_keys));
				q->m_lastError = EIO;
				delete cipher;
				cipher = nullptr;
				closeFileOrDiscReader();
				return;
			}

			// Check the first filename.
			if (strcmp(exefs_header.files[0].name, filename_chk) != 0) {
				if (isDebug) {
					// We already tried both sets.
					// Zero out the keys.
					memset(ncch_keys, 0, sizeof(ncch_keys));
					q->m_lastError = EIO;
					delete cipher;
					cipher = nullptr;
					closeFileOrDiscReader();
					return;
				}

				// Retail keys failed.
				// Try again with debug keys.
				// TODO: Consolidate this code.
				verifyResult = N3DSVerifyKeys::loadNCCHKeys(ncch_keys, &ncch_header,
					N3DS_TICKET_TITLEKEY_ISSUER_DEBUG);
				if (verifyResult != KeyManager::VERIFY_OK) {
					// Failed to load the keyset.
					// Zero out the keys.
					memset(ncch_keys, 0, sizeof(ncch_keys));
					q->m_lastError = EIO;
					delete cipher;
					cipher = nullptr;
					closeFileOrDiscReader();
					return;
				}

				// Reload the ExeFS header.
				size = readFromROM(exefs_offset, &exefs_header, sizeof(exefs_header));
				if (size != sizeof(exefs_header)) {
					// Read error.
					// NOTE: readFromROM() sets q->m_lastError.
					delete cipher;
					cipher = nullptr;
					closeFileOrDiscReader();
					return;
				}

				// Decrypt the ExeFS header.
				// ExeFS header uses ncchKey0.
				cipher->setKey(ncch_keys[0].u8, sizeof(ncch_keys[0].u8));
				ctr.init_ctr(tid_be, N3DS_NCCH_SECTION_EXEFS, 0);
				cipher->setIV(ctr.u8, sizeof(ctr.u8));
				cipher->decrypt(reinterpret_cast<uint8_t*>(&exefs_header), sizeof(exefs_header));

				// Check the first filename, again.
				if (strcmp(exefs_header.files[0].name, filename_chk) != 0) {
					// Still not usable.
					delete cipher;
					q->m_lastError = EIO;
					cipher = nullptr;
					closeFileOrDiscReader();
					return;
				}

				// We're using debug keys.
				isDebug = true;
			}
		}

		// Initialize encrypted section handling.
		// Reference: https://github.com/profi200/Project_CTR/blob/master/makerom/ncch.c
		// Encryption details:
		// - ExHeader: ncchKey0, N3DS_NCCH_SECTION_EXHEADER
		// - acexDesc (TODO): ncchKey0, N3DS_NCCH_SECTION_EXHEADER
		// - Logo: Plaintext (SDK5+ only)
		// - ExeFS:
		//   - Header, "icon" and "banner": ncchKey0, N3DS_NCCH_SECTION_EXEFS
		//   - Other files: ncchKey1, N3DS_NCCH_SECTION_EXEFS
		// - RomFS (TODO): ncchKey1, N3DS_NCCH_SECTION_ROMFS

		// Logo (SDK5+)
		// NOTE: This is plaintext, but read() doesn't work properly
		// unless a section is defined. Use N3DS_NCCH_SECTION_PLAIN
		// to indicate this.
		const uint32_t logo_region_size = le32_to_cpu(ncch_header.hdr.logo_region_size) << media_unit_shift;
		if (logo_region_size > 0) {
			const uint32_t logo_region_offset = le32_to_cpu(ncch_header.hdr.logo_region_offset) << media_unit_shift;
			encSections.push_back(EncSection(
				logo_region_offset,	// Address within NCCH.
				logo_region_offset,		// Counter base address.
				logo_region_size,
				0, N3DS_NCCH_SECTION_PLAIN));
		}

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
		if (ncch_header.hdr.romfs_size != cpu_to_le32(0)) {
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
	delete cipher;
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
	for (int i = static_cast<int>(encSections.size())-1; i >= 0; i--) {
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

	// Seek to the start of the data and read it.
	const int64_t phys_addr = ncch_offset + offset;
	size_t sz_read;
	if (q->m_hasDiscReader) {
		sz_read = q->m_discReader->seekAndRead(phys_addr, ptr, size);
	} else {
		sz_read = q->m_file->seekAndRead(phys_addr, ptr, size);
	}
	if (sz_read != size) {
		// Seek and/or read error.
		q->m_lastError = (q->m_hasDiscReader ? q->m_discReader->lastError() : q->m_file->lastError());
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
	}

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
	if (!q->isOpen()) {
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
	exheader_length = ALIGN_BYTES(16, exheader_length);

	// Load the ExHeader.
	// ExHeader is stored immediately after the main header.
	int64_t prev_pos = q->tell();
	q->m_lastError = 0;
	size_t size = q->seekAndRead(sizeof(N3DS_NCCH_Header_t),
				     &ncch_exheader, exheader_length);
	if (size != exheader_length) {
		// Seek and/or read error.
		q->m_lastError = q->m_file->lastError();
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
 * @param file 			[in] IRpFile. (for CCIs only)
 * @param media_unit_shift	[in] Media unit shift.
 * @param ncch_offset		[in] NCCH start offset, in bytes.
 * @param ncch_length		[in] NCCH length, in bytes.
 */
NCCHReader::NCCHReader(IRpFile *file, uint8_t media_unit_shift,
		int64_t ncch_offset, uint32_t ncch_length)
	: super(file)
	, d_ptr(new NCCHReaderPrivate(this, media_unit_shift, ncch_offset, ncch_length))
{ }

/**
 * Construct an NCCHReader with the specified CIAReader.
 *
 * NOTE: The NCCHReader *takes ownership* of the CIAReader.
 * This makes it easier to create a temporary CIAReader
 * without worrying about keeping track of it.
 *
 * @param ciaReader		[in] CIAReader. (for CIAs only)
 * @param media_unit_shift	[in] Media unit shift.
 * @param ncch_offset		[in] NCCH start offset, in bytes.
 * @param ncch_length		[in] NCCH length, in bytes.
 * @param ticket		[in,opt] Ticket for CIA decryption. (nullptr if NoCrypto)
 * @param tmd_content_index	[in,opt] TMD content index for CIA decryption.
 */
NCCHReader::NCCHReader(CIAReader *ciaReader, uint8_t media_unit_shift,
		int64_t ncch_offset, uint32_t ncch_length)
	: super(ciaReader)
	, d_ptr(new NCCHReaderPrivate(this, media_unit_shift, ncch_offset, ncch_length))
{ }

NCCHReader::~NCCHReader()
{
	delete d_ptr;

	if (m_hasDiscReader) {
		// Delete the IDiscReader, since it's
		// most likely a temporary CIAReader.
		// TODO: Use reference counting?
		delete m_discReader;
	}
}

/** IDiscReader **/

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
	assert(isOpen());
	if (!ptr) {
		m_lastError = EINVAL;
		return 0;
	} else if (!isOpen()) {
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
	if (d->pos + static_cast<int64_t>(size) >= d->ncch_length) {
		size = static_cast<size_t>(d->ncch_length - d->pos);
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

	uint8_t *ptr8 = static_cast<uint8_t*>(ptr);
	size_t sz_total_read = 0;
	while (size > 0) {
		// Determine what section we're in.
		int sectIdx = d->findEncSection(d->pos);
		const NCCHReaderPrivate::EncSection *section = (
			sectIdx >= 0 ? &d->encSections.at(sectIdx) : nullptr);

		size_t sz_to_read = 0;
		if (!section) {
			// Not in a defined section.
			// TODO: Handle this?
			assert(!"Reading in an undefined section.");
			return sz_total_read;
		} else {
			// We're in an encrypted section.
			uint32_t section_offset = static_cast<uint32_t>(d->pos - section->address);
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

		if (section && section->section > N3DS_NCCH_SECTION_PLAIN) {
			// Set the required key.
			// TODO: Don't set the key if it hasn't changed.
			d->cipher->setKey(d->ncch_keys[section->keyIdx].u8, sizeof(d->ncch_keys[section->keyIdx].u8));

			// Initialize the counter based on section and offset.
			u128_t ctr;
			ctr.init_ctr(d->tid_be, section->section, d->pos - section->ctr_base);
			d->cipher->setIV(ctr.u8, sizeof(ctr.u8));

			// Decrypt the data.
			// FIXME: Round up to 16 if a short read occurred?
			ret_sz = d->cipher->decrypt(static_cast<uint8_t*>(ptr), ret_sz);
		}

		d->pos += static_cast<uint32_t>(ret_sz);
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
	assert(isOpen());
	if (!isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	// Handle out-of-range cases.
	if (pos < 0) {
		// Negative is invalid.
		m_lastError = EINVAL;
		return -1;
	} else if (pos >= d->ncch_length) {
		d->pos = d->ncch_length;
	} else {
		d->pos = static_cast<uint32_t>(pos);
	}
	return 0;
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
int64_t NCCHReader::tell(void)
{
	RP_D(const NCCHReader);
	assert(isOpen());
	if (!isOpen()) {
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
	if (!isOpen()) {
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
		if (pNcchHeader->program_id.hi & cpu_to_le32(0x10)) {
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

#ifdef ENABLE_DECRYPTION
/**
 * Are we using debug keys?
 * @return True if using debug keys; false if not.
 */
bool NCCHReader::isDebug(void) const
{
	RP_D(const NCCHReader);
	return d->isDebug;
}
#endif /* ENABLE_DECRYPTION */

/**
 * Get the content type as a string.
 * @return Content type, or nullptr on error.
 */
const char *NCCHReader::contentType(void) const
{
	const char *content_type;

	const N3DS_NCCH_Header_NoSig_t *ncch_header = ncchHeader();
	if (!ncch_header) {
		// NCCH header is not loaded.
		// Check if this is another content type.
		RP_D(const NCCHReader);
		switch (d->nonNcchContentType) {
			case NCCHReaderPrivate::NONCCH_NDHT:
				// NDHT (DS Whitelist)
				content_type = "NDHT";
				break;
			case NCCHReaderPrivate::NONCCH_NARC:
				// NARC (TWL Version Data)
				content_type = "NARC";
				break;
			default:
				content_type = nullptr;
				break;
		}
		return content_type;
	}

	const uint8_t ctype_flag = ncch_header->flags[N3DS_NCCH_FLAG_CONTENT_TYPE];
	if ((ctype_flag & N3DS_NCCH_CONTENT_TYPE_Child) == N3DS_NCCH_CONTENT_TYPE_Child) {
		// DLP child
		content_type = "Download Play";
	} else if (ctype_flag & N3DS_NCCH_CONTENT_TYPE_Trial) {
		// Demo
		content_type = "Demo";
	} else if (ctype_flag & N3DS_NCCH_CONTENT_TYPE_Executable) {
		// CXI
		content_type = "CXI";
	} else if (ctype_flag & N3DS_NCCH_CONTENT_TYPE_Manual) {
		// Manual
		content_type = "Manual";
	} else if (ctype_flag & N3DS_NCCH_CONTENT_TYPE_SystemUpdate) {
		// System Update
		content_type = "Update";
	} else if (ctype_flag & N3DS_NCCH_CONTENT_TYPE_Data) {
		// CFA
		content_type = "CFA";
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
	RP_D(const NCCHReader);
	assert(isOpen());
	assert(section == N3DS_NCCH_SECTION_EXEFS);
	assert(filename != nullptr);
	if (!isOpen()) {
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
	    (static_cast<int64_t>(offset) + size) > d->ncch_length)
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

/**
 * Open the logo section.
 *
 * For CXIs compiled with pre-SDK5, opens the "logo" file in ExeFS.
 * Otherwise, this opens the separate logo section.
 *
 * @return IRpFile*, or nullptr on error.
 */
LibRpBase::IRpFile *NCCHReader::openLogo(void)
{
	RP_D(const NCCHReader);
	assert(isOpen());
	if (!isOpen()) {
		m_lastError = EBADF;
		return nullptr;
	}

	// Check if the dedicated logo section is present.
	const uint32_t logo_region_size = le32_to_cpu(d->ncch_header.hdr.logo_region_size) << d->media_unit_shift;
	if (logo_region_size > 0) {
		// Dedicated logo section is present.
		return new PartitionFile(this,
			le32_to_cpu(d->ncch_header.hdr.logo_region_offset) << d->media_unit_shift,
			logo_region_size);
	}

	// Pre-SDK5. Load the "logo" file from ExeFS.
	return this->open(N3DS_NCCH_SECTION_EXEFS, "logo");
}

}
