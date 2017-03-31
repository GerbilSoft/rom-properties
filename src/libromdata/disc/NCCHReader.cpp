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

#include "byteswap.h"
#include "n3ds_structs.h"

#include "file/IRpFile.hpp"

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

namespace LibRomData {

class NCCHReaderPrivate
{
	public:
		NCCHReaderPrivate(NCCHReader *q, IRpFile *file,
			uint8_t media_unit_shift,
			int64_t ncch_offset, uint32_t ncch_length);
		~NCCHReaderPrivate();

	private:
		RP_DISABLE_COPY(NCCHReaderPrivate)
	protected:
		NCCHReader *const q_ptr;

	public:
		IRpFile *file;		// 3DS ROM image.

		// NCCH offsets.
		const int64_t ncch_offset;	// NCCH start offset, in bytes.
		const uint32_t ncch_length;	// NCCH length, in bytes.
		const uint8_t media_unit_shift;

		// Current read position within the NCCH.
		// pos = 0 indicates the beginning of the NCCH header.
		// NOTE: This cannot be more than 4 GB,
		// so we're using uint32_t.
		uint32_t pos;

		// Loaded headers.
		enum HeadersPresent {
			HEADER_NONE	= 0,
			HEADER_NCCH	= (1 << 0),
			HEADER_EXHEADER	= (1 << 1),
			HEADER_EXEFS	= (1 << 2),
		};
		uint32_t headers_loaded;	// HeadersPresent

		// NCCH header.
		N3DS_NCCH_Header_NoSig_t ncch_header;
		// NCCH ExHeader.
		N3DS_NCCH_ExHeader_t ncch_exheader;
		// ExeFS header.
		N3DS_ExeFS_Header_t exefs_header;

#ifdef ENABLE_DECRYPTION
		// Title ID. Used for AES-CTR initialization.
		// (Big-endian format)
		uint64_t tid_be;

		// Encryption keys.
		// TODO: Use correct key index depending on file.
		// For now, only supporting NoCrypto and FixedCryptoKey
		// with a zero key.
		uint8_t ncch_keys[2][16];

		// AES cipher.
		// TODO: Move to N3DSFile, since it may use per-file values?
		IAesCipher *cipher;

		union ctr_t {
			uint8_t u8[16];
			uint32_t u32[4];
			uint64_t u64[2];
		};

		/**
		 * Initialize an AES-CTR counter using the Title ID.
		 * @param ctr AES-CTR counter.
		 * @param section NCCH section.
		 * @param offset Partition offset, in bytes.
		 */
		inline void init_ctr(ctr_t *ctr, uint8_t section, uint32_t offset)
		{
			ctr->u64[0] = tid_be;
			ctr->u8[8] = section;
			ctr->u8[9] = 0;
			ctr->u8[10] = 0;
			ctr->u8[11] = 0;
			offset /= 16;
			ctr->u32[3] = cpu_to_be32(offset);
		}

		// Encrypted section addresses.
		struct EncSection {
			uint32_t address;	// relative to ncch_offset
			uint32_t ctr_address;	// Counter relative address.
			uint32_t length;
			uint8_t keyIdx;		// ncch_keys[] index
			uint8_t section;	// N3DS_NCCH_Sections

			EncSection(uint32_t address, uint32_t ctr_address,
				uint32_t length, uint8_t keyIdx, uint8_t section)
				: address(address)
				, ctr_address(ctr_address)
				, length(length)
				, keyIdx(keyIdx)
				, section(section)
				{ }

