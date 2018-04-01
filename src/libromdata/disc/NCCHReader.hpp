/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NCCHReader.hpp: Nintendo 3DS NCCH reader.                               *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_NCCHREADER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_NCCHREADER_HPP__

#include "../Handheld/n3ds_structs.h"

// librpbase
#include "librpbase/disc/IPartition.hpp"
#include "librpbase/crypto/KeyManager.hpp"

namespace LibRpBase {
	class IRpFile;
	class IDiscReader;
}

namespace LibRomData {

class NCCHReaderPrivate;
class NCCHReader : public LibRpBase::IPartition
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
		NCCHReader(LibRpBase::IRpFile *file,
			uint8_t media_unit_shift,
			int64_t ncch_offset, uint32_t ncch_length);

		/**
		 * Construct an NCCHReader with the specified IDiscReader.
		 *
		 * NOTE: The NCCHReader *takes ownership* of the IDiscReader.
		 * This makes it easier to create a temporary CIAReader
		 * without worrying about keeping track of it.
		 *
		 * @param discReader		[in] IDiscReader. (for CCIs or CIAs)
		 * @param media_unit_shift	[in] Media unit shift.
		 * @param ncch_offset		[in] NCCH start offset, in bytes.
		 * @param ncch_length		[in] NCCH length, in bytes.
		 */
		NCCHReader(LibRpBase::IDiscReader *discReader,
			uint8_t media_unit_shift,
			int64_t ncch_offset, uint32_t ncch_length);
		virtual ~NCCHReader();

	private:
		typedef IPartition super;
		RP_DISABLE_COPY(NCCHReader)

	protected:
		friend class NCCHReaderPrivate;
		NCCHReaderPrivate *const d_ptr;

	public:
		/** IDiscReader **/

		/**
		 * Is the partition open?
		 * This usually only returns false if an error occurred.
		 * @return True if the partition is open; false if it isn't.
		 */
		bool isOpen(void) const final;

		/**
		 * Read data from the partition.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		size_t read(void *ptr, size_t size) final;

		/**
		 * Set the partition position.
		 * @param pos Partition position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(int64_t pos) final;

		/**
		 * Seek to the beginning of the partition.
		 */
		void rewind(void) final;

		/**
		 * Get the partition position.
		 * @return Partition position on success; -1 on error.
		 */
		int64_t tell(void) final;

		/**
		 * Get the data size.
		 * This size does not include the partition header,
		 * and it's adjusted to exclude hashes.
		 * @return Data size, or -1 on error.
		 */
		int64_t size(void) final;

	public:
		/** IPartition **/

		/**
		 * Get the partition size.
		 * This size includes the partition header and hashes.
		 * @return Partition size, or -1 on error.
		 */
		int64_t partition_size(void) const final;

		/**
		 * Get the used partition size.
		 * This size includes the partition header and hashes,
		 * but does not include "empty" sectors.
		 * @return Used partition size, or -1 on error.
		 */
		int64_t partition_size_used(void) const final;

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
		LibRpBase::IRpFile *open(int section, const char *filename);

		/**
		 * Open the logo section.
		 *
		 * For CXIs compiled with pre-SDK5, opens the "logo" file in ExeFS.
		 * Otherwise, this opens the separate logo section.
		 *
		 * @return IRpFile*, or nullptr on error.
		 */
		LibRpBase::IRpFile *openLogo(void);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_NCCHREADER_HPP__ */
