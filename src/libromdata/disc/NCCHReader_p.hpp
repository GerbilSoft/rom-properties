/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NCCHReader_p.hpp: Nintendo 3DS NCCH reader.                             *
 * Private class declaration.                                              *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_NCCHREADER_P_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_NCCHREADER_P_HPP__

#include "NCCHReader.hpp"
#include "config.libromdata.h"

#include "byteswap.h"
#include "n3ds_structs.h"

#ifdef ENABLE_DECRYPTION
#include "crypto/KeyManager.hpp"
#include "crypto/CtrKeyScrambler.hpp"
#endif /* ENABLE_DECRYPTION */

// C includes.
#include <stdint.h>

// C++ includes.
#include <vector>

namespace LibRomData {

#ifdef ENABLE_DECRYPTION
class IAesCipher;
#endif /* ENABLE_DECRYPTION */

class NCCHReaderPrivate
{
	public:
		NCCHReaderPrivate(NCCHReader *q, IRpFile *file,
			uint8_t media_unit_shift,
			int64_t ncch_offset, uint32_t ncch_length,
			const N3DS_Ticket_t *ticket = nullptr,
			uint16_t tmd_content_index = 0);
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
		N3DS_NCCH_Header_t ncch_header;
		// NCCH ExHeader.
		N3DS_NCCH_ExHeader_t ncch_exheader;
		// ExeFS header.
		N3DS_ExeFS_Header_t exefs_header;

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
		size_t readFromROM(uint32_t offset, void *ptr, size_t size);

		/**
		 * Load the NCCH Extended Header.
		 * @return 0 on success; non-zero on error.
		 */
		int loadExHeader(void);

#ifdef ENABLE_DECRYPTION
		// Title ID. Used for AES-CTR initialization.
		// (Big-endian format)
		uint64_t tid_be;

		// Encryption keys.
		// TODO: Use correct key index depending on file.
		// For now, only supporting NoCrypto and FixedCryptoKey
		// with a zero key.
		uint8_t ncch_keys[2][16];

		// NCCH cipher.
		IAesCipher *cipher_ncch;	// NCCH cipher.

		// CIA cipher.
		uint8_t title_key[16];		// Decrypted title key.
		IAesCipher *cipher_cia;		// CIA cipher.

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
			uint32_t address;	// Relative to ncch_offset.
			uint32_t ctr_base;	// Base address for the AES-CTR counter.
			uint32_t length;
			uint8_t keyIdx;		// ncch_keys[] index
			uint8_t section;	// N3DS_NCCH_Sections

			EncSection(uint32_t address, uint32_t ctr_base,
				uint32_t length, uint8_t keyIdx, uint8_t section)
				: address(address)
				, ctr_base(ctr_base)
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
		std::vector<EncSection> encSections;

		/**
		 * Find the encrypted section containing a given address.
		 * @param address Address.
		 * @return Index in encSections, or -1 if not encrypted.
		 */
		int findEncSection(uint32_t address) const;

		// KeyY index for title key encryption. (CIA only)
		uint8_t titleKeyEncIdx;
		// TMD content index.
		uint16_t tmd_content_index;

		/**
		 * Attempt to load an AES normal key.
		 * @param pKeyOut		[out] Output key data.
		 * @param keyNormal_name	[in,opt] KeyNormal slot name.
		 * @param keyX_name		[in,opt] KeyX slot name.
		 * @param keyY_name		[in,opt] KeyY slot name.
		 * @param keyNormal_verify	[in,opt] KeyNormal verification data. (NULL or 16 bytes)
		 * @param keyX_verify		[in,opt] KeyX verification data. (NULL or 16 bytes)
		 * @param keyY_verify		[in,opt] KeyY verification data. (NULL or 16 bytes)
		 * @return VerifyResult.
		 */
		KeyManager::VerifyResult loadKeyNormal(u128_t *pKeyOut,
			const char *keyNormal_name,
			const char *keyX_name,
			const char *keyY_name,
			const uint8_t *keyNormal_verify,
			const uint8_t *keyX_verify,
			const uint8_t *keyY_verify);

		/**
		 * Verification data for debug Slot0x3DKeyX.
		 * This is the string "AES-128-ECB-TEST"
		 * encrypted using the key with AES-128-ECB.
		 */
		static const uint8_t verifyData_ctr_dev_Slot0x3DKeyX[16];

		/**
		 * Verification data for debug Slot0x3DKeyX.
		 * This is the string "AES-128-ECB-TEST"
		 * encrypted using the key with AES-128-ECB.
		 * Primary index is ticket->keyY_index.
		 */
		static const uint8_t verifyData_ctr_dev_Slot0x3DKeyY_tbl[6][16];

		/**
		 * Verification data for debug Slot0x3DKeyNormal.
		 * This is the string "AES-128-ECB-TEST"
		 * encrypted using the key with AES-128-ECB.
		 * Primary index is ticket->keyY_index.
		 */
		static const uint8_t verifyData_ctr_dev_Slot0x3DKeyNormal_tbl[6][16];
#endif /* ENABLE_DECRYPTION */
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_NCCHREADER_P_HPP__ */