			/**
			 * Comparison operator for std::sort().
			 */
			inline bool operator<(const EncSection& other) const
			{
				return (other.address < this->address);
			}
		};
		vector<EncSection> encSections;
#endif /* ENABLE_DECRYPTION */
};

/** NCCHReaderPrivate **/

NCCHReaderPrivate::NCCHReaderPrivate(NCCHReader *q, IRpFile *file,
	uint8_t media_unit_shift,
	int64_t ncch_offset, uint32_t ncch_length)
	: q_ptr(q)
	, file(file)
	, ncch_offset(ncch_offset)
	, ncch_length(ncch_length)
	, media_unit_shift(media_unit_shift)
	, pos(0)
	, headers_loaded(0)
#ifdef ENABLE_DECRYPTION
	, tid_be(0)
	, cipher(nullptr)
#endif /* ENABLE_DECRYPTION */
{
	// TODO: CIA outer decryption.

	// Read the NCCH header. (without the signature)
	int ret = file->seek(ncch_offset + 0x100);
	if (ret != 0) {
		// Seek error.
		this->file = nullptr;
		q->m_lastError = file->lastError();
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		return;
	}
	size_t size = file->read(&ncch_header, sizeof(ncch_header));
	if (size != sizeof(ncch_header)) {
		// Read error.
		this->file = nullptr;
		q->m_lastError = file->lastError();
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		return;
	}
	headers_loaded |= HEADER_NCCH;

#ifdef ENABLE_DECRYPTION
	// Byteswap the title ID. (It's used for the AES counter.)
	tid_be = __swab64(ncch_header.program_id.id);
#endif /* ENABLE_DECRYPTION */

	// Determine the keyset to use.
	// Crypto settings, in priority order:
	// 1. NoCrypto: AES key is all 0s. (FixedCryptoKey should also be set.)
	// 2. FixedCryptoKey: Fixed key is used.
	// 3. Neither: Standard key is used.
#ifdef ENABLE_DECRYPTION
	if (ncch_header.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto) {
		// No encryption.
		memset(ncch_keys, 0, sizeof(ncch_keys));
	} else if (ncch_header.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_FixedCryptoKey) {
		// Fixed key encryption.
		// TODO: Determine which keyset is in use.
		// For now, assuming TEST. (Zero-key) [FBI.3ds uses this]
		memset(ncch_keys, 0, sizeof(ncch_keys));
	} else {
		// TODO: Other encryption methods.
		q->m_lastError = EIO;
		this->file = nullptr;
		return;
	}
#else /* !ENABLE_DECRYPTION */
	// Decryption is not available, so only NoCrypto is allowed.
	if (!(ncch_header.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto)) {
		// Unsupported.
		// TODO: Set an error like WiiPartition.
		q->m_lastError = EIO;
		this->file = nullptr;
		return;
	}
#endif /* ENABLE_DECRYPTION */

	// TODO: If ExeFS size is 0, no ExeFS is present.
	// Need to indicate missing ExHeader, ExeFS, RomFS, etc.

	// Load the ExHeader region.
	// ExHeader is stored immediately after the main header.
	// TODO: Verify sizes; other error checking.
	const int64_t exheader_offset = ncch_offset + sizeof(N3DS_NCCH_Header_t);
	uint32_t exheader_length = le32_to_cpu(ncch_header.exheader_size);
	if (exheader_length < N3DS_NCCH_EXHEADER_MIN_SIZE ||
	    exheader_length > sizeof(ncch_exheader))
	{
		// ExHeader is either too small or too big.
		q->m_lastError = -EIO;
		this->file = nullptr;
		return;
	}

	// Round up exheader_length to the nearest 16 bytes for decryption purposes.
	exheader_length = (exheader_length + 15) & ~15;

	ret = file->seek(exheader_offset);
	if (ret != 0) {
		// Seek error.
		q->m_lastError = file->lastError();
		this->file = nullptr;
		return;
	}
	size = file->read(&ncch_exheader, exheader_length);
	if (size != exheader_length) {
		// Read error.
		q->m_lastError = file->lastError();
		this->file = nullptr;
		return;
	}

	// If the ExHeader size is smaller than the maximum size,
	// clear the rest of the ExHeader.
	// TODO: Possible strict aliasing violation...
	if (exheader_length < sizeof(ncch_exheader)) {
		uint8_t *exzero = reinterpret_cast<uint8_t*>(&ncch_exheader) + exheader_length;
		memset(exzero, 0, sizeof(ncch_exheader) - exheader_length);
	}

	// Load the ExeFS header.
	// TODO: Verify length.
	const uint32_t exefs_offset = (le32_to_cpu(ncch_header.exefs_offset) << media_unit_shift);
	ret = file->seek(ncch_offset + exefs_offset);
	if (ret != 0) {
		// Seek error.
		q->m_lastError = file->lastError();
		this->file = nullptr;
		return;
	}
	size = file->read(&exefs_header, sizeof(exefs_header));
	if (size != sizeof(exefs_header)) {
		// Read error.
		q->m_lastError = file->lastError();
		this->file = nullptr;
		return;
	}

#ifdef ENABLE_DECRYPTION
	if (!(ncch_header.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto)) {
		// Initialize the AES cipher.
		// TODO: Check for errors.
		cipher = AesCipherFactory::getInstance();
		cipher->setChainingMode(IAesCipher::CM_CTR);
		// ExHeader and ExeFS header both use ncchKey0.
		cipher->setKey(ncch_keys[0], sizeof(ncch_keys[0]));

		// Decrypt the NCCH ExHeader.
		ctr_t ctr;
		init_ctr(&ctr, N3DS_NCCH_SECTION_EXHEADER, 0);
		cipher->setIV(ctr.u8, sizeof(ctr.u8));
		cipher->decrypt(reinterpret_cast<uint8_t*>(&ncch_exheader), sizeof(ncch_exheader));
		headers_loaded |= HEADER_EXHEADER;

		// Decrypt the ExeFS header.
		init_ctr(&ctr, N3DS_NCCH_SECTION_EXEFS, 0);
		cipher->setIV(ctr.u8, sizeof(ctr.u8));
		cipher->decrypt(reinterpret_cast<uint8_t*>(&exefs_header), sizeof(exefs_header));
		headers_loaded |= HEADER_EXEFS;

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
			sizeof(N3DS_NCCH_Header_t), 0,
			le32_to_cpu(ncch_header.exheader_size),
			0, N3DS_NCCH_SECTION_EXHEADER));

