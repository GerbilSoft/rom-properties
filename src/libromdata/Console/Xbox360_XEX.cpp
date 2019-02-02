/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox360_XEX.cpp: Microsoft Xbox 360 executable reader.                  *
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

#include "librpbase/config.librpbase.h"
#include "libromdata/config.libromdata.h"

#include "Xbox360_XEX.hpp"
#include "Xbox360_XDBF.hpp"
#include "../Other/EXE.hpp"
#include "librpbase/RomData_p.hpp"

#include "xbox360_xex_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/disc/CBCReader.hpp"
#include "librpbase/disc/PartitionFile.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "librpbase/file/RpMemFile.hpp"
using namespace LibRpBase;

#ifdef ENABLE_DECRYPTION
# include "librpbase/crypto/IAesCipher.hpp"
# include "librpbase/crypto/AesCipherFactory.hpp"
# include "librpbase/crypto/KeyManager.hpp"
#endif /* ENABLE_DECRYPTION */

#ifdef ENABLE_LIBMSPACK
# include "mspack.h"
# include "xenia_lzx.h"
#endif /* ENABLE_LIBMSPACK */

// libi18n
#include "libi18n/i18n.h"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
using std::ostringstream;
using std::string;
using std::unique_ptr;
using std::vector;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData {

ROMDATA_IMPL(Xbox360_XEX)

// Workaround for RP_D() expecting the no-underscore naming convention.
#define Xbox360_XEXPrivate Xbox360_XEX_Private

class Xbox360_XEX_Private : public RomDataPrivate
{
	public:
		Xbox360_XEX_Private(Xbox360_XEX *q, IRpFile *file);
		virtual ~Xbox360_XEX_Private();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(Xbox360_XEX_Private)

	public:
		// XEX headers.
		// NOTE: Only xex2Header is byteswapped, except for the magic number.
		XEX2_Header xex2Header;
		XEX2_Security_Info xex2Security;

		// Optional header table.
		// NOTE: This array of structs **IS NOT** byteswapped!
		ao::uvector<XEX2_Optional_Header_Tbl> optHdrTbl;

		// Resource information. (XEX2_OPTHDR_RESOURCE_INFO)
		// Initialized by getXdbfResInfo().
		// NOTE: This struct **IS** byteswapped.
		XEX2_Resource_Info resInfo;

		// File format info. (XEX2_OPTHDR_FILE_FORMAT_INFO)
		// Initialized by initPeReader().
		// NOTE: This struct **IS** byteswapped.
		XEX2_File_Format_Info fileFormatInfo;

		// Encryption key in use.
		// If fileFormatInfo indicates the PE is encrypted:
		// - -1: Unknown
		// -  0: Retail
		// -  1: Debug
		// NOTE: We can't use EncryptionKeys because the debug key
		// is all zeroes, so we're not handling it here.
		int keyInUse;

		// Basic compression: Data segments.
		struct BasicZDataSeg_t {
			uint32_t vaddr;		// Virtual address in memory (base address is 0)
			uint32_t physaddr;	// Physical address in the PE executable
			uint32_t length;	// Length of segment
		};
		ao::uvector<BasicZDataSeg_t> basicZDataSegments;

		// Amount of data we'll read for the PE header.
		static const unsigned int PE_HEADER_SIZE = 8192;

#ifdef ENABLE_LIBMSPACK
		// Decompressed EXE header.
		ao::uvector<uint8_t> lzx_peHeader;
		// Decompressed XDBF section.
		ao::uvector<uint8_t> lzx_xdbfSection;
#endif /* ENABLE_LIBMSPACK */

		/**
		 * Get the specified optional header table entry.
		 * @param header_id Optional header ID.
		 * @return Optional header table entry, or nullptr if not found.
		 */
		const XEX2_Optional_Header_Tbl *getOptHdrTblEntry(uint32_t header_id) const;

		/**
		 * Get the resource information.
		 * @return Resource information, or nullptr on error.
		 */
		const XEX2_Resource_Info *getXdbfResInfo(void);

		/**
		 * Convert game ratings from Xbox 360 format to RomFields format.
		 * @param age_ratings RomFields::age_ratings_t
		 * @param game_ratings XEX2_Game_Ratings
		 */
		static void convertGameRatings(RomFields::age_ratings_t &age_ratings,
			const XEX2_Game_Ratings &game_ratings);

	public:
		// CBC reader for encrypted PE executables.
		// Also used for unencrypted executables.
		CBCReader *peReader;
		IRpFile *peFile_exe;	// uses peReader or lzx_peHeader
		EXE *pe_exe;		// uses peFile_exe
		IRpFile *peFile_xdbf;	// uses peReader or lzx_xdbfSection
		Xbox360_XDBF *pe_xdbf;	// uses peFile_xdbf

		/**
		 * Initialize the PE executable reader.
		 * @return peReader on success; nullptr on error.
		 */
		CBCReader *initPeReader(void);

		/**
		 * Initialize the EXE object.
		 * @return EXE object on success; nullptr on error.
		 */
		const EXE *initEXE(void);

		/**
		 * Initialize the Xbox360_XDBF object.
		 * @return Xbox360_XDBF object on success; nullptr on error.
		 */
		const Xbox360_XDBF *initXDBF(void);

#ifdef ENABLE_DECRYPTION
	public:
		// Verification key names.
		static const char *const EncryptionKeyNames[Xbox360_XEX::Key_Max];

		// Verification key data.
		static const uint8_t EncryptionKeyVerifyData[Xbox360_XEX::Key_Max][16];
#endif
};

#ifdef ENABLE_DECRYPTION
// Verification key names.
const char *const Xbox360_XEX_Private::EncryptionKeyNames[Xbox360_XEX::Key_Max] = {
	// Retail
	"xbox360-xex-retail",
};

const uint8_t Xbox360_XEXPrivate::EncryptionKeyVerifyData[Xbox360_XEX::Key_Max][16] = {
	/** Retail **/

	// xbox360-xex-retail
	{0xAC,0xA0,0xC9,0xE3,0x78,0xD3,0xC6,0x54,
	 0xA3,0x1D,0x65,0x67,0x38,0xAB,0xB0,0x6B},
};
#endif /* ENABLE_DECRYPTION */

/** Xbox360_XEX_Private **/

Xbox360_XEX_Private::Xbox360_XEX_Private(Xbox360_XEX *q, IRpFile *file)
	: super(q, file)
	, keyInUse(-1)
	, peReader(nullptr)
	, peFile_exe(nullptr)
	, pe_exe(nullptr)
	, peFile_xdbf(nullptr)
	, pe_xdbf(nullptr)
{
	// Clear the headers.
	memset(&xex2Header, 0, sizeof(xex2Header));
	memset(&xex2Security, 0, sizeof(xex2Security));
	memset(&resInfo, 0, sizeof(resInfo));
	memset(&fileFormatInfo, 0, sizeof(fileFormatInfo));
}

Xbox360_XEX_Private::~Xbox360_XEX_Private()
{
	if (pe_xdbf) {
		pe_xdbf->unref();
	}
	if (pe_exe) {
		pe_exe->unref();
	}

	delete peFile_xdbf;
	delete peFile_exe;
	delete peReader;
}

/**
 * Get the specified optional header table entry.
 * @param header_id Optional header ID.
 * @return Optional header table entry, or nullptr if not found.
 */
const XEX2_Optional_Header_Tbl *Xbox360_XEX_Private::getOptHdrTblEntry(uint32_t header_id) const
{
	if (optHdrTbl.empty()) {
		// No optional headers...
		return nullptr;
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the ID to make it easier to find things.
	header_id = cpu_to_be32(header_id);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// Search for the header.
	for (auto iter = optHdrTbl.cbegin(); iter != optHdrTbl.cend(); ++iter) {
		if (iter->header_id == header_id) {
			// Found the header.
			return &(*iter);
		}
	}

	// Not found.
	return nullptr;
}

/**
 * Get the resource information.
 * @return Resource information, or nullptr on error.
 */
const XEX2_Resource_Info *Xbox360_XEX_Private::getXdbfResInfo(void)
{
	if (resInfo.resource_vaddr != 0) {
		// Already loaded.
		return &resInfo;
	}

	// Get the resource information.
	const XEX2_Optional_Header_Tbl *const entry = getOptHdrTblEntry(XEX2_OPTHDR_RESOURCE_INFO);
	if (!entry) {
		// No resource information.
		return nullptr;
	}

	size_t size = file->seekAndRead(be32_to_cpu(entry->offset), &resInfo, sizeof(resInfo));
	if (size != sizeof(resInfo)) {
		// Seek and/or read error.
		resInfo.resource_vaddr = 0;
		return nullptr;
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the resInfo struct.
	resInfo.size		= be32_to_cpu(resInfo.size);
	resInfo.resource_vaddr	= be32_to_cpu(resInfo.resource_vaddr);
	resInfo.resource_size	= be32_to_cpu(resInfo.resource_size);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// Sanity check: resource_size should be less than 2 MB.
	assert(resInfo.resource_size <= 2*1024*1024);
	if (resInfo.resource_size > 2*1024*1024) {
		// That's too much!
		resInfo.resource_vaddr = 0;
		return nullptr;
	}

	return &resInfo;
}

/**
 * Initialize the PE executable reader.
 * @return peReader on success; nullptr on error.
 */
CBCReader *Xbox360_XEX_Private::initPeReader(void)
{
	if (peReader) {
		// PE Reader is already initialized.
		return peReader;
	}
#ifdef ENABLE_LIBMSPACK
	if (!lzx_peHeader.empty()) {
		// LZX has been decompressed.
		return peReader;
	}
#endif /* ENABLE_LIBMSPACK */

	// Get the file format info.
	const XEX2_Optional_Header_Tbl *const entry = getOptHdrTblEntry(XEX2_OPTHDR_FILE_FORMAT_INFO);
	if (!entry) {
		// No file format info.
		return nullptr;
	}

	size_t size = file->seekAndRead(be32_to_cpu(entry->offset), &fileFormatInfo, sizeof(fileFormatInfo));
	if (size != sizeof(fileFormatInfo)) {
		// Seek and/or read error.
		return nullptr;
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the fileFormatInfo struct.
	fileFormatInfo.size             = be32_to_cpu(fileFormatInfo.size);
	fileFormatInfo.encryption_type  = be16_to_cpu(fileFormatInfo.encryption_type);
	fileFormatInfo.compression_type = be16_to_cpu(fileFormatInfo.compression_type);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// NOTE: Using two CBCReader instances.
	// - [0]: Retail key and/or no encryption.
	// - [1]: Debug key
	//
	// If encrypted but the retail key isn't available,
	// [0] will be nullptr, and [1] will be valid.
	//
	// We need to decrypt before we decompress, so we can't check
	// if the decryption key works until decompression is done.
	CBCReader *reader[2] = {nullptr, nullptr};

	// Create the CBCReader for decryption.
	const size_t pe_length = (size_t)file->size() - xex2Header.pe_offset;
	if (fileFormatInfo.encryption_type == cpu_to_be16(XEX2_ENCRYPTION_TYPE_NONE)) {
		// No encryption.
		reader[0] = new CBCReader(file, xex2Header.pe_offset, pe_length, nullptr, nullptr);
	}
#ifdef ENABLE_DECRYPTION
	else {
		// Decrypt the title key.
		// Get the Key Manager instance.
		KeyManager *const keyManager = KeyManager::instance();
		assert(keyManager != nullptr);

		// Get the common keys.

		// Zero data.
		uint8_t zero16[16];
		memset(zero16, 0, sizeof(zero16));

		// Key data.
		// - 0: retail
		// - 1: debug (pseudo-keydata)
		KeyManager::KeyData_t keyData[2];
		unsigned int idx0 = 0;

		// Debug key
		keyData[1].key = zero16;
		keyData[1].length = 16;

		// Try to load the retail key.
		KeyManager::VerifyResult verifyResult = keyManager->getAndVerify(
			EncryptionKeyNames[0], &keyData[0],
			EncryptionKeyVerifyData[0], 16);
		if (verifyResult != KeyManager::VERIFY_OK) {
			// An error occurred while loading the retail key.
			// Start with the debug key.
			idx0 = 1;
		}

		// IAesCipher instance.
		unique_ptr<IAesCipher> cipher(AesCipherFactory::create());

		for (unsigned int i = idx0; i < ARRAY_SIZE(keyData); i++) {
			// Load the common key. (CBC mode)
			int ret = cipher->setKey(keyData[i].key, keyData[i].length);
			ret |= cipher->setChainingMode(IAesCipher::CM_CBC);
			if (ret != 0) {
				// Error initializing the cipher.
				continue;
			}

			// Decrypt the title key.
			uint8_t title_key[16];
			memcpy(title_key, xex2Security.title_key, sizeof(title_key));
			if (cipher->decrypt(title_key, sizeof(title_key), zero16, sizeof(zero16)) != sizeof(title_key)) {
				// Error decrypting the title key.
				continue;
			}

			// Initialize the CBCReader.
			reader[i] = new CBCReader(file, xex2Header.pe_offset, pe_length, title_key, zero16);
			if (!reader[i]->isOpen()) {
				// Unable to open the CBCReader.
				delete reader[i];
				reader[i] = nullptr;
				continue;
			}

			// PE header will be verified later.
		}
	}
#endif /* ENABLE_DECRYPTION */

	if ((!reader[0] || !reader[0]->isOpen()) &&
	    (!reader[1] || !reader[1]->isOpen()))
	{
		// Unable to open any CBCReader.
		delete reader[0];
		delete reader[1];
		return nullptr;
	}

	// Check the compression type.
	switch (fileFormatInfo.compression_type) {
		case XEX2_COMPRESSION_TYPE_NONE:
			// No compression.
			break;

		case XEX2_COMPRESSION_TYPE_BASIC: {
			// Basic compression.
			// Load the compression information, then convert it
			// to physical addresses.
			// TODO: IDiscReader subclass to handle this?
			assert(fileFormatInfo.size > sizeof(fileFormatInfo));
			if (fileFormatInfo.size <= sizeof(fileFormatInfo)) {
				// No segment information is available.
				delete reader[0];
				delete reader[1];
				return nullptr;
			}

			const uint32_t seg_len = fileFormatInfo.size - sizeof(fileFormatInfo);
			assert(seg_len % sizeof(XEX2_Compression_Basic_Info) == 0);
			const unsigned int seg_count = seg_len / sizeof(XEX2_Compression_Basic_Info);
			unique_ptr<XEX2_Compression_Basic_Info[]> cbi(new XEX2_Compression_Basic_Info[seg_count]);
			size = file->read(cbi.get(), seg_len);
			if (size != seg_len) {
				// Seek and/or read error.
				delete reader[0];
				delete reader[1];
				return nullptr;
			}

			uint32_t vaddr = 0, physaddr = 0;
			basicZDataSegments.resize(seg_len);
			const XEX2_Compression_Basic_Info *p = cbi.get();
			for (unsigned int i = 0; i < seg_count; i++, p++) {
				const uint32_t data_size = be32_to_cpu(p->data_size);
				basicZDataSegments[i].vaddr = vaddr;
				basicZDataSegments[i].physaddr = physaddr;
				basicZDataSegments[i].length = data_size;
				vaddr += data_size;
				vaddr += be32_to_cpu(p->zero_size);
				physaddr += data_size;
			}
			break;
		}

#ifdef ENABLE_LIBMSPACK
		case XEX2_COMPRESSION_TYPE_NORMAL: {
			// Normal (LZX) compression.
			// Load the block segment data.
			assert(fileFormatInfo.size >= sizeof(fileFormatInfo) + sizeof(XEX2_Compression_Normal_Header));
			if (fileFormatInfo.size < sizeof(fileFormatInfo) + sizeof(XEX2_Compression_Normal_Header)) {
				// No segment information is available.
				delete reader[0];
				delete reader[1];
				return nullptr;
			}

			// Image size must be at least 8 KB.
			assert(be32_to_cpu(xex2Security.image_size >= PE_HEADER_SIZE));
			if (xex2Security.image_size < PE_HEADER_SIZE) {
				// Too small.
				delete reader[0];
				delete reader[1];
				return nullptr;
			}

			// Window size.
			// NOTE: Technically part of XEX2_Compression_Normal_Header,
			// but we're not using that in order to be
			// able to swap lzx_blocks.
			uint32_t window_size;
			size = file->read(&window_size, sizeof(window_size));
			if (size != sizeof(window_size)) {
				// Seek and/or read error.
				delete reader[0];
				delete reader[1];
				return nullptr;
			}
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
			window_size = be32_to_cpu(window_size);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

			// First block.
			XEX2_Compression_Normal_Info first_block, lzx_blocks[2];
			unsigned int lzx_idx = 0;
			size = file->read(&first_block, sizeof(first_block));
			if (size != sizeof(first_block)) {
				// Seek and/or read error.
				delete reader[0];
				delete reader[1];
				return nullptr;
			}
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
			first_block.block_size = be32_to_cpu(first_block.block_size);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */
			memcpy(&lzx_blocks[0], &first_block, sizeof(first_block));
			// First block header is stored in the XEX header.
			// Second block header is stored at the beginning of the compressed data.

			// NOTE: We can't easily randomly seek within the compressed data,
			// since the uncompressed block size isn't stored anywhere.
			// We'll have to load the entire executable into memory,
			// save the relevant portions, then free it.
			// FIXME: It *might* be possible to randomly seek...
			// Need to analyze the format more.
			const int64_t fileSize = file->size();
			const uint32_t image_size = be32_to_cpu(xex2Security.image_size);
			if (fileSize > 64*1024*1024 || image_size > 64*1024*1024) {
				// 64 MB is our compressed and uncompressed limit.
				delete reader[0];
				delete reader[1];
				return nullptr;
			}

			// Compressed EXE buffer.
			// We have to de-block the compressed data first.
			const size_t compressed_size = static_cast<size_t>(fileSize) - xex2Header.pe_offset;
			unique_ptr<uint8_t[]> compressed_deblock(new uint8_t[compressed_size]);

			// Pointer within the deblocked compressed data.
			uint8_t *p_dblk = compressed_deblock.get();
			uint8_t *const p_dblk_end = p_dblk + compressed_size;

			// CBCReader index.
			// If a block size is invalid, we'll switch to the other one.
			// If both are invalid, we have a problem.
			unsigned int rd_idx = (reader[0] ? 0 : 1);
			if (!reader[rd_idx]) {
				// No readers available...
				return nullptr;
			}

			// Start at the beginning.
			reader[rd_idx]->rewind();

			// Based on: https://github.com/xenia-project/xenia/blob/5f764fc752c82674981a9f402f1bbd96b399112a/src/xenia/cpu/xex_module.cc
			while (lzx_blocks[lzx_idx].block_size != 0) {
				// Read the next block header.
				size = reader[rd_idx]->read(&lzx_blocks[!lzx_idx], sizeof(lzx_blocks[!lzx_idx]));
				if (size != sizeof(lzx_blocks[!lzx_idx])) {
					// Seek and/or read error.
					delete reader[0];
					delete reader[1];
					return nullptr;
				}

				// Does the block size make sense?
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
				lzx_blocks[!lzx_idx].block_size = be32_to_cpu(lzx_blocks[!lzx_idx].block_size);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */
				if (lzx_blocks[!lzx_idx].block_size > 65536) {
					// Block size is invalid.
					// Switch to the other reader.
					if (rd_idx == 1) {
						// No more readers...
						delete reader[1];
						return nullptr;
					}
					delete reader[0];
					reader[0] = nullptr;

					// reader[1] might be nullptr here...
					if (!reader[1]) {
						// Cannot continue.
						return nullptr;
					}
					rd_idx = 1;

					// Restart decompression in case the first few blocks
					// were decompressed without any "errors".
					// TODO: Also do this for errors after reading the block size?
					reader[1]->rewind();
					lzx_idx = 0;
					memcpy(&lzx_blocks[0], &first_block, sizeof(first_block));
					p_dblk = compressed_deblock.get();
					continue;
				}

				// Read the current block.
				uint32_t block_size = lzx_blocks[lzx_idx].block_size;
				assert(block_size > sizeof(lzx_blocks[!lzx_idx]));
				if (block_size <= sizeof(lzx_blocks[!lzx_idx])) {
					// Block is missing the "next block" header...
					delete reader[0];
					delete reader[1];
					return nullptr;
				}
				block_size -= sizeof(lzx_blocks[!lzx_idx]);

				while (block_size > 2) {
					// Get the chunk size.
					uint16_t chunk_size;
					size = reader[rd_idx]->read(&chunk_size, sizeof(chunk_size));
					if (size != sizeof(chunk_size)) {
						// Seek and/or read error.
						delete reader[0];
						delete reader[1];
						return nullptr;
					}
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
					chunk_size = be16_to_cpu(chunk_size);
#endif /* SYS_BYTEORDER = SYS_LIL_ENDIAN */
					block_size -= 2;
					if (chunk_size == 0 || chunk_size > block_size) {
						// End of block, or not enough data is available.
						break;
					}

					// Do we have enough space?
					if (p_dblk + chunk_size >= p_dblk_end) {
						// Out of memory.
						// TODO: Error reporting.
						delete reader[0];
						delete reader[1];
						return nullptr;
					}

					size = reader[rd_idx]->read(p_dblk, chunk_size);
					if (size != chunk_size) {
						// Seek and/or read error.
						delete reader[0];
						delete reader[1];
						return nullptr;
					}

					p_dblk += chunk_size;
					block_size -= chunk_size;
				}
				if (block_size > 0) {
					// Empty data at the end of the block.
					// TODO: Error handling.
					// TODO: SEEK_CUR?
					reader[rd_idx]->seek(reader[rd_idx]->tell() + block_size);
				}

				// Next block.
				lzx_idx = !lzx_idx;
			}

			// Decompress the data.
			unique_ptr<uint8_t[]> decompressed_exe(new uint8_t[image_size]);
			int res = lzx_decompress(compressed_deblock.get(),
				static_cast<size_t>(p_dblk - compressed_deblock.get()),
				decompressed_exe.get(), image_size,
				window_size, nullptr, 0);
			if (res != MSPACK_ERR_OK) {
				// Error decompressing the data.
				delete reader[0];
				delete reader[1];
				return nullptr;
			}

			// Verify the MZ header.
			uint16_t mz;
			memcpy(&mz, decompressed_exe.get(), sizeof(mz));
			if (mz != cpu_to_be16('MZ')) {
				// MZ header is not valid.
				// TODO: Other checks?
				delete reader[0];
				delete reader[1];
				return nullptr;
			}

			// Copy the PE header.
			lzx_peHeader.resize(PE_HEADER_SIZE);
			memcpy(lzx_peHeader.data(), decompressed_exe.get(), PE_HEADER_SIZE);

			// Copy the XDBF section.
			const XEX2_Resource_Info *const pResInfo = getXdbfResInfo();
			if (pResInfo) {
				const uint32_t xdbf_physaddr = pResInfo->resource_vaddr -
							       be32_to_cpu(xex2Security.load_address);
				if (xdbf_physaddr + pResInfo->resource_size <= image_size) {
					lzx_xdbfSection.resize(pResInfo->resource_size);
					memcpy(lzx_xdbfSection.data(),
						decompressed_exe.get() + xdbf_physaddr,
						pResInfo->resource_size);
				}
			}

			// Save the correct reader.
			this->peReader = reader[rd_idx];
			reader[rd_idx] = nullptr;
			keyInUse = rd_idx;
			break;
		}
#endif /* ENABLE_LIBMSPACK */
	}

	// Verify the MZ header for non-LZX compression.
#ifdef ENABLE_LIBMSPACK
	if (lzx_peHeader.empty())
#endif /* ENABLE_LIBMSPACK */
	{
		// Check the CBCReader objects.
		for (unsigned int i = 0; i < ARRAY_SIZE(reader); i++) {
			if (!reader[i])
				break;
			uint16_t mz;
			size_t size = reader[i]->read(&mz, sizeof(mz));
			if (size == sizeof(mz)) {
				if (mz == cpu_to_be16('MZ')) {
					// MZ header is valid.
					// TODO: Other checks?
					this->peReader = reader[i];
					reader[i] = nullptr;
					keyInUse = i;
					break;
				}
			}
		}
	}

	// Delete the incorrect CBCReaders.
	delete reader[0];
	delete reader[1];

	// CBCReader is open and file decompression has been initialized.
	return this->peReader;
}

/**
 * Convert game ratings from Xbox 360 format to RomFields format.
 * @param age_ratings RomFields::age_ratings_t
 * @param game_ratings XEX2_Game_Ratings
 */
void Xbox360_XEX_Private::convertGameRatings(
	RomFields::age_ratings_t &age_ratings,
	const XEX2_Game_Ratings &game_ratings)
{
	// RomFields::age_ratings_t uses a format that matches
	// Nintendo's systems.

	// Clear the ratings first.
	age_ratings.fill(0);

	// Region conversion table:
	// - Index: Xbox 360 region (-1 if not supported)
	// - Value: RomFields::age_ratings_t region
	static const int8_t region_conv[14] = {
		RomFields::AGE_USA,
		RomFields::AGE_EUROPE,
		RomFields::AGE_FINLAND,
		RomFields::AGE_PORTUGAL,
		RomFields::AGE_ENGLAND,
		RomFields::AGE_JAPAN,
		RomFields::AGE_GERMANY,
		RomFields::AGE_AUSTRALIA,
		-1,	// TODO: NZ (usually the same as AU)
		RomFields::AGE_SOUTH_KOREA,
		-1,	// TODO: Brazil
		-1,	// TODO: FPB?
		RomFields::AGE_TAIWAN,
		-1,	// TODO: Singapore
	};

	// Rating conversion table:
	// - Primary index: Xbox 360 region
	// - Secondary index: Xbox 360 age value, from 0-15
	// - Value: RomFields::age_ratings_t age value.
	// If the Xbox 360 age value is over 15, the rating is invalid.
	// If the age_ratings_t value is 0xFF, the rating is invalid.
	//
	// Values are set using the following formula:
	// - If rating A is 0, and rating B is 2:
	//   - The value for "A" gets slot 0.
	//   - The value for "B" gets slots 1 and 2.
	static const uint8_t region_values[14][16] = {
		// AGE_USA (ESRB)
		{3, 6, 6, 10, 10, 13, 13, 17, 17, 18, 18, 18, 18, 18, 18, 0xFF},
		// AGE_EUROPE (PEGI)
		{3, 4, 4, 4, 4, 12, 12, 12, 12, 12, 16, 16, 16, 16, 18, 0xFF},
		// AGE_FINLAND (PEGI-FI/MEKU)
		{3, 7, 7, 7, 7, 11, 11, 11, 11, 15, 15, 15, 15, 18, 18, 0xFF},
		// AGE_PORTUGAL (PEGI-PT)
		{4, 4, 6, 6, 12, 12, 12, 12, 12, 12, 16, 16, 16, 16, 18, 0xFF},
		// AGE_ENGLAND (BBFC)
		// TODO: How are Universal and PG handled for Nintendo?
		{3, 3, 7, 7, 7, 7, 12, 12, 12, 12, 15, 15, 15, 16, 18, 0xFF},
		// AGE_JAPAN (CERO)
		{0, 12, 12, 15, 15, 17, 17, 18, 18,            0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
		// AGE_GERMANY (USK)
		{0, 6, 6, 12, 12, 16, 16, 18, 18,              0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
		// AGE_AUSTRALIA (OFLC_AU)
		// TODO: Is R18+ available on Xbox 360?
		{0, 7, 7, 14, 14, 15, 15, 0xFF,           0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
		// TODO: NZ
		{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
		// AGE_SOUTH_KOREA (KMRB/GRB)
		{0, 12, 12, 15, 15, 18, 18, 0xFF,         0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
		// TODO: Brazil
		{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
		// TODO: FPB?
		{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
		// TODO: Taiwan
		{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
		// TODO: Singapore
		{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
	};

	// 14 ratings for Xbox 360 games.
	for (unsigned int ridx = 0; ridx < 14; ridx++) {
		const int8_t ar_idx = region_conv[ridx];
		if (ar_idx < 0) {
			// Not supported.
			continue;
		}

		const uint8_t xb_val = game_ratings.ratings[ridx];
		if (xb_val > 16) {
			// Invalid rating.
			continue;
		}

		uint8_t rf_val = region_values[ridx][xb_val];
		if (rf_val == 0xFF) {
			// Invalid rating.
			continue;
		}

		age_ratings[ar_idx] = rf_val | RomFields::AGEBF_ACTIVE;
	}
}

/**
 * Initialize the EXE object.
 * @return EXE object on success; nullptr on error.
 */
const EXE *Xbox360_XEX_Private::initEXE(void)
{
	if (pe_exe) {
		// EXE is already initialized.
		return pe_exe;
	}

	// Initialize the PE reader.
	if (!initPeReader()) {
		// Error initializing the PE reader.
		return nullptr;
	}

	// The EXE header is located at the beginning of the
	// PE section, so we don't have to look anything up.
	// TODO: LZX support.

	// Attempt to open the EXE section.
	// Assuming a maximum of 8 KB for the PE headers.
	IRpFile *peFile_tmp;
#ifdef ENABLE_LIBMSPACK
	if (!lzx_peHeader.empty()) {
		peFile_tmp = new RpMemFile(lzx_peHeader.data(), lzx_peHeader.size());
	} else
#endif /* ENABLE_LIBMSPACK */
	{
		peFile_tmp = new PartitionFile(peReader, 0, PE_HEADER_SIZE);
	}
	if (peFile_tmp->isOpen()) {
		EXE *const pe_exe_tmp = new EXE(peFile_tmp);
		if (pe_exe_tmp->isOpen()) {
			peFile_exe = peFile_tmp;
			pe_exe = pe_exe_tmp;
		} else {
			pe_exe_tmp->unref();
			delete peFile_tmp;
		}
	} else {
		delete peFile_tmp;
	}

	return pe_exe;
}

/**
 * Initialize the Xbox360_XDBF object.
 * @return Xbox360_XDBF object on success; nullptr on error.
 */
const Xbox360_XDBF *Xbox360_XEX_Private::initXDBF(void)
{
	if (pe_xdbf) {
		// XDBF is already initialized.
		return pe_xdbf;
	}

	// Initialize the PE reader.
	if (!initPeReader()) {
		// Error initializing the PE reader.
		return nullptr;
	}

	// Attempt to open the XDBF section.
	IRpFile *peFile_tmp;
#ifdef ENABLE_LIBMSPACK
	if (!lzx_xdbfSection.empty()) {
		peFile_tmp = new RpMemFile(lzx_xdbfSection.data(), lzx_xdbfSection.size());
	} else
#endif /* ENABLE_LIBMSPACK */
	{
		// Get the XDBF resource information.
		const XEX2_Resource_Info *const pResInfo = getXdbfResInfo();
		if (!pResInfo) {
			// No XDBF section.
			return nullptr;
		}

		// Calculate the XDBF physical address.
		uint32_t xdbf_physaddr = pResInfo->resource_vaddr -
					 be32_to_cpu(xex2Security.load_address);

		if (fileFormatInfo.compression_type == XEX2_COMPRESSION_TYPE_BASIC) {
			// File has zero padding removed.
			// Determine the actual physical address.
			for (auto iter = basicZDataSegments.cbegin();
			     iter != basicZDataSegments.cend(); ++iter)
			{
				if (xdbf_physaddr >= iter->vaddr &&
				    xdbf_physaddr < (iter->vaddr + iter->length))
				{
					// Found the correct segment.
					// Adjust the physical address.
					xdbf_physaddr -= (iter->vaddr - iter->physaddr);
					break;
				}
			}
		}
		peFile_tmp = new PartitionFile(peReader, xdbf_physaddr, pResInfo->resource_size);
	}
	if (peFile_tmp->isOpen()) {
		Xbox360_XDBF *const pe_xdbf_tmp = new Xbox360_XDBF(peFile_tmp, true);
		if (pe_xdbf_tmp->isOpen()) {
			peFile_xdbf = peFile_tmp;
			pe_xdbf = pe_xdbf_tmp;
		} else {
			pe_xdbf_tmp->unref();
			delete peFile_tmp;
		}
	} else {
		delete peFile_tmp;
	}

	return pe_xdbf;
}

/** Xbox360_XEX **/

/**
 * Read an Xbox 360 XEX file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open XEX file.
 */
Xbox360_XEX::Xbox360_XEX(IRpFile *file)
	: super(new Xbox360_XEX_Private(this, file))
{
	// This class handles executables.
	RP_D(Xbox360_XEX);
	d->className = "Xbox360_XEX";
	d->fileType = FTYPE_EXECUTABLE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the XEX2 header.
	d->file->rewind();
	size_t size = d->file->read(&d->xex2Header, sizeof(d->xex2Header));
	if (size != sizeof(d->xex2Header)) {
		d->xex2Header.magic = 0;
		delete d->file;
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->xex2Header);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->xex2Header);
	info.ext = nullptr;	// Not needed for XEX.
	info.szFile = 0;	// Not needed for XEX.
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->xex2Header.magic = 0;
		delete d->file;
		d->file = nullptr;
		return;
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the header for little-endian systems.
	// NOTE: The magic number is *not* byteswapped here.
	d->xex2Header.module_flags	= be32_to_cpu(d->xex2Header.module_flags);
	d->xex2Header.pe_offset		= be32_to_cpu(d->xex2Header.pe_offset);
	d->xex2Header.reserved		= be32_to_cpu(d->xex2Header.reserved);
	d->xex2Header.sec_info_offset	= be32_to_cpu(d->xex2Header.sec_info_offset);
	d->xex2Header.opt_header_count	= be32_to_cpu(d->xex2Header.opt_header_count);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// Read the security info.
	size = d->file->seekAndRead(d->xex2Header.sec_info_offset, &d->xex2Security, sizeof(d->xex2Security));
	if (size != sizeof(d->xex2Security)) {
		// Seek and/or read error.
		d->xex2Header.magic = 0;
		delete d->file;
		d->file = nullptr;
		return;
	}

	// Read the optional header table.
	// Maximum of 32 optional headers.
	assert(d->xex2Header.opt_header_count <= 32);
	const unsigned int opt_header_count = std::min(d->xex2Header.opt_header_count, 32U);
	d->optHdrTbl.resize(opt_header_count);
	const size_t opt_header_sz = (size_t)opt_header_count * sizeof(XEX2_Optional_Header_Tbl);
	size = d->file->seekAndRead(sizeof(d->xex2Header), d->optHdrTbl.data(), opt_header_sz);
	if (size != opt_header_sz) {
		// Seek and/or read error.
		d->optHdrTbl.clear();
		d->optHdrTbl.shrink_to_fit();
		d->xex2Header.magic = 0;
		delete d->file;
		d->file = nullptr;
		return;
	}
}

/**
 * Close the opened file.
 */
void Xbox360_XEX::close(void)
{
	RP_D(Xbox360_XEX);
	if (d->pe_xdbf) {
		d->pe_xdbf->unref();
		d->pe_xdbf = nullptr;
	}
	if (d->pe_exe) {
		d->pe_exe->unref();
		d->pe_exe = nullptr;
	}

	delete d->peFile_xdbf;
	delete d->peFile_exe;
	delete d->peReader;
	d->peFile_xdbf = nullptr;
	d->peFile_exe = nullptr;
	d->peReader = nullptr;

#ifdef ENABLE_LIBMSPACK
	d->lzx_peHeader.clear();
	d->lzx_peHeader.shrink_to_fit();
	d->lzx_xdbfSection.clear();
	d->lzx_xdbfSection.shrink_to_fit();
#endif /* ENABLE_LIBMSPACK */

	// Call the superclass function.
	super::close();
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Xbox360_XEX::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(XEX2_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for XEX.
	const XEX2_Header *const xex2Header =
		reinterpret_cast<const XEX2_Header*>(info->header.pData);
	if (xex2Header->magic == cpu_to_be32(XEX2_MAGIC)) {
		// We have an XEX2 file.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *Xbox360_XEX::systemName(unsigned int type) const
{
	RP_D(const Xbox360_XEX);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (long, short, abbreviation)
	// TODO: XEX-specific, or just use Xbox 360?
	static const char *const sysNames[4] = {
		"Microsoft Xbox 360", "Xbox 360", "X360", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *Xbox360_XEX::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".xex",

		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *Xbox360_XEX::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"application/x-xbox360-xex",

		nullptr
	};
	return mimeTypes;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Xbox360_XEX::supportedImageTypes(void) const
{
	RP_D(const Xbox360_XEX);
	if (const_cast<Xbox360_XEX_Private*>(d)->initXDBF()) {
		return d->pe_xdbf->supportedImageTypes();
	}

	return 0;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> Xbox360_XEX::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	RP_D(const Xbox360_XEX);
	if (const_cast<Xbox360_XEX_Private*>(d)->initXDBF()) {
		return d->pe_xdbf->supportedImageSizes(imageType);
	}
	
	return vector<ImageSizeDef>();
}

/**
 * Get image processing flags.
 *
 * These specify post-processing operations for images,
 * e.g. applying transparency masks.
 *
 * @param imageType Image type.
 * @return Bitfield of ImageProcessingBF operations to perform.
 */
uint32_t Xbox360_XEX::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	RP_D(const Xbox360_XEX);
	if (const_cast<Xbox360_XEX_Private*>(d)->initXDBF()) {
		return d->pe_xdbf->imgpf(imageType);
	}

	return 0;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Xbox360_XEX::loadFieldData(void)
{
	RP_D(Xbox360_XEX);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// XEX file isn't valid.
		return -EIO;
	}

	// Parse the XEX file.
	// NOTE: The magic number is NOT byteswapped in the constructor.
	const XEX2_Header *const xex2Header = &d->xex2Header;
	const XEX2_Security_Info *const xex2Security = &d->xex2Security;
	if (xex2Header->magic != cpu_to_be32(XEX2_MAGIC)) {
		// Invalid magic number.
		return 0;
	}

	// Maximum of 11 fields.
	d->fields->reserve(11);
	d->fields->setTabName(0, "XEX");

	// Game name.
	const Xbox360_XDBF *const xdbf = d->initXDBF();
	if (xdbf) {
		string title = xdbf->getGameTitle();
		if (!title.empty()) {
			d->fields->addField_string(C_("RomData", "Title"), title);
		}
	}

	// Original executable name
	const XEX2_Optional_Header_Tbl *entry = d->getOptHdrTblEntry(XEX2_OPTHDR_ORIGINAL_PE_NAME);
	if (entry) {
		// Read the filename length.
		uint32_t length = 0;
		size_t size = d->file->seekAndRead(be32_to_cpu(entry->offset), &length, sizeof(length));
		if (size == sizeof(length)) {
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
			length = be32_to_cpu(length);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */
			// Length includes the length DWORD.
			// Sanity check: Actual filename must be less than 260 bytes. (PATH_MAX)
			assert(length > sizeof(uint32_t));
			assert(length <= 260+sizeof(uint32_t));
			if (length > sizeof(uint32_t) && length <= 260+sizeof(uint32_t)) {
				// Remove the DWORD length from the filename length.
				length -= sizeof(uint32_t);
				unique_ptr<char[]> pe_filename(new char[length+1]);
				size = d->file->read(pe_filename.get(), length);
				if (size == length) {
					d->fields->addField_string(C_("Xbox360_XEX", "PE Filename"),
						pe_filename.get(), RomFields::STRF_TRIM_END);
				}
			}
		}
	}

	// Module flags
	static const char *const module_flags_tbl[] = {
		NOP_C_("Xbox360_XEX", "Title"),
		NOP_C_("Xbox360_XEX", "Exports"),
		NOP_C_("Xbox360_XEX", "Debugger"),
		NOP_C_("Xbox360_XEX", "DLL"),
		NOP_C_("Xbox360_XEX", "Module Patch"),
		NOP_C_("Xbox360_XEX", "Full Patch"),
		NOP_C_("Xbox360_XEX", "Delta Patch"),
		NOP_C_("Xbox360_XEX", "User Mode"),
	};
	vector<string> *const v_module_flags = RomFields::strArrayToVector_i18n(
		"Xbox360_XEX", module_flags_tbl, ARRAY_SIZE(module_flags_tbl));
	d->fields->addField_bitfield(C_("Xbox360_XEX", "Module Flags"),
		v_module_flags, 4, xex2Header->module_flags);

	// TODO: Show image flags as-is?
	const uint32_t image_flags = be32_to_cpu(d->xex2Security.image_flags);

	// Media types
	// NOTE: Using a string instead of a bitfield because very rarely
	// are all of these set, and in most cases, none are.
	// TODO: RFT_LISTDATA?
	if (image_flags & XEX2_IMAGE_FLAG_XGD2_MEDIA_ONLY) {
		// XGD2 media only.
		d->fields->addField_string(C_("Xbox360_XEX", "Media Types"),
			C_("Xbox360_XEX", "XGD2 only"));
	} else {
		// Other types.
		static const char *const media_type_tbl[] = {
			// 0
			NOP_C_("Xbox360_XEX", "Hard Disk"),
			NOP_C_("Xbox360_XEX", "DVD X2"),
			NOP_C_("Xbox360_XEX", "DVD / CD"),
			NOP_C_("Xbox360_XEX", "DVD (Single Layer)"),
			// 4
			NOP_C_("Xbox360_XEX", "DVD (Dual Layer)"),
			NOP_C_("Xbox360_XEX", "Internal Flash Memory"),
			nullptr,
			NOP_C_("Xbox360_XEX", "Memory Unit"),
			// 8
			NOP_C_("Xbox360_XEX", "USB Mass Storage Device"),
			NOP_C_("Xbox360_XEX", "Network"),
			NOP_C_("Xbox360_XEX", "Direct from Memory"),
			NOP_C_("Xbox360_XEX", "Hard RAM Drive"),
			// 12
			NOP_C_("Xbox360_XEX", "SVOD"),
			nullptr, nullptr, nullptr,
			// 16
			nullptr, nullptr, nullptr, nullptr,
			// 20
			nullptr, nullptr, nullptr, nullptr,
			// 24
			NOP_C_("Xbox360_XEX", "Insecure Package"),
			NOP_C_("Xbox360_XEX", "Savegame Package"),
			NOP_C_("Xbox360_XEX", "Locally Signed Package"),
			NOP_C_("Xbox360_XEX", "Xbox Live Signed Package"),
			// 28
			NOP_C_("Xbox360_XEX", "Xbox Package"),
		};

		ostringstream oss;
		unsigned int found = 0;
		uint32_t media_types = be32_to_cpu(xex2Security->allowed_media_types);
		for (unsigned int i = 0; i < ARRAY_SIZE(media_type_tbl); i++, media_types >>= 1) {
			if (!(media_types & 1))
				continue;

			if (found > 0) {
				if (found % 4 == 0) {
					oss << ",\n";
				} else {
					oss << ", ";
				}
			}
			found++;

			if (media_type_tbl[i]) {
				oss << media_type_tbl[i];
			} else {
				oss << i;
			}
		}

		d->fields->addField_string(C_("Xbox360_XEX", "Media Types"),
			found ? oss.str() : C_("Xbox360_XEX", "None"));
	}

	// Region code
	// TODO: Special handling for region-free?
	static const char *const region_code_tbl[] = {
		NOP_C_("Region", "USA"),
		NOP_C_("Region", "Japan"),
		NOP_C_("Region", "China"),
		NOP_C_("Region", "Asia"),
		NOP_C_("Region", "Europe"),
		NOP_C_("Region", "Australia"),
		NOP_C_("Region", "New Zealand"),
	};

	// Convert region code to a bitfield.
	const uint32_t region_code_xbx = be32_to_cpu(xex2Security->region_code);
	uint32_t region_code = 0;
	if (region_code_xbx & XEX2_REGION_CODE_NTSC_U) {
		region_code |= (1 << 0);
	}
	if (region_code_xbx & XEX2_REGION_CODE_NTSC_J_JAPAN) {
		region_code |= (1 << 1);
	}
	if (region_code_xbx & XEX2_REGION_CODE_NTSC_J_CHINA) {
		region_code |= (1 << 2);
	}
	if (region_code_xbx & XEX2_REGION_CODE_NTSC_J_OTHER) {
		region_code |= (1 << 3);
	}
	if (region_code_xbx & XEX2_REGION_CODE_PAL_OTHER) {
		region_code |= (1 << 4);
	}
	if (region_code_xbx & XEX2_REGION_CODE_PAL_AU_NZ) {
		// TODO: Combine these bits?
		region_code |= (1 << 5) | (1 << 6);
	}

	vector<string> *const v_region_code = RomFields::strArrayToVector_i18n(
		"Region", region_code_tbl, ARRAY_SIZE(region_code_tbl));
	d->fields->addField_bitfield(C_("RomData", "Region Code"),
		v_region_code, 4, region_code);

	/** Execution ID **/
	entry = d->getOptHdrTblEntry(XEX2_OPTHDR_EXECUTION_ID);
	if (entry) {
		XEX2_Execution_ID execution_id;
		size_t size = d->file->seekAndRead(be32_to_cpu(entry->offset), &execution_id, sizeof(execution_id));
		if (size == sizeof(execution_id)) {
			// Media ID
			d->fields->addField_string_numeric(C_("Xbox360_XEX", "Media ID"),
				be32_to_cpu(execution_id.media_id),
				RomFields::FB_HEX, 8, RomFields::STRF_MONOSPACE);

			// Title ID
			// FIXME: Verify behavior on big-endian.
			d->fields->addField_string(C_("Xbox360_XEX", "Title ID"),
				rp_sprintf_p(C_("Xbox360_HEX", "0x%1$08X (%2$.2s-%3$u)"),
					be32_to_cpu(execution_id.title_id.u32),
					execution_id.title_id.c,
					be16_to_cpu(execution_id.title_id.u16)),
				RomFields::STRF_MONOSPACE);

			// Savegame ID
			d->fields->addField_string_numeric(C_("Xbox360_XEX", "Savegame ID"),
				be32_to_cpu(execution_id.savegame_id),
				RomFields::FB_HEX, 8, RomFields::STRF_MONOSPACE);

			// Disc number
			// NOTE: Not shown for single-disc games.
			const char *const disc_number_title = C_("RomData", "Disc #");
			if (execution_id.disc_number != 0 && execution_id.disc_count > 1) {
				if (execution_id.disc_number > 0) {
					d->fields->addField_string(disc_number_title,
						// tr: Disc X of Y (for multi-disc games)
						rp_sprintf_p(C_("RomData|Disc", "%1$u of %2$u"),
							execution_id.disc_number,
							execution_id.disc_count));
				} else {
					d->fields->addField_string(disc_number_title,
						C_("RomData", "Unknown"));
				}
			}
		}
	}

	/** File format info **/
	// Loaded by initPeReader(), which is called by initXDBF().

	// Encryption key
	const char *s_encryption_key;
	if (d->fileFormatInfo.encryption_type == cpu_to_be16(XEX2_ENCRYPTION_TYPE_NONE)) {
		// No encryption.
		s_encryption_key = C_("Xbox360_XEX|EncKey", "None");
	} else {
		switch (d->keyInUse) {
			default:
			case -1:
				s_encryption_key = C_("RomData", "Unknown");
				break;
			case 0:
				s_encryption_key = C_("Xbox360_XEX|EncKey", "Retail");
				break;
			case 1:
				s_encryption_key = C_("Xbox360_XEX|EncKey", "Debug");
				break;
		}
	}
	d->fields->addField_string(C_("Xbox360_XEX", "Encryption Key"), s_encryption_key);

	// Compression
	static const char *const compression_tbl[] = {
		NOP_C_("Xbox360_XEX|Compression", "None"),
		NOP_C_("Xbox360_XEX|Compression", "Basic (Sparse)"),
		NOP_C_("Xbox360_XEX|Compression", "Normal (LZX)"),
		NOP_C_("Xbox360_XEX|Compression", "Delta"),
	};
	if (d->fileFormatInfo.compression_type < ARRAY_SIZE(compression_tbl)) {
		d->fields->addField_string(C_("Xbox360_XEX", "Compression"),
			dpgettext_expr(RP_I18N_DOMAIN, "Xbox360_XEX|Compression",
				compression_tbl[d->fileFormatInfo.compression_type]));
	} else {
		d->fields->addField_string(C_("Xbox360_XEX", "Compression"),
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"),
				d->fileFormatInfo.compression_type));
	}

	/** Age ratings **/
	// NOTE: RomFields' RFT_AGE_RATINGS type uses a format that matches
	// Nintendo's systems. For Xbox 360, we'll need to convert the format.
	entry = d->getOptHdrTblEntry(XEX2_OPTHDR_GAME_RATINGS);
	if (entry) {
		XEX2_Game_Ratings game_ratings;
		size_t size = d->file->seekAndRead(be32_to_cpu(entry->offset), &game_ratings, sizeof(game_ratings));
		if (size == sizeof(game_ratings)) {
			// Convert the game ratings.
			RomFields::age_ratings_t age_ratings;
			d->convertGameRatings(age_ratings, game_ratings);
			d->fields->addField_ageRatings(C_("RomData", "Age Ratings"), age_ratings);
		}
	}

	// Can we get the EXE section?
	const EXE *const pe_exe = const_cast<Xbox360_XEX_Private*>(d)->initEXE();
	if (pe_exe && pe_exe->isOpen()) {
		// Add the fields.
		d->fields->addFields_romFields(pe_exe->fields(), -2);
	}

	// Can we get the XDBF section?
	const Xbox360_XDBF *const pe_xdbf = const_cast<Xbox360_XEX_Private*>(d)->initXDBF();
	if (pe_xdbf && pe_xdbf->isOpen()) {
		// Add the fields.
		d->fields->addFields_romFields(pe_xdbf->fields(), RomFields::TabOffset_AddTabs);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int Xbox360_XEX::loadMetaData(void)
{
	RP_D(Xbox360_XEX);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// SMDH file isn't valid.
		return -EIO;
	}

	// Make sure the XDBF section is loaded.
	const Xbox360_XDBF *const xdbf = d->initXDBF();
	if (!xdbf) {
		// Unable to load the XDBF section.
		return 0;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// Title.
	string title = xdbf->getGameTitle();
	if (!title.empty()) {
		d->metaData->addMetaData_string(Property::Title, title);
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Pointer to const rp_image* to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int Xbox360_XEX::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(Xbox360_XEX);

	// FIXME: Load the PE section if it isn't already loaded.
	if (d->pe_xdbf) {
		return d->pe_xdbf->loadInternalImage(imageType, pImage);
	}

	return -ENOENT;
}

#ifdef ENABLE_DECRYPTION
/** Encryption keys. **/

/**
 * Get the total number of encryption key names.
 * @return Number of encryption key names.
 */
int Xbox360_XEX::encryptionKeyCount_static(void)
{
	return Key_Max;
}

/**
 * Get an encryption key name.
 * @param keyIdx Encryption key index.
 * @return Encryption key name (in ASCII), or nullptr on error.
 */
const char *Xbox360_XEX::encryptionKeyName_static(int keyIdx)
{
	assert(keyIdx >= 0);
	assert(keyIdx < Key_Max);
	if (keyIdx < 0 || keyIdx >= Key_Max)
		return nullptr;
	return Xbox360_XEX_Private::EncryptionKeyNames[keyIdx];
}

/**
 * Get the verification data for a given encryption key index.
 * @param keyIdx Encryption key index.
 * @return Verification data. (16 bytes)
 */
const uint8_t *Xbox360_XEX::encryptionVerifyData_static(int keyIdx)
{
	assert(keyIdx >= 0);
	assert(keyIdx < Key_Max);
	if (keyIdx < 0 || keyIdx >= Key_Max)
		return nullptr;
	return Xbox360_XEX_Private::EncryptionKeyVerifyData[keyIdx];
}
#endif /* ENABLE_DECRYPTION */

}
