/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiPartition.hpp: Wii partition reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_WIIPARTITION_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_WIIPARTITION_HPP__

#include "librpbase/config.librpbase.h"
#include "GcnPartition.hpp"
#include "../Console/wii_structs.h"

// librpbase
#include "librpbase/crypto/KeyManager.hpp"

namespace LibRomData {

class WiiPartitionPrivate;
class WiiPartition : public GcnPartition
{
	public:
		// Bitfield enum indicating the encryption type.
		enum CryptoMethod {
			CM_ENCRYPTED = 0,	// Data is encrypted.
			CM_UNENCRYPTED = 1,	// Data is not encrypted.
			CM_MASK_ENCRYPTED = 1,

			CM_1K_31K = 0,		// 1k hashes, 31k data
			CM_32K = 2,		// 32k data
			CM_MASK_SECTOR = 2,

			CM_STANDARD = (CM_ENCRYPTED | CM_1K_31K),	// Standard encrypted Wii disc
			CM_RVTH = (CM_UNENCRYPTED | CM_32K),		// Unencrypted RVT-H disc image
			CM_NASOS = (CM_UNENCRYPTED | CM_1K_31K),	// NASOS compressed retail disc image
		};

		/**
		 * Construct a WiiPartition with the specified IDiscReader.
		 *
		 * NOTE: The IDiscReader *must* remain valid while this
		 * WiiPartition is open.
		 *
		 * @param discReader		[in] IDiscReader.
		 * @param partition_offset	[in] Partition start offset.
		 * @param partition_size	[in] Calculated partition size. Used if the size in the header is 0.
		 * @param cryptoMethod		[in] Crypto method.
		 */
		WiiPartition(IDiscReader *discReader, int64_t partition_offset,
			int64_t partition_size, CryptoMethod crypto = CM_STANDARD);
		~WiiPartition();

	private:
		typedef GcnPartition super;
		RP_DISABLE_COPY(WiiPartition)

	protected:
		friend class WiiPartitionPrivate;
		// d_ptr is used from the subclass.
		//WiiPartitionPrivate *const d_ptr;

	public:
		/** IDiscReader **/

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
		 * Get the partition position.
		 * @return Partition position on success; -1 on error.
		 */
		int64_t tell(void) final;

		/** WiiPartition **/

		/**
		 * Encryption key verification result.
		 * @return Encryption key verification result.
		 */
		LibRpBase::KeyManager::VerifyResult verifyResult(void) const;

		// Encryption key in use.
		// TODO: Merge with EncryptionKeys.
		enum EncKey {
			ENCKEY_UNKNOWN = -1,
			ENCKEY_COMMON = 0,	// Wii common key
			ENCKEY_KOREAN = 1,	// Korean key
			ENCKEY_VWII = 2,	// vWii common key
			ENCKEY_DEBUG = 3,	// RVT-R debug key
			ENCKEY_NONE = 4,	// No encryption (RVT-H)
		};

		/**
		 * Get the encryption key in use.
		 * @return Encryption key in use.
		 */
		EncKey encKey(void) const;

		/**
		 * Get the encryption key that would be in use if the partition was encrypted.
		 * This is only needed for NASOS images.
		 * @return "Real" encryption key in use.
		 */
		EncKey encKeyReal(void) const;

		/**
		 * Get the ticket.
		 * @return Ticket, or nullptr if unavailable.
		 */
		const RVL_Ticket *ticket(void) const;

		/**
		 * Get the TMD header.
		 * @return TMD header, or nullptr if unavailable.
		 */
		const RVL_TMD_Header *tmdHeader(void) const;

	public:
		// Encryption key indexes.
		enum EncryptionKeys {
			// Retail
			Key_Rvl_Common,
			Key_Rvl_Korean,
			Key_Wup_vWii_Common,
			Key_Rvl_SD_AES,
			Key_Rvl_SD_IV,
			Key_Rvl_SD_MD5,

			// Debug
			Key_Rvt_Debug,

			Key_Max
		};

#ifdef ENABLE_DECRYPTION
	public:
		/**
		 * Get the total number of encryption key names.
		 * @return Number of encryption key names.
		 */
		static int encryptionKeyCount_static(void);

		/**
		 * Get an encryption key name.
		 * @param keyIdx Encryption key index.
		 * @return Encryption key name (in ASCII), or nullptr on error.
		 */
		static const char *encryptionKeyName_static(int keyIdx);

		/**
		 * Get the verification data for a given encryption key index.
		 * @param keyIdx Encryption key index.
		 * @return Verification data. (16 bytes)
		 */
		static const uint8_t *encryptionVerifyData_static(int keyIdx);
#endif /* ENABLE_DECRYPTION */
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_WIIPARTITION_HPP__ */