		// ExeFS header
		encSections.push_back(EncSection(
			exefs_offset, 0,
			(le32_to_cpu(ncch_header.exefs_size) << media_unit_shift),
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
				le32_to_cpu(file_header->offset),	// Counter address.
				le32_to_cpu(file_header->size),
				keyIdx, N3DS_NCCH_SECTION_EXEFS));
		}

		// RomFS
		encSections.push_back(EncSection(
			(le32_to_cpu(ncch_header.romfs_offset) << media_unit_shift), 0,
			(le32_to_cpu(ncch_header.romfs_size) << media_unit_shift),
			0, N3DS_NCCH_SECTION_ROMFS));

		// Sort encSections by NCCH-relative address.
		// TODO: Check for overlap?
		std::sort(encSections.begin(), encSections.end());
	} else
#endif /* ENABLE_DECRYPTION */
	{
		// ExHeader and ExeFS headers are loaded.
		headers_loaded |= (HEADER_EXHEADER | HEADER_EXEFS);
	}
}

NCCHReaderPrivate::~NCCHReaderPrivate()
{
#ifdef ENABLE_DECRYPTION
	delete cipher;
#endif /* ENABLE_DECRYPTION */
}

/** NCCHReader **/

/**
 * Construct an NCCHReader with the specified IRpFile.
 *
 * NOTE: The IRpFile *must* remain valid while this
 * NCCHReader is open.
 *
 * @param file IRpFile.
 * @param media_unit_shift Media unit shift.
 * @param ncch_offset NCCH start offset, in bytes.
 * @param ncch_length NCCH length, in bytes.
 */
