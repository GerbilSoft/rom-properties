/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NCCHReader.hpp: Nintendo 3DS NCCH reader.                               *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "../Handheld/n3ds_structs.h"

// librpbase
#include "librpbase/disc/IPartition.hpp"
#include "librpbase/crypto/KeyManager.hpp"

namespace LibRomData {

class CIAReader;

class NCCHReaderPrivate;
class NCCHReader final : public LibRpBase::IPartition
{
	public:
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
		NCCHReader(const std::shared_ptr<LibRpFile::IRpFile> &file,
			uint8_t media_unit_shift,
			off64_t ncch_offset, uint32_t ncch_length);

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
		 */
		NCCHReader(CIAReader *ciaReader,
			uint8_t media_unit_shift,
			off64_t ncch_offset, uint32_t ncch_length);
	protected:
		~NCCHReader() final;	// call unref() instead

	private:
		typedef IPartition super;
		RP_DISABLE_COPY(NCCHReader)

	protected:
		friend class NCCHReaderPrivate;
		NCCHReaderPrivate *const d_ptr;

	public:
		/** IDiscReader **/

		/**
		 * Read data from the partition.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		ATTR_ACCESS_SIZE(write_only, 2, 3)
		size_t read(void *ptr, size_t size) final;

		/**
		 * Set the partition position.
		 * @param pos Partition position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(off64_t pos) final;

		/**
		 * Get the partition position.
		 * @return Partition position on success; -1 on error.
		 */
		off64_t tell(void) final;

		/**
		 * Get the data size.
		 * This size does not include the partition header,
		 * and it's adjusted to exclude hashes.
		 * @return Data size, or -1 on error.
		 */
		off64_t size(void) final;

	public:
		/** IPartition **/

		/**
		 * Get the partition size.
		 * This size includes the partition header and hashes.
		 * @return Partition size, or -1 on error.
		 */
		off64_t partition_size(void) const final;

		/**
		 * Get the used partition size.
		 * This size includes the partition header and hashes,
		 * but does not include "empty" sectors.
		 * @return Used partition size, or -1 on error.
		 */
		off64_t partition_size_used(void) const final;

	public:
		/** NCCHReader **/

		/**
		 * Get the NCCH header.
		 * @return NCCH header, or nullptr if it couldn't be loaded.
		 */
		const N3DS_NCCH_Header_NoSig_t *ncchHeader(void) const;

		/**
		 * Get the NCCH extended header.
		 * @return NCCH extended header, or nullptr if it couldn't be loaded.
		 */
		const N3DS_NCCH_ExHeader_t *ncchExHeader(void) const;

		/**
		 * Get the ExeFS header.
		 * @return ExeFS header, or nullptr if it couldn't be loaded.
		 */
		const N3DS_ExeFS_Header_t *exefsHeader(void) const;

		/**
		 * Crypto type information.
		 */
		struct CryptoType {
			const char *name;	// Crypto method name. (ASCII)
			bool encrypted;		// True if it's encrypted.
			uint8_t keyslot;	// Keyslot. (0x00-0x3F)
			bool seed;		// True if SEED encryption is in use.
		};

		/**
		 * Get the NCCH crypto type.
		 * @param pCryptoType	[out] Crypto type.
		 * @param pNcchHeader	[in] NCCH header.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int cryptoType_static(CryptoType *pCryptoType, const N3DS_NCCH_Header_NoSig_t *pNcchHeader);

		/**
		 * Get the NCCH crypto type.
		 * @param pCryptoType	[out] Crypto type.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int cryptoType(CryptoType *pCryptoType) const;

		/**
		 * Encryption key verification result.
		 * @return Encryption key verification result.
		 */
		LibRpBase::KeyManager::VerifyResult verifyResult(void) const;

		/*
		 * Are we forcing NoCrypto due to incorrect NCCH flags?
		 * This happens with some badly-decrypted NCSD images.
		 * @return True if forcing NoCrypto; false if not.
		 */
		bool isForceNoCrypto(void) const;

#ifdef ENABLE_DECRYPTION
		/**
		 * Are we using debug keys?
		 * @return True if using debug keys; false if not.
		 */
		bool isDebug(void) const;
#endif /* ENABLE_DECRYPTION */

		/**
		 * Get the content type as a string.
		 * @return Content type, or nullptr on error.
		 */
		const char *contentType(void) const;

		/**
		 * Open a file. (read-only)
		 *
		 * NOTE: Only ExeFS is currently supported.
		 *
		 * @param section NCCH section.
		 * @param filename Filename. (ASCII)
		 * @return IRpFile*, or nullptr on error.
		 */
		std::shared_ptr<LibRpFile::IRpFile> open(int section, const char *filename);

		/**
		 * Open the logo section.
		 *
		 * For CXIs compiled with pre-SDK5, opens the "logo" file in ExeFS.
		 * Otherwise, this opens the separate logo section.
		 *
		 * @return IRpFile*, or nullptr on error.
		 */
		std::shared_ptr<LibRpFile::IRpFile> openLogo(void);
};

}
