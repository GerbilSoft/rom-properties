/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * N3DSExeFS.cpp: Nintendo 3DS ExeFS reader.                               *
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

#include "N3DSExeFS.hpp"
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
#include <memory>
using std::unique_ptr;

namespace LibRomData {

class N3DSExeFSPrivate
{
	public:
		N3DSExeFSPrivate(N3DSExeFS *q, IRpFile *file,
			const N3DS_NCCH_Header_NoSig_t *ncch_header,
			int64_t offset, uint32_t length);
		~N3DSExeFSPrivate();

	private:
		RP_DISABLE_COPY(N3DSExeFSPrivate)
	protected:
		N3DSExeFS *const q_ptr;

	public:
		IRpFile *file;		// 3DS ROM image.

		// Offsets.
		int64_t fs_offset;	// ExeFS start offset, in bytes.
		uint32_t fs_length;	// ExeFS length, in bytes.

		// Current read position within the ExeFS.
		// pos = 0 indicates the beginning of the ExeFS header.
		// NOTE: This cannot be more than 4 GB,
		// so we're using uint32_t.
		uint32_t pos;

		// ExeFS header.
		N3DS_ExeFS_Header_t exefs_header;

		// NCCH flags.
		// See N3DS_NCCH_Flags for more information.
		uint8_t ncch_flags[8];

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
		 * @param offset Partition offset, in bytes.
		 */
		inline void init_ctr(ctr_t *ctr, uint32_t offset)
		{
			ctr->u64[0] = tid_be;
			ctr->u8[8] = N3DS_NCCH_SECTION_EXEFS;
			ctr->u8[9] = 0;
			ctr->u8[10] = 0;
			ctr->u8[11] = 0;
			offset /= 16;
			ctr->u32[3] = cpu_to_be32(offset);
		}
#endif /* ENABLE_DECRYPTION */
};

/** N3DSExeFSPrivate **/

N3DSExeFSPrivate::N3DSExeFSPrivate(N3DSExeFS *q, IRpFile *file,
	const N3DS_NCCH_Header_NoSig_t *ncch_header,
	int64_t offset, uint32_t length)
	: q_ptr(q)
	, file(file)
	, fs_offset(offset)
	, fs_length(length)
	, pos(0)
#ifdef ENABLE_DECRYPTION
	, cipher(nullptr)
#endif /* ENABLE_DECRYPTION */
{
	// Copy fields from the NCCH header.
	memcpy(ncch_flags, ncch_header->flags, sizeof(ncch_flags));
	tid_be = __swab64(ncch_header->program_id.id);

	// Determine the keyset to use.
	// Crypto settings, in priority order:
	// 1. NoCrypto: AES key is all 0s. (FixedCryptoKey should also be set.)
	// 2. FixedCryptoKey: Fixed key is used.
	// 3. Neither: Standard key is used.
#ifdef ENABLE_DECRYPTION
	if (ncch_flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto) {
		// No encryption.
		memset(ncch_keys, 0, sizeof(ncch_keys));
	} else if (ncch_flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_FixedCryptoKey) {
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
	if (!(ncch_flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto)) {
		// Unsupported.
		// TODO: Set an error like WiiPartition.
		q->m_lastError = EIO;
		this->file = nullptr;
		return;
	}
#endif /* ENABLE_DECRYPTION */

	// Load the ExeFS header.
	int ret = file->seek(fs_offset);
	if (ret != 0) {
		// Seek error.
		q->m_lastError = file->lastError();
		this->file = nullptr;
		return;
	}
	size_t size = file->read(&exefs_header, sizeof(exefs_header));
	if (size != sizeof(exefs_header)) {
		// Read error.
		q->m_lastError = file->lastError();
		this->file = nullptr;
		return;
	}

#ifdef ENABLE_DECRYPTION
	if (!(ncch_flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto)) {
		// Initialize the AES cipher.
		// TODO: Check for errors.
		cipher = AesCipherFactory::getInstance();
		cipher->setChainingMode(IAesCipher::CM_CTR);
		// TODO: Use Key1 if needed.
		cipher->setKey(ncch_keys[0], sizeof(ncch_keys[0]));

		// Decrypt the ExeFS header.
		ctr_t ctr;
		init_ctr(&ctr, 0);
		cipher->setIV(ctr.u8, sizeof(ctr.u8));
		cipher->decrypt(reinterpret_cast<uint8_t*>(&exefs_header), sizeof(exefs_header));
	}
#endif /* ENABLE_DECRYPTION */

	// ExeFS is ready.
}

N3DSExeFSPrivate::~N3DSExeFSPrivate()
{
#ifdef ENABLE_DECRYPTION
	delete cipher;
#endif /* ENABLE_DECRYPTION */
}

/** N3DSExeFS **/

/**
 * Construct an N3DSExeFS with the specified IRpFile.
 *
 * NOTE: The IRpFile *must* remain valid while this
 * N3DSExeFS is open.
 *
 * @param file IRpFile.
 * @param ncch_header NCCH header. Needed for encryption parameters.
 * @param offset ExeFS start offset, in bytes.
 * @param length ExeFS length, in bytes.
 */
N3DSExeFS::N3DSExeFS(IRpFile *file, const N3DS_NCCH_Header_NoSig_t *ncch_header,
	  int64_t offset, uint32_t length)
	: d_ptr(new N3DSExeFSPrivate(this, file, ncch_header, offset, length))
{ }

N3DSExeFS::~N3DSExeFS()
{
	delete d_ptr;
}

/** IDiscReader **/

/**
 * Is the partition open?
 * This usually only returns false if an error occurred.
 * @return True if the partition is open; false if it isn't.
 */
bool N3DSExeFS::isOpen(void) const
{
	RP_D(N3DSExeFS);
	return (d->file && d->file->isOpen());
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t N3DSExeFS::read(void *ptr, size_t size)
{
	RP_D(N3DSExeFS);
	assert(d->file != nullptr);
	assert(d->file->isOpen());
	if (!d->file || !d->file->isOpen()) {
		m_lastError = EBADF;
		return 0;
	}

	// Are we already at the end of the file?
	if (d->pos >= d->fs_length)
		return 0;

	// Make sure d->pos + size <= d->fs_length.
	// If it isn't, we'll do a short read.
	if (d->pos + (int64_t)size >= d->fs_length) {
		size = (size_t)(d->fs_length - d->pos);
	}

	if (d->ncch_flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto) {
		// No encryption. Read directly from the ExeFS.
		int ret = d->file->seek(d->fs_offset + d->pos);
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

	int ret = d->file->seek(d->fs_offset + d->pos);
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
	N3DSExeFSPrivate::ctr_t ctr;
	d->init_ctr(&ctr, d->pos);
	d->cipher->setIV(ctr.u8, sizeof(ctr.u8));
	ret_sz = d->cipher->decrypt(static_cast<uint8_t*>(ptr), (unsigned int)ret_sz);
	d->pos += (uint32_t)ret_sz;
	if (d->pos > d->fs_length) {
		d->pos = d->fs_length;
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
int N3DSExeFS::seek(int64_t pos)
{
	RP_D(N3DSExeFS);
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
	else if (pos >= d->fs_length)
		d->pos = d->fs_length;
	else
		d->pos = (uint32_t)pos;
	return 0;
}

/**
 * Seek to the beginning of the partition.
 */
void N3DSExeFS::rewind(void)
{
	seek(0);
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
int64_t N3DSExeFS::tell(void)
{
	RP_D(N3DSExeFS);
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
 * This size does not include the partition header,
 * and it's adjusted to exclude hashes.
 * @return Data size, or -1 on error.
 */
int64_t N3DSExeFS::size(void)
{
	// TODO: Errors?
	RP_D(const N3DSExeFS);
	const int64_t ret = d->fs_length - sizeof(N3DS_ExeFS_Header_t);
	return (ret >= 0 ? ret : 0);
}

/** IPartition **/

/**
 * Get the partition size.
 * This size includes the partition header and hashes.
 * @return Partition size, or -1 on error.
 */
int64_t N3DSExeFS::partition_size(void) const
{
	// TODO: Errors?
	RP_D(const N3DSExeFS);
	return d->fs_length;
}

/**
 * Get the used partition size.
 * This size includes the partition header and hashes,
 * but does not include "empty" sectors.
 * @return Used partition size, or -1 on error.
 */
int64_t N3DSExeFS::partition_size_used(void) const
{
	// TODO: Errors?
	// NOTE: For N3DSExeFS, this is the same as partition_size().
	RP_D(const N3DSExeFS);
	return d->fs_length;
}

}