NCCHReader::NCCHReader(IRpFile *file, uint8_t media_unit_shift,
		int64_t ncch_offset, uint32_t ncch_length)
	: d_ptr(new NCCHReaderPrivate(this, file, media_unit_shift, ncch_offset, ncch_length))
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
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
		m_lastError = EBADF;
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

	if (d->ncch_header.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto) {
		// No encryption. Read directly from the NCCH.
		int ret = d->file->seek(d->ncch_offset + d->pos);
		if (ret != 0) {
			// Seek error.
			m_lastError = d->file->lastError();
			return 0;
		}
		size_t ret_sz = d->file->read(ptr, size);
		if (ret_sz != size) {
			// Possible error occurred...
			m_lastError = d->file->lastError();
		}
		return ret_sz;
	}

#ifdef ENABLE_DECRYPTION
	// TODO: Handle reads of differently-encrypted areas.
	// For now, assuming ncch_keys[0] for everything.
	// Either need to move decryption down to N3DSFile,
	// or handle separate regions here.

	// TODO: Handle reads that aren't a multiple of 16 bytes.
	assert(d->pos % 16 == 0);
	assert(size % 16 == 0);
	if (d->pos % 16 != 0 || size % 16 != 0) {
		// Cannot read now.
		return 0;
	}

	int ret = d->file->seek(d->ncch_offset + d->pos);
	if (ret != 0) {
		// Seek error.
		m_lastError = d->file->lastError();
		return 0;
	}
	size_t ret_sz = d->file->read(ptr, size);
	if (ret_sz != size) {
		// Possible error occurred...
		m_lastError = d->file->lastError();
	}

	// Decrypt the data.
	// FIXME: Round up to 16 if a short read occurred?
	NCCHReaderPrivate::ctr_t ctr;
	d->init_ctr(&ctr, N3DS_NCCH_SECTION_EXEFS, d->pos);
	d->cipher->setIV(ctr.u8, sizeof(ctr.u8));
	ret_sz = d->cipher->decrypt(static_cast<uint8_t*>(ptr), (unsigned int)ret_sz);
	d->pos += (uint32_t)ret_sz;
	if (d->pos > d->ncch_length) {
		d->pos = d->ncch_length;
	}
	return ret_sz;
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

	return &d->ncch_header;
}

/**
 * Get the NCCH extended header.
 * @return NCCH extended header, or nullptr if it couldn't be loaded.
 */
const N3DS_NCCH_ExHeader_t *NCCHReader::ncchExHeader(void) const
{
	RP_D(const NCCHReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
		//m_lastError = EBADF;
		return nullptr;
	}

	if (!(d->headers_loaded & NCCHReaderPrivate::HEADER_EXHEADER)) {
		// ExHeader wasn't loaded.
		// TODO: Try to load it here?
		return nullptr;
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
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
		//m_lastError = EBADF;
		return nullptr;
	}

	if (!(d->headers_loaded & NCCHReaderPrivate::HEADER_EXEFS)) {
		// ExeFS header wasn't loaded.
		// TODO: Try to load it here?
		return nullptr;
	}

	// TODO: Check if the ExeFS header was actually loaded.
	return &d->exefs_header;
}

/**
 * Get the ExeFS data offset within the NCCH.
 * This is immediately after the ExeFS header.
 * TODO: Remove once open() is added.
 * @return ExeFS data offset, or 0 on error.
 */
uint32_t NCCHReader::exefsDataOffset(void) const
{
	RP_D(const NCCHReader);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
		//m_lastError = EBADF;
		return 0;
	}

	if (!(d->headers_loaded & NCCHReaderPrivate::HEADER_EXEFS)) {
		// ExeFS header wasn't loaded.
		// TODO: Try to load it here?
		return 0;
	}

	// TODO: Check if the ExeFS header was actually loaded.
	return (le32_to_cpu(d->ncch_header.exefs_offset) << d->media_unit_shift) + sizeof(d->exefs_header);
}

}
