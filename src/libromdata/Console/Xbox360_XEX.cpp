/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox360_XEX.cpp: Microsoft Xbox 360 executable reader.                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"
#include "libromdata/config.libromdata.h"

#include "Xbox360_XEX.hpp"
#include "Xbox360_XDBF.hpp"
#include "Other/EXE.hpp"
#include "xbox360_xex_structs.h"
#include "data/XboxPublishers.hpp"

// Other rom-properties libraries
#include "librpbase/Achievements.hpp"
#include "librpbase/disc/CBCReader.hpp"
#include "librpfile/MemFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

#ifdef ENABLE_DECRYPTION
#  include "librpbase/crypto/IAesCipher.hpp"
#  include "librpbase/crypto/AesCipherFactory.hpp"
#  include "librpbase/crypto/KeyManager.hpp"
#endif /* ENABLE_DECRYPTION */

#ifdef ENABLE_LIBMSPACK
#  include "mspack.h"
#  include "xenia_lzx.h"
#endif /* ENABLE_LIBMSPACK */

// C++ STL classes
using std::array;
using std::ostringstream;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

namespace LibRomData {

// Workaround for RP_D() expecting the no-underscore naming convention.
#define Xbox360_XEXPrivate Xbox360_XEX_Private

class Xbox360_XEX_Private final : public RomDataPrivate
{
	public:
		Xbox360_XEX_Private(const IRpFilePtr &file);
		~Xbox360_XEX_Private() final;

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(Xbox360_XEX_Private)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// XEX type.
		enum class XexType {
			Unknown = -1,

			XEX1	= 0,	// XEX1
			XEX2	= 1,	// XEX2

			Max
		};
		XexType xexType;

	public:
		// XEX headers.
		// NOTE: Only xex2Header is byteswapped, except for the magic number.
		XEX2_Header xex2Header;
		union {
			XEX1_Security_Info xex1;
			XEX2_Security_Info xex2;
		} secInfo;

		// Optional header table.
		// NOTE: This array of structs **IS NOT** byteswapped!
		ao::uvector<XEX2_Optional_Header_Tbl> optHdrTbl;

		// Execution ID. (XEX2_OPTHDR_EXECUTION_ID)
		// Initialized by getXdbfResInfo().
		// NOTE: This struct **IS** byteswapped,
		// except for the title ID.
		bool isExecutionIDLoaded;
		XEX2_Execution_ID executionID;

		// Resource information. (XEX2_OPTHDR_RESOURCE_INFO)
		// Initialized by getXdbfResInfo().
		// NOTE: These structs **ARE** byteswapped.
		// - Key: Resource ID. (Title ID for normal resources; "HASHSEC" for SHA-1 hashes.)
		// - Value: XEX2_Resource_Info
		unordered_map<std::string, XEX2_Resource_Info> mapResInfo;

		// File format info. (XEX2_OPTHDR_FILE_FORMAT_INFO)
		// Initialized by initPeReader().
		// NOTE: This struct **IS** byteswapped.
		XEX2_File_Format_Info fileFormatInfo;

		// Encryption key in use.
		// If fileFormatInfo indicates the PE is encrypted:
		// - -1: Unknown
		// -  0: Retail (may be either XEX1 or XEX2)
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
		// NOTE: Changed from `static const unsigned int` to #define
		// due to shared_ptr causing problems in debug builds.
#define PE_HEADER_SIZE 8192U

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
		 * Get data from an optional header.
		 *
		 * This function is for headers that either have a 32-bit value
		 * stored as the offset (low byte == 0x00), or have a single
		 * DWORD stored in the file (low byte == 0x01).
		 *
		 * @param header_id	[in] Optional header ID.
		 * @param pOut32	[out] Pointer to uint32_t to store the DWORD.
		 * @return Number of bytes read on success; 0 on error.
		 */
		size_t getOptHdrData(uint32_t header_id, uint32_t *pOut32);

		/**
		 * Get data from an optional header.
		 *
		 * This function is for headers that have either a fixed size
		 * (low byte is between 0x02 and 0xFE), or have a size stored
		 * at the beginning of the data (low byte == 0xFF).
		 *
		 * @param header_id	[in] Optional header ID.
		 * @param pVec		[out] ao::uvector<uint8_t>&
		 * @return Number of bytes read on success; 0 on error.
		 */
		size_t getOptHdrData(uint32_t header_id, ao::uvector<uint8_t> &pVec);

		/**
		 * Get the resource information.
		 * @param resource_id Resource ID. (If not specified, use the title ID.)
		 * @return Resource information, or nullptr on error.
		 */
		const XEX2_Resource_Info *getXdbfResInfo(const char *resource_id = nullptr);

		/**
		 * Format a media ID or disc profile ID.
		 *
		 * The last four bytes are separated from the rest of the ID.
		 * This format is used by abgx360.
		 *
		 * @param pId Media ID or disc profile ID. (16 bytes)
		 * @return Formatted ID.
		 */
		static string formatMediaID(const uint8_t *pId);

		/**
		 * Convert game ratings from Xbox 360 format to RomFields format.
		 * @param age_ratings RomFields::age_ratings_t
		 * @param game_ratings XEX2_Game_Ratings
		 */
		static void convertGameRatings(RomFields::age_ratings_t &age_ratings,
			const XEX2_Game_Ratings &game_ratings);

		/**
		 * Get the minimum kernel version required for this XEX.
		 * @return Minimum kernel version, or 0 on error.
		 */
		Xbox360_Version_t getMinKernelVersion(void);

	public:
		// CBC reader for encrypted PE executables.
		// Also used for unencrypted executables.
		CBCReader *peReader;
		EXE *pe_exe;
		Xbox360_XDBF *pe_xdbf;

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

		/**
		 * Get the publisher.
		 * @return Publisher.
		 */
		string getPublisher(void) const;

#ifdef ENABLE_DECRYPTION
	public:
		// Verification key names.
		static const char *const EncryptionKeyNames[Xbox360_XEX::Key_Max];

		// Verification key data.
		static const uint8_t EncryptionKeyVerifyData[Xbox360_XEX::Key_Max][16];
#endif
};

ROMDATA_IMPL(Xbox360_XEX)

/** Xbox360_XEX_Private **/

/* RomDataInfo */
const char *const Xbox360_XEX_Private::exts[] = {
	".xex",		// Executable
	".xexp",	// Patch

	nullptr
};
const char *const Xbox360_XEX_Private::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-xbox360-executable",
	"application/x-xbox360-patch",

	nullptr
};
const RomDataInfo Xbox360_XEX_Private::romDataInfo = {
	"Xbox360_XEX", exts, mimeTypes
};

#ifdef ENABLE_DECRYPTION
// Verification key names.
const char *const Xbox360_XEX_Private::EncryptionKeyNames[Xbox360_XEX::Key_Max] = {
	// XEX1
	"xbox360-xex1",

	// XEX2 (retail only; debug uses an all-zero key)
	"xbox360-xex2",
};

const uint8_t Xbox360_XEXPrivate::EncryptionKeyVerifyData[Xbox360_XEX::Key_Max][16] = {
	// xbox360-xex1
	{0xB9,0x41,0x44,0x80,0xA4,0xE1,0x94,0x82,
	 0xA2,0x9B,0xCD,0x7E,0xC4,0x68,0xB8,0xF0},

	// xbox360-xex2
	{0xAC,0xA0,0xC9,0xE3,0x78,0xD3,0xC6,0x54,
	 0xA3,0x1D,0x65,0x67,0x38,0xAB,0xB0,0x6B},
};
#endif /* ENABLE_DECRYPTION */

Xbox360_XEX_Private::Xbox360_XEX_Private(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, xexType(XexType::Unknown)
	, isExecutionIDLoaded(false)
	, keyInUse(-1)
	, peReader(nullptr)
	, pe_exe(nullptr)
	, pe_xdbf(nullptr)
{
	// Clear the headers.
	memset(&xex2Header, 0, sizeof(xex2Header));
	memset(&secInfo, 0, sizeof(secInfo));
	memset(&executionID, 0, sizeof(executionID));
	memset(&fileFormatInfo, 0, sizeof(fileFormatInfo));
}

Xbox360_XEX_Private::~Xbox360_XEX_Private()
{
	UNREF(pe_xdbf);
	UNREF(pe_exe);
	UNREF(peReader);
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
	// NOTE: The optional headers are *usually* sorted, but we
	// can't depend on that always being the case, since someone
	// might do something screwy.
	auto iter = std::find_if(optHdrTbl.cbegin(), optHdrTbl.cend(),
		[header_id](const XEX2_Optional_Header_Tbl &tbl) noexcept -> bool {
			return (tbl.header_id == header_id);
		}
	);

	return (iter != optHdrTbl.cend() ? &(*iter) : nullptr);
}

/**
 * Get data from an optional header.
 *
 * This function is for headers that either have a 32-bit value
 * stored as the offset (low byte == 0x00), or have a single
 * DWORD stored in the file (low byte == 0x01).
 *
 * @param header_id	[in] Optional header ID.
 * @param pOut32	[out] Pointer to uint32_t to store the DWORD.
 * @return Number of bytes read on success; 0 on error.
 */
size_t Xbox360_XEX_Private::getOptHdrData(uint32_t header_id, uint32_t *pOut32)
{
	assert(pOut32 != nullptr);
	assert((header_id & 0xFF) <= 0x01);
	if ((header_id & 0xFF) > 0x01) {
		// Not supported by this function.
		return 0;
	}

	if ((int)xexType < 0) {
		// Invalid XEX type.
		return 0;
	} else if (!file) {
		// File is closed. Can't read the optional header.
		return 0;
	}

	// Get the entry.
	const XEX2_Optional_Header_Tbl *const entry = getOptHdrTblEntry(header_id);
	if (!entry) {
		// Not found.
		return 0;
	}

	if ((header_id & 0xFF) == 0) {
		// The value is stored as the offset.
		*pOut32 = be32_to_cpu(entry->offset);
		return sizeof(uint32_t);
	}

	// Read the DWORD from the file.
	uint32_t dwData;
	size_t size = file->seekAndRead(be32_to_cpu(entry->offset), &dwData, sizeof(dwData));
	if (size != sizeof(dwData)) {
		// Seek and/or read error.
		return 0;
	}

	*pOut32 = be32_to_cpu(dwData);
	return sizeof(uint32_t);
}

/**
 * Get data from an optional header.
 *
 * This function is for headers that have either a fixed size
 * (low byte is between 0x02 and 0xFE), or have a size stored
 * at the beginning of the data (low byte == 0xFF).
 *
 * @param header_id	[in] Optional header ID.
 * @param pVec		[out] ao::uvector<uint8_t>&
 * @return Number of bytes read on success; 0 on error.
 */
size_t Xbox360_XEX_Private::getOptHdrData(uint32_t header_id, ao::uvector<uint8_t> &pVec)
{
	assert((header_id & 0xFF) > 0x01);
	if ((header_id & 0xFF) <= 0x01) {
		// Not supported by this function.
		return 0;
	}

	if ((int)xexType < 0) {
		// Invalid XEX type.
		return 0;
	}

	// Get the entry.
	const XEX2_Optional_Header_Tbl *const entry = getOptHdrTblEntry(header_id);
	if (!entry) {
		// Not found.
		return 0;
	}

	size_t size;
	const uint32_t offset = be32_to_cpu(entry->offset);
	if ((header_id & 0xFF) != 0xFF) {
		// Size is the low byte, in DWORD units.
		size = (header_id & 0xFF) * sizeof(uint32_t);
	} else {
		// Size is the first DWORD of the data.
		uint32_t dwSize = 0;
		size_t sz_read = file->seekAndRead(offset, &dwSize, sizeof(dwSize));
		if (sz_read != sizeof(dwSize)) {
			// Seek and/or read error.
			return 0;
		}
		size = be32_to_cpu(dwSize);
	}

	// Read the data.
	// NOTE: This includes the size value for 0xFF structs.
	pVec.resize(size);
	size_t sz_read = file->seekAndRead(offset, pVec.data(), size);
	if (sz_read != size) {
		// Seek and/or read error.
		return 0;
	}
	return size;
}

/**
 * Get the resource information.
 * @param resource_id Resource ID. (If not specified, use the title ID.)
 * @return Resource information, or nullptr on error.
 */
const XEX2_Resource_Info *Xbox360_XEX_Private::getXdbfResInfo(const char *resource_id)
{
	if ((int)xexType < 0) {
		// Invalid XEX type.
		return nullptr;
	}

	// General data buffer for loading optional headers.
	ao::uvector<uint8_t> u8_data;

	// Title ID is part of the execution ID, so load it if it
	// hasn't been loaded already.
	// Note that this is loaded even if we don't need the title ID,
	// since other functions may need the execution ID.
	if (!isExecutionIDLoaded) {
		size_t size = getOptHdrData(XEX2_OPTHDR_EXECUTION_ID, u8_data);
		if (size != sizeof(XEX2_Execution_ID)) {
			// Unable to read the execution ID...
			// Can't get the title ID.
			return nullptr;
		}

		const XEX2_Execution_ID *const pLdExecutionId =
			reinterpret_cast<const XEX2_Execution_ID*>(u8_data.data());
		executionID.media_id		= be32_to_cpu(pLdExecutionId->media_id);
		executionID.version.u32		= be32_to_cpu(pLdExecutionId->version.u32);
		executionID.base_version.u32	= be32_to_cpu(pLdExecutionId->base_version.u32);
		// NOTE: Not byteswapping the title ID.
		executionID.title_id		= pLdExecutionId->title_id;
		executionID.savegame_id		= be32_to_cpu(pLdExecutionId->savegame_id);
		isExecutionIDLoaded = true;
	}

	char title_id[9];
	if (!resource_id) {
		// No resource ID specified. Use the title ID.
		snprintf(title_id, sizeof(title_id), "%08X", be32_to_cpu(executionID.title_id.u32));
		resource_id = title_id;
	}

	// Check if the resource information is already loaded.
	auto iter = mapResInfo.find(resource_id);
	if (iter != mapResInfo.end()) {
		// Resource information is already loaded.
		return &(iter->second);
	}

	// Get the resource information.
	size_t size = getOptHdrData(XEX2_OPTHDR_RESOURCE_INFO, u8_data);
	if (size < sizeof(uint32_t) + sizeof(XEX2_Resource_Info)) {
		// No resource information.
		return nullptr;
	}

	// Search the resource table for the specified resource ID.
	const unsigned int res_count = static_cast<unsigned int>(
		(size - sizeof(uint32_t)) / sizeof(XEX2_Resource_Info));
	if (res_count == 0) {
		// No resource information...
		return nullptr;
	}

	XEX2_Resource_Info res;
	res.vaddr = 0;
	res.size = 0;
	const XEX2_Resource_Info *p =
		reinterpret_cast<const XEX2_Resource_Info*>(u8_data.data() + sizeof(uint32_t));
	for (unsigned int i = 0; i < res_count; i++, p++) {
		// Using a string comparison, since the resource ID might not
		// be the full 8 characters.
		if (!strncmp(resource_id, p->resource_id, sizeof(p->resource_id))) {
			// Found a match!
			memcpy(res.resource_id, p->resource_id, sizeof(p->resource_id));
			res.vaddr = be32_to_cpu(p->vaddr);
			res.size  = be32_to_cpu(p->size);
			break;
		}
	}

	if (res.vaddr == 0) {
		// Not found.
		return nullptr;
	}

	// Sanity check: Resource_size should be 2 MB or less.
	assert(res.size <= 2*1024*1024);
	if (res.size > 2*1024*1024) {
		// That's too much!
		return nullptr;
	}

	auto ins_iter = mapResInfo.emplace(resource_id, res);
	return &(ins_iter.first->second);
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
	if (!file) {
		// File is closed. Can't initialize PE Reader.
		return nullptr;
	}

	if (xexType <= XexType::Unknown || xexType >= XexType::Max) {
		// Invalid XEX type.
		return nullptr;
	}

	// Get the file format info.
	ao::uvector<uint8_t> u8_ffi;
	size_t size = getOptHdrData(XEX2_OPTHDR_FILE_FORMAT_INFO, u8_ffi);
	if (size < sizeof(fileFormatInfo)) {
		// Seek and/or read error.
		return nullptr;
	}

	// Copy the file format information.
	const XEX2_File_Format_Info *const pLdFileFormatInfo =
		reinterpret_cast<const XEX2_File_Format_Info*>(u8_ffi.data());
	fileFormatInfo.size             = be32_to_cpu(pLdFileFormatInfo->size);
	fileFormatInfo.encryption_type  = be16_to_cpu(pLdFileFormatInfo->encryption_type);
	fileFormatInfo.compression_type = be16_to_cpu(pLdFileFormatInfo->compression_type);

	// NOTE: Using two CBCReader instances.
	// - [0]: Retail key and/or no encryption.
	// - [1]: Debug key
	//
	// If encrypted but the retail key isn't available,
	// [0] will be nullptr, and [1] will be valid.
	//
	// We need to decrypt before we decompress, so we can't check
	// if the decryption key works until decompression is done.
	array<CBCReader*, 2> reader;
	reader.fill(nullptr);

	// Create the CBCReader for decryption.
	const size_t pe_length = (size_t)file->size() - xex2Header.pe_offset;
	if (fileFormatInfo.encryption_type == XEX2_ENCRYPTION_TYPE_NONE) {
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
		array<KeyManager::KeyData_t, 2> keyData;
		unsigned int idx0 = 0;

		// Debug key
		keyData[1].key = zero16;
		keyData[1].length = 16;

		// Determine which retail key to use.
		const uint32_t image_flags = (xexType != Xbox360_XEX_Private::XexType::XEX1)
			? be32_to_cpu(secInfo.xex2.image_flags)
			: be32_to_cpu(secInfo.xex1.image_flags);
		// TODO: Xeika key?
		int xex_keyIdx;
		if (unlikely(image_flags & XEX2_IMAGE_FLAG_CARDEA_KEY)) {
			xex_keyIdx = 0;	// XEX1
		} else {
			xex_keyIdx = 1;	// XEX2
		}

		// Try to load the XEX key.
		// TODO: Show a warning if it didn't work.
		KeyManager::VerifyResult verifyResult = keyManager->getAndVerify(
			EncryptionKeyNames[xex_keyIdx], &keyData[0],
			EncryptionKeyVerifyData[xex_keyIdx], 16);
		if (verifyResult != KeyManager::VerifyResult::OK) {
			// An error occurred while loading the XEX key.
			// Start with the all-zero key used on devkits.
			idx0 = 1;
		}

		// Title key.
		const uint8_t *const pTitleKey =
			(xexType != XexType::XEX1
				? secInfo.xex2.title_key
				: secInfo.xex1.title_key);

		// IAesCipher instance.
		unique_ptr<IAesCipher> cipher(AesCipherFactory::create());

		for (size_t i = idx0; i < keyData.size(); i++) {
			// Load the common key. (CBC mode)
			int ret = cipher->setKey(keyData[i].key, keyData[i].length);
			ret |= cipher->setChainingMode(IAesCipher::ChainingMode::CBC);
			if (ret != 0) {
				// Error initializing the cipher.
				continue;
			}

			// Decrypt the title key.
			uint8_t dec_title_key[16];
			memcpy(dec_title_key, pTitleKey, sizeof(dec_title_key));
			if (cipher->decrypt(dec_title_key, sizeof(dec_title_key), zero16, sizeof(zero16)) != sizeof(dec_title_key)) {
				// Error decrypting the title key.
				continue;
			}

			// Initialize the CBCReader.
			reader[i] = new CBCReader(file, xex2Header.pe_offset, pe_length, dec_title_key, zero16);
			if (!reader[i]->isOpen()) {
				// Unable to open the CBCReader.
				UNREF_AND_NULL_NOCHK(reader[i]);
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
		UNREF(reader[0]);
		UNREF(reader[1]);
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
				UNREF(reader[0]);
				UNREF(reader[1]);
				return nullptr;
			}

			const uint32_t seg_len = fileFormatInfo.size - sizeof(fileFormatInfo);
			assert(seg_len % sizeof(XEX2_Compression_Basic_Info) == 0);
			const unsigned int seg_count = seg_len / sizeof(XEX2_Compression_Basic_Info);

			uint32_t vaddr = 0, physaddr = 0;
			basicZDataSegments.resize(seg_len);
			const XEX2_Compression_Basic_Info *p =
				reinterpret_cast<const XEX2_Compression_Basic_Info*>(u8_ffi.data() + sizeof(XEX2_File_Format_Info));
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
				UNREF(reader[0]);
				UNREF(reader[1]);
				return nullptr;
			}

			// Image size must be at least 8 KB.
			const uint32_t image_size = be32_to_cpu(
				(xexType != XexType::XEX1
					? secInfo.xex2.image_size
					: secInfo.xex1.image_size));
			assert(image_size >= PE_HEADER_SIZE);
			if (image_size < PE_HEADER_SIZE) {
				// Too small.
				UNREF(reader[0]);
				UNREF(reader[1]);
				return nullptr;
			}

			// Window size.
			// NOTE: Technically part of XEX2_Compression_Normal_Header,
			// but we're not using that in order to be
			// able to swap lzx_blocks.
			const uint8_t *p = u8_ffi.data() + sizeof(fileFormatInfo);
			const uint32_t *const pWindowSize =
				reinterpret_cast<const uint32_t*>(p);
			const uint32_t window_size = be32_to_cpu(*pWindowSize);

			// First block.
			XEX2_Compression_Normal_Info first_block, lzx_blocks[2];
			unsigned int lzx_idx = 0;
			memcpy(&first_block, p+sizeof(window_size), sizeof(first_block));
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
			const off64_t fileSize = file->size();
			if (fileSize > 64*1024*1024 || image_size > 64*1024*1024) {
				// 64 MB is our compressed and uncompressed limit.
				UNREF(reader[0]);
				UNREF(reader[1]);
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
				// FIXME: Not sure if this is needed...
				UNREF(reader[!rd_idx]);
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
					UNREF(reader[0]);
					UNREF(reader[1]);
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
						UNREF(reader[1]);
						return nullptr;
					}
					UNREF_AND_NULL(reader[0]);

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
					UNREF(reader[0]);
					UNREF(reader[1]);
					return nullptr;
				}
				block_size -= sizeof(lzx_blocks[!lzx_idx]);

				while (block_size > 2) {
					// Get the chunk size.
					uint16_t chunk_size;
					size = reader[rd_idx]->read(&chunk_size, sizeof(chunk_size));
					if (size != sizeof(chunk_size)) {
						// Seek and/or read error.
						UNREF(reader[0]);
						UNREF(reader[1]);
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
						UNREF(reader[0]);
						UNREF(reader[1]);
						return nullptr;
					}

					size = reader[rd_idx]->read(p_dblk, chunk_size);
					if (size != chunk_size) {
						// Seek and/or read error.
						UNREF(reader[0]);
						UNREF(reader[1]);
						return nullptr;
					}

					p_dblk += chunk_size;
					block_size -= chunk_size;
				}
				if (block_size > 0) {
					// Empty data at the end of the block.
					// TODO: Error handling.
					reader[rd_idx]->seek_cur(block_size);
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
				UNREF(reader[0]);
				UNREF(reader[1]);
				return nullptr;
			}

			// Verify the MZ header.
			uint16_t mz;
			memcpy(&mz, decompressed_exe.get(), sizeof(mz));
			if (mz != cpu_to_be16('MZ')) {
				// MZ header is not valid.
				// TODO: Other checks?
				UNREF(reader[0]);
				UNREF(reader[1]);
				return nullptr;
			}

			// Copy the PE header.
			lzx_peHeader.resize(PE_HEADER_SIZE);
			memcpy(lzx_peHeader.data(), decompressed_exe.get(), PE_HEADER_SIZE);

			// Copy the XDBF section.
			const XEX2_Resource_Info *const pResInfo = getXdbfResInfo();
			if (pResInfo) {
				const uint32_t load_address = be32_to_cpu(
					(xexType != XexType::XEX1
						? secInfo.xex2.load_address
						: secInfo.xex1.load_address));

				const uint32_t xdbf_physaddr = pResInfo->vaddr - load_address;
				if (xdbf_physaddr + pResInfo->size <= image_size) {
					lzx_xdbfSection.resize(pResInfo->size);
					memcpy(lzx_xdbfSection.data(),
						decompressed_exe.get() + xdbf_physaddr,
						pResInfo->size);
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
		for (size_t i = 0; i < reader.size(); i++) {
			if (!reader[i])
				continue;
			uint16_t mz;
			size_t size = reader[i]->read(&mz, sizeof(mz));
			if (size == sizeof(mz)) {
				if (mz == cpu_to_be16('MZ')) {
					// MZ header is valid.
					// TODO: Other checks?
					this->peReader = reader[i];
					reader[i] = nullptr;
					keyInUse = static_cast<int>(i);
					break;
				}
			}
		}
	}

	// Delete the incorrect CBCReaders.
	UNREF(reader[0]);
	UNREF(reader[1]);

	// CBCReader is open and file decompression has been initialized.
	return this->peReader;
}

/**
 * Format a media ID or disc profile ID.
 *
 * The last four bytes are separated from the rest of the ID.
 * This format is used by abgx360.
 *
 * @param pId Media ID or disc profile ID. (16 bytes)
 * @return Formatted ID.
 */
string Xbox360_XEX_Private::formatMediaID(const uint8_t *pId)
{
	static const char hex_lookup[16] = {
		'0','1','2','3','4','5','6','7',
		'8','9','A','B','C','D','E','F',
	};

	// TODO: Use a string here?
	char buf[(16*2)+2];
	char *p = buf;
	for (unsigned int i = 16; i > 0; i--, pId++, p += 2) {
		p[0] = hex_lookup[*pId >> 4];
		p[1] = hex_lookup[*pId & 0x0F];
		if (i == 5) {
			p[2] = '-';
			p++;
		}
	}
	*p = '\0';

	return string(buf, (16*2)+1);
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
	static const RomFields::AgeRatingsCountry region_conv[14] = {
		RomFields::AgeRatingsCountry::USA,
		RomFields::AgeRatingsCountry::Europe,
		RomFields::AgeRatingsCountry::Finland,
		RomFields::AgeRatingsCountry::Portugal,
		RomFields::AgeRatingsCountry::England,
		RomFields::AgeRatingsCountry::Japan,
		RomFields::AgeRatingsCountry::Germany,
		RomFields::AgeRatingsCountry::Australia,
		RomFields::AgeRatingsCountry::Invalid,		// TODO: NZ (usually the same as AU)
		RomFields::AgeRatingsCountry::SouthKorea,
		RomFields::AgeRatingsCountry::Invalid,		// TODO: Brazil
		RomFields::AgeRatingsCountry::Invalid,		// TODO: FPB?
		RomFields::AgeRatingsCountry::Taiwan,
		RomFields::AgeRatingsCountry::Invalid,		// TODO: Singapore
	};

	// Rating conversion table:
	// - Primary index: Xbox 360 region
	// - Secondary index: Xbox 360 age value, from 0-15
	// - Value: RomFields::age_ratings_t age value.
	// If the Xbox 360 age value is over 15, the rating is invalid.
	// If the age_ratings_t value is -1, the rating is invalid.
	//
	// Values are set using the following formula:
	// - If rating A is 0, and rating B is 2:
	//   - The value for "A" gets slot 0.
	//   - The value for "B" gets slots 1 and 2.
	static const int8_t region_values[14][16] = {
		// USA (ESRB)
		{3, 6, 6, 10, 10, 13, 13, 17, 17, 18, 18, 18, 18, 18, 18, -1},
		// Europe (PEGI)
		{3, 4, 4, 4, 4, 12, 12, 12, 12, 12, 16, 16, 16, 16, 18, -1},
		// Finland (PEGI-FI/MEKU)
		{3, 7, 7, 7, 7, 11, 11, 11, 11, 15, 15, 15, 15, 18, 18, -1},
		// Portugal (PEGI-PT)
		{4, 4, 6, 6, 12, 12, 12, 12, 12, 12, 16, 16, 16, 16, 18, -1},
		// England (BBFC)
		// TODO: How are Universal and PG handled for Nintendo?
		{3, 3, 7, 7, 7, 7, 12, 12, 12, 12, 15, 15, 15, 16, 18, -1},
		// Japan (CERO)
		{0, 12, 12, 15, 15, 17, 17, 18, 18,    -1,-1,-1,-1,-1,-1,-1},
		// Germany (USK)
		{0, 6, 6, 12, 12, 16, 16, 18, 18,      -1,-1,-1,-1,-1,-1,-1},
		// Australia (OFLC_AU)
		// TODO: Is R18+ available on Xbox 360?
		{0, 7, 7, 14, 14, 15, 15, -1,       -1,-1,-1,-1,-1,-1,-1,-1},
		// TODO: NZ
		{-1,-1,-1,-1,-1,-1,-1,-1,           -1,-1,-1,-1,-1,-1,-1,-1},
		// South Korea (KMRB/GRB)
		{0, 12, 12, 15, 15, 18, 18, -1,     -1,-1,-1,-1,-1,-1,-1,-1},

		// TODO: Brazil
		{-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1},
		// TODO: FPB?
		{-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1},
		// TODO: Taiwan
		{-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1},
		// TODO: Singapore
		{-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1},
	};

	// 14 ratings for Xbox 360 games.
	for (unsigned int ridx = 0; ridx < 14; ridx++) {
		const RomFields::AgeRatingsCountry ar_idx = region_conv[ridx];
		if ((int)ar_idx < 0) {
			// Not supported.
			continue;
		}

		const uint8_t xb_val = game_ratings.ratings[ridx];
		if (xb_val >= 16) {
			// Invalid rating.
			continue;
		}

		int8_t rf_val = region_values[ridx][xb_val];
		if (rf_val < 0) {
			// Invalid rating.
			continue;
		}

		age_ratings[(int)ar_idx] = static_cast<uint8_t>(rf_val) | RomFields::AGEBF_ACTIVE;
	}
}

/**
 * Get the minimum kernel version required for this XEX.
 * @return Minimum kernel version, or 0 on error.
 */
Xbox360_Version_t Xbox360_XEX_Private::getMinKernelVersion(void)
{
	Xbox360_Version_t rver;
	rver.u32 = 0;

	// Minimum kernel version is determined by checking the
	// import libraries and taking the maximum version.
	ao::uvector<uint8_t> u8_implib;
	size_t size = getOptHdrData(XEX2_OPTHDR_IMPORT_LIBRARIES, u8_implib);
	if (size < sizeof(XEX2_Import_Libraries_Header) + (sizeof(XEX2_Import_Library_Entry) * 2)) {
		// Too small...
		return rver;
	}

	const XEX2_Import_Libraries_Header *const pLibHdr =
		reinterpret_cast<const XEX2_Import_Libraries_Header*>(u8_implib.data());

	// Pointers
	const uint8_t *p = u8_implib.data();
	const uint8_t *const p_end = p + u8_implib.size();

	// Skip the string table.
	p += sizeof(*pLibHdr) + be32_to_cpu(pLibHdr->str_tbl_size);

	while (p < p_end - sizeof(XEX2_Import_Library_Entry)) {
		// Check the minimum version of this import library.
		const XEX2_Import_Library_Entry *const entry =
			reinterpret_cast<const XEX2_Import_Library_Entry*>(p);
		const uint32_t vmin = be32_to_cpu(entry->version_min.u32);
		if (vmin > rver.u32) {
			rver.u32 = vmin;
		}

		// Next import library entry.
		p += be32_to_cpu(entry->size);
	}

	return rver;
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

	// Attempt to open the EXE section.
	// Assuming a maximum of 8 KB for the PE headers.
	IRpFilePtr peFile_tmp;
#ifdef ENABLE_LIBMSPACK
	if (!lzx_peHeader.empty()) {
		peFile_tmp = std::make_shared<MemFile>(lzx_peHeader.data(), lzx_peHeader.size());
	} else
#endif /* ENABLE_LIBMSPACK */
	{
		peFile_tmp = std::make_shared<PartitionFile>(peReader, 0, PE_HEADER_SIZE);
	}
	if (peFile_tmp->isOpen()) {
		EXE *const pe_exe_tmp = new EXE(peFile_tmp);
		if (pe_exe_tmp->isOpen()) {
			pe_exe = pe_exe_tmp;
		} else {
			pe_exe_tmp->unref();
		}
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
	IRpFilePtr peFile_tmp;
#ifdef ENABLE_LIBMSPACK
	if (!lzx_xdbfSection.empty()) {
		peFile_tmp = std::make_shared<MemFile>(lzx_xdbfSection.data(), lzx_xdbfSection.size());
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
		const uint32_t load_address = be32_to_cpu(
			(xexType != XexType::XEX1
				? secInfo.xex2.load_address
				: secInfo.xex1.load_address));
		uint32_t xdbf_physaddr = pResInfo->vaddr - load_address;

		if (fileFormatInfo.compression_type == XEX2_COMPRESSION_TYPE_BASIC) {
			// File has zero padding removed.
			// Determine the actual physical address.
			auto iter = std::find_if(basicZDataSegments.cbegin(), basicZDataSegments.cend(),
				[xdbf_physaddr](const BasicZDataSeg_t &p) noexcept -> bool {
					return (xdbf_physaddr >= p.vaddr &&
					        xdbf_physaddr < (p.vaddr + p.length));
				});
			if (iter != basicZDataSegments.cend()) {
				// Found the correct segment.
				// Adjust the physical address.
				xdbf_physaddr -= (iter->vaddr - iter->physaddr);
			}
		}
		peFile_tmp = std::make_shared<PartitionFile>(peReader, xdbf_physaddr, pResInfo->size);
	}
	if (peFile_tmp->isOpen()) {
		// FIXME: XEX1 XDBF is either encrypted or garbage...
		Xbox360_XDBF *const pe_xdbf_tmp = new Xbox360_XDBF(peFile_tmp, true);
		if (pe_xdbf_tmp->isOpen()) {
			pe_xdbf = pe_xdbf_tmp;
		} else {
			pe_xdbf_tmp->unref();
		}
	}

	return pe_xdbf;
}

/**
 * Get the publisher.
 * @return Publisher.
 */
string Xbox360_XEX_Private::getPublisher(void) const
{
	if (!isExecutionIDLoaded) {
		const_cast<Xbox360_XEX_Private*>(this)->getXdbfResInfo();
		if (!isExecutionIDLoaded) {
			// Unable to get the publisher.
			return string();
		}
	}

	uint16_t pub_id = ((unsigned int)executionID.title_id.a << 8) |
	                  ((unsigned int)executionID.title_id.b);
	const char *const publisher = XboxPublishers::lookup(pub_id);
	if (publisher) {
		return publisher;
	}

	// Unknown publisher.
	if (ISALNUM(executionID.title_id.a) &&
	    ISALNUM(executionID.title_id.b))
	{
		// Publisher ID is alphanumeric.
		return rp_sprintf(C_("RomData", "Unknown (%c%c)"),
			executionID.title_id.a,
			executionID.title_id.b);
	}

	// Publisher ID is not alphanumeric.
	return rp_sprintf(C_("RomData", "Unknown (%02X %02X)"),
		static_cast<uint8_t>(executionID.title_id.a),
		static_cast<uint8_t>(executionID.title_id.b));
}

/** Xbox360_XEX **/

/**
 * Read an Xbox 360 XEX file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open XEX file.
 */
Xbox360_XEX::Xbox360_XEX(const IRpFilePtr &file)
	: super(new Xbox360_XEX_Private(file))
{
	// This class handles executables.
	RP_D(Xbox360_XEX);
	d->mimeType = "application/x-xbox360-executable";	// unofficial, not on fd.o
	d->fileType = FileType::Executable;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the XEX2 header and optional header table.
	// NOTE: Reading all at once to reduce seeking.
	// NOTE: Limiting to one DVD sector.
	uint8_t header[2048];
	d->file->rewind();
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header)) {
		d->xex2Header.magic = 0;
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(header), header},
		nullptr,	// ext (not needed for Xbox360_XEX)
		0		// szFile (not needed for Xbox360_XEX)
	};
	d->xexType = static_cast<Xbox360_XEX_Private::XexType>(isRomSupported_static(&info));
	d->isValid = ((int)d->xexType >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// Copy the XEX2 header.
	memcpy(&d->xex2Header, header, sizeof(d->xex2Header));
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the header for little-endian systems.
	// NOTE: The magic number is *not* byteswapped here.
	d->xex2Header.module_flags	= be32_to_cpu(d->xex2Header.module_flags);
	d->xex2Header.pe_offset		= be32_to_cpu(d->xex2Header.pe_offset);
	d->xex2Header.reserved		= be32_to_cpu(d->xex2Header.reserved);
	d->xex2Header.sec_info_offset	= be32_to_cpu(d->xex2Header.sec_info_offset);
	d->xex2Header.opt_header_count	= be32_to_cpu(d->xex2Header.opt_header_count);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// Is this a patch?
	if ((d->xex2Header.module_flags & (XEX2_MODULE_FLAG_MODULE_PATCH |
	                                   XEX2_MODULE_FLAG_PATCH_FULL |
                                           XEX2_MODULE_FLAG_PATCH_DELTA)) != 0)
	{
		// This is a patch.
		d->mimeType = "application/x-xbox360-patch";	// unofficial, not on fd.o
		d->fileType = FileType::PatchFile;
	}

	// Read the security info.
	// NOTE: This must be done after copying the XEX2 header,
	// since the security info offset is stored in the header.
	// TODO: Read the offset directly before copying?
	// TODO: Use the XEX1 security info size for XEX1?
	if (d->xex2Header.sec_info_offset + sizeof(d->secInfo) <= 2048) {
		// Security info is in the first sector.
		memcpy(&d->secInfo, &header[d->xex2Header.sec_info_offset], sizeof(d->secInfo));
	} else {
		// Security info is not fully located in the first sector.
		size = d->file->seekAndRead(d->xex2Header.sec_info_offset, &d->secInfo, sizeof(d->secInfo));
		if (size != sizeof(d->secInfo)) {
			// Seek and/or read error.
			d->file.reset();
			d->xexType = Xbox360_XEX_Private::XexType::Unknown;
			d->isValid = false;
			return;
		}
	}

	// Copy the optional header table.
	// Maximum of 64 optional headers.
	assert(d->xex2Header.opt_header_count <= 64);
	const unsigned int opt_header_count = std::min(d->xex2Header.opt_header_count, 64U);
	d->optHdrTbl.resize(opt_header_count);
	const size_t opt_header_sz = (size_t)opt_header_count * sizeof(XEX2_Optional_Header_Tbl);
	memcpy(d->optHdrTbl.data(), &header[sizeof(d->xex2Header)], opt_header_sz);
}

/**
 * Close the opened file.
 */
void Xbox360_XEX::close(void)
{
	RP_D(Xbox360_XEX);

	if (d->pe_xdbf) {
		d->pe_xdbf->close();
	}
	if (d->pe_exe) {
		d->pe_exe->close();
	}

	UNREF_AND_NULL(d->peReader);

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
		return static_cast<int>(Xbox360_XEX_Private::XexType::Unknown);
	}

	// Check for XEX.
	Xbox360_XEX_Private::XexType xexType;
	const XEX2_Header *const xex2Header =
		reinterpret_cast<const XEX2_Header*>(info->header.pData);
	if (xex2Header->magic == cpu_to_be32(XEX2_MAGIC)) {
		// We have an XEX2 file.
		xexType = Xbox360_XEX_Private::XexType::XEX2;
	} else if (xex2Header->magic == cpu_to_be32(XEX1_MAGIC)) {
		// We have an XEX1 file.
		xexType = Xbox360_XEX_Private::XexType::XEX1;
	} else {
		// Not supported.
		xexType = Xbox360_XEX_Private::XexType::Unknown;
	}

	return static_cast<int>(xexType);
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

	// Xbox 360 has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Xbox360_XEX::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	// TODO: XEX-specific, or just use Xbox 360?
	static const char *const sysNames[4] = {
		"Microsoft Xbox 360", "Xbox 360", "X360", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Xbox360_XEX::supportedImageTypes(void) const
{
	RP_D(const Xbox360_XEX);
	const Xbox360_XDBF *const pe_xdbf = const_cast<Xbox360_XEX_Private*>(d)->initXDBF();
	if (pe_xdbf) {
		return pe_xdbf->supportedImageTypes();
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
	const Xbox360_XDBF *const pe_xdbf = const_cast<Xbox360_XEX_Private*>(d)->initXDBF();
	if (pe_xdbf) {
		return pe_xdbf->supportedImageSizes(imageType);
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
	const Xbox360_XDBF *const pe_xdbf = const_cast<Xbox360_XEX_Private*>(d)->initXDBF();
	if (pe_xdbf) {
		return pe_xdbf->imgpf(imageType);
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
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->xexType < 0) {
		// XEX file isn't valid.
		return -EIO;
	}

	// Parse the XEX file.
	// NOTE: The magic number is NOT byteswapped in the constructor.
	const XEX2_Header *const xex2Header = &d->xex2Header;

	// Maximum of 14 fields, not including RomData subclasses.
	const char *const s_xexType = (d->xexType != Xbox360_XEX_Private::XexType::XEX1 ? "XEX2" : "XEX1");
	d->fields.reserve(14);
	d->fields.setTabName(0, s_xexType);

	// Image flags.
	// NOTE: Same for both XEX1 and XEX2 according to Xenia.
	// TODO: Show image flags as-is?
	const uint32_t image_flags = (d->xexType != Xbox360_XEX_Private::XexType::XEX1)
		? be32_to_cpu(d->secInfo.xex2.image_flags)
		: be32_to_cpu(d->secInfo.xex1.image_flags);

	// Is the encryption key available?
	bool noKeyAvailable = false;
	if (d->initPeReader() != nullptr) {
		if (d->keyInUse < 0 &&
		    d->fileFormatInfo.encryption_type != cpu_to_be16(XEX2_ENCRYPTION_TYPE_NONE))
		{
			// Key not available.
			noKeyAvailable = true;
		}
	} else {
		// Unable to initialize the PE reader.
		// Assume the key is not available.
		noKeyAvailable = true;
	}
	if (noKeyAvailable) {
		// FIXME: xextool can detect the encryption keys for
		// delta patches. Figure out how to do that here.
		if (!(d->xex2Header.module_flags & XEX2_MODULE_FLAG_PATCH_DELTA)) {
			// TODO: Xeika key?
			const char *s_xexKeyID;
			if (unlikely(image_flags & XEX2_IMAGE_FLAG_CARDEA_KEY)) {
				s_xexKeyID = "XEX1";
			} else {
				s_xexKeyID = "XEX2";
			}
			d->fields.addField_string(C_("RomData", "Warning"),
				rp_sprintf(C_("Xbox360_XEX", "The Xbox 360 %s encryption key is not available."), s_xexKeyID),
				RomFields::STRF_WARNING);
		}
	}

	// XDBF fields
	const Xbox360_XDBF *const pe_xdbf = d->initXDBF();
	if (pe_xdbf) {
		pe_xdbf->addFields_strings(&d->fields);
	}

	// Original executable name
	ao::uvector<uint8_t> u8_data;
	size_t size = d->getOptHdrData(XEX2_OPTHDR_ORIGINAL_PE_NAME, u8_data);
	if (size > sizeof(uint32_t)) {
		// Sanity check: Must be less than 260 bytes. (PATH_MAX)
		assert(size <= 260+sizeof(uint32_t));
		if (size <= 260+sizeof(uint32_t)) {
			// Got the filename.
			// NOTE: May or may not be NULL-terminated.
			// TODO: What encoding? Assuming cp1252...
			int len = static_cast<int>(size - sizeof(uint32_t));
			d->fields.addField_string(C_("Xbox360_XEX", "PE Filename"),
				cp1252_to_utf8(reinterpret_cast<const char*>(
					u8_data.data() + sizeof(uint32_t)), len),
				RomFields::STRF_TRIM_END);
		}
	}

	// tr: Minimum kernel version (i.e. dashboard)
	//const char *const s_minver = C_("Xbox360_XEX", "Min. Kernel");
	const Xbox360_Version_t minver = d->getMinKernelVersion();
	string s_minver;
	if (minver.u32 != 0) {
		s_minver = rp_sprintf("%u.%u.%u.%u",
			static_cast<unsigned int>(minver.major),
			static_cast<unsigned int>(minver.minor),
			static_cast<unsigned int>(minver.build),
			static_cast<unsigned int>(minver.qfe));
	} else {
		s_minver = C_("RomData", "Unknown");
	}
	if (d->xexType == Xbox360_XEX_Private::XexType::XEX1) {
		// Indicate that an XEX1 kernel is needed.
		s_minver += " (XEX1)";
	}
	d->fields.addField_string(C_("Xbox360_XEX", "Min. Kernel"), s_minver);

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
	d->fields.addField_bitfield(C_("Xbox360_XEX", "Module Flags"),
		v_module_flags, 4, xex2Header->module_flags);

	// Media types
	// NOTE: Using a string instead of a bitfield because very rarely
	// are all of these set, and in most cases, none are.
	// TODO: RFT_LISTDATA?
	// NOTE 2: For Unleashed XEX, XGD2 flag is set and media type is 4 (DVD/CD).
	// For Unleashed XEXP, XGD2 flag is set and media type is 0x18000000.
	if (image_flags & XEX2_IMAGE_FLAG_XGD2_MEDIA_ONLY) {
		// XGD2/XGD3 media only.
		// TODO: Check the Burger King games. (XGD1)
		d->fields.addField_string(C_("Xbox360_XEX", "Media Types"),
			C_("Xbox360_XEX", "Xbox Game Disc only"));
	} else {
		// Other types.
		static const char *const media_type_tbl[] = {
			// 0
			NOP_C_("Xbox360_XEX", "Hard Disk"),
			NOP_C_("Xbox360_XEX", "XGD1"),
			NOP_C_("Xbox360_XEX", "DVD/CD"),
			NOP_C_("Xbox360_XEX", "DVD-ROM SL"),
			// 4
			NOP_C_("Xbox360_XEX", "DVD-ROM DL"),
			NOP_C_("Xbox360_XEX", "System Flash Memory"),
			nullptr,
			NOP_C_("Xbox360_XEX", "Memory Unit"),
			// 8
			NOP_C_("Xbox360_XEX", "USB Mass Storage Device"),
			NOP_C_("Xbox360_XEX", "Network"),
			NOP_C_("Xbox360_XEX", "Direct from RAM"),
			NOP_C_("Xbox360_XEX", "RAM Drive"),
			// 12
			NOP_C_("Xbox360_XEX", "Secure Virtual Optical Device"),
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

		uint32_t media_types = be32_to_cpu(
			(d->xexType != Xbox360_XEX_Private::XexType::XEX1
				? d->secInfo.xex2.allowed_media_types
				: d->secInfo.xex1.allowed_media_types));

		ostringstream oss;
		unsigned int found = 0;
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

		d->fields.addField_string(C_("Xbox360_XEX", "Media Types"),
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
	const uint32_t region_code_xbx = be32_to_cpu(
		(d->xexType != Xbox360_XEX_Private::XexType::XEX1
			? d->secInfo.xex2.region_code
			: d->secInfo.xex1.region_code));
	uint32_t region_code = 0;
	if (region_code_xbx & XEX2_REGION_CODE_NTSC_U) {
		region_code |= (1U << 0);
	}
	if (region_code_xbx & XEX2_REGION_CODE_NTSC_J_JAPAN) {
		region_code |= (1U << 1);
	}
	if (region_code_xbx & XEX2_REGION_CODE_NTSC_J_CHINA) {
		region_code |= (1U << 2);
	}
	if (region_code_xbx & XEX2_REGION_CODE_NTSC_J_OTHER) {
		region_code |= (1U << 3);
	}
	if (region_code_xbx & XEX2_REGION_CODE_PAL_OTHER) {
		region_code |= (1U << 4);
	}
	if (region_code_xbx & XEX2_REGION_CODE_PAL_AU_NZ) {
		// TODO: Combine these bits?
		region_code |= (1U << 5) | (1U << 6);
	}

	vector<string> *const v_region_code = RomFields::strArrayToVector_i18n(
		"Region", region_code_tbl, ARRAY_SIZE(region_code_tbl));
	d->fields.addField_bitfield(C_("RomData", "Region Code"),
		v_region_code, 4, region_code);

	// Media ID
	d->fields.addField_string(C_("Xbox360_XEX", "Media ID"),
		d->formatMediaID(
			(d->xexType != Xbox360_XEX_Private::XexType::XEX1
				? d->secInfo.xex2.xgd2_media_id
				: d->secInfo.xex1.xgd2_media_id)),
		RomFields::STRF_MONOSPACE);

	// Disc Profile ID
	size = d->getOptHdrData(XEX2_OPTHDR_DISC_PROFILE_ID, u8_data);
	if (size == 16) {
		d->fields.addField_string(C_("Xbox360_XEX", "Disc Profile ID"),
			d->formatMediaID(u8_data.data()),
			RomFields::STRF_MONOSPACE);
	}

	/** Execution ID **/
	// NOTE: Initialized by d->getXdbfResInfo().
	d->getXdbfResInfo();
	if (d->isExecutionIDLoaded) {
		// Title ID
		// FIXME: Verify behavior on big-endian.
		// TODO: Consolidate implementations into a shared function.
		string tid_str;
		char hexbuf[4];
		if (ISUPPER(d->executionID.title_id.a)) {
			tid_str += (char)d->executionID.title_id.a;
		} else {
			tid_str += "\\x";
			snprintf(hexbuf, sizeof(hexbuf), "%02X",
				(uint8_t)d->executionID.title_id.a);
			tid_str.append(hexbuf, 2);
		}
		if (ISUPPER(d->executionID.title_id.b)) {
			tid_str += (char)d->executionID.title_id.b;
		} else {
			tid_str += "\\x";
			snprintf(hexbuf, sizeof(hexbuf), "%02X",
				(uint8_t)d->executionID.title_id.b);
			tid_str.append(hexbuf, 2);
		}
			
		d->fields.addField_string(C_("Xbox360_XEX", "Title ID"),
			rp_sprintf_p(C_("Xbox360_XEX", "%1$08X (%2$s-%3$04u)"),
				be32_to_cpu(d->executionID.title_id.u32),
				tid_str.c_str(),
				be16_to_cpu(d->executionID.title_id.u16)),
			RomFields::STRF_MONOSPACE);

		// Publisher
		const string publisher = d->getPublisher();
		if (!publisher.empty()) {
			d->fields.addField_string(C_("RomData", "Publisher"), publisher);
		}

		// Disc number
		// NOTE: Not shown for single-disc games.
		if (d->executionID.disc_number != 0 && d->executionID.disc_count > 1) {
			d->fields.addField_string(C_("RomData", "Disc #"),
				// tr: Disc X of Y (for multi-disc games)
				rp_sprintf_p(C_("RomData|Disc", "%1$u of %2$u"),
					d->executionID.disc_number,
					d->executionID.disc_count));
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
				// FIXME: xextool can detect the encryption keys for
				// delta patches. Figure out how to do that here.
				if (!(d->xex2Header.module_flags & XEX2_MODULE_FLAG_PATCH_DELTA)) {
					s_encryption_key = C_("RomData", "Unknown");
				} else {
					s_encryption_key = C_("Xbox360_XEX|EncKey", "Cannot Determine");
				}
				break;
			case 0:
				s_encryption_key = C_("Xbox360_XEX|EncKey", "Retail");
				break;
			case 1:
				s_encryption_key = C_("Xbox360_XEX|EncKey", "Debug");
				break;
		}
	}
	d->fields.addField_string(C_("Xbox360_XEX", "Encryption Key"), s_encryption_key);

	// Compression
	static const char *const compression_tbl[] = {
		NOP_C_("Xbox360_XEX|Compression", "None"),
		NOP_C_("Xbox360_XEX|Compression", "Basic (Sparse)"),
		NOP_C_("Xbox360_XEX|Compression", "Normal (LZX)"),
		NOP_C_("Xbox360_XEX|Compression", "Delta"),
	};
	if (d->fileFormatInfo.compression_type < ARRAY_SIZE(compression_tbl)) {
		d->fields.addField_string(C_("Xbox360_XEX", "Compression"),
			dpgettext_expr(RP_I18N_DOMAIN, "Xbox360_XEX|Compression",
				compression_tbl[d->fileFormatInfo.compression_type]));
	} else {
		d->fields.addField_string(C_("Xbox360_XEX", "Compression"),
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"),
				d->fileFormatInfo.compression_type));
	}

	/** Age ratings **/
	// NOTE: RomFields' RFT_AGE_RATINGS type uses a format that matches
	// Nintendo's systems. For Xbox 360, we'll need to convert the format.
	// NOTE: The actual game ratings field is 64 bytes, but only the first
	// 14 bytes are actually used.
	size = d->getOptHdrData(XEX2_OPTHDR_GAME_RATINGS, u8_data);
	if (size >= sizeof(XEX2_Game_Ratings)) {
		const XEX2_Game_Ratings *const pLdGameRatings =
			reinterpret_cast<const XEX2_Game_Ratings*>(u8_data.data());

		// Convert the game ratings.
		RomFields::age_ratings_t age_ratings;
		d->convertGameRatings(age_ratings, *pLdGameRatings);
		d->fields.addField_ageRatings(C_("RomData", "Age Ratings"), age_ratings);
	}

	// NOTE: If this is an XEXP with a delta patch instead of a
	// full patch, we can't get any useful information from the
	// EXE or XDBF sections.
	if (!(xex2Header->module_flags & XEX2_MODULE_FLAG_PATCH_DELTA)) {
		// Can we get the EXE section?
		const EXE *const pe_exe = const_cast<Xbox360_XEX_Private*>(d)->initEXE();
		if (pe_exe) {
			// Add the fields.
			const RomFields *const exeFields = pe_exe->fields();
			if (exeFields) {
				d->fields.addFields_romFields(exeFields, RomFields::TabOffset_AddTabs);
			}
		}

		// Can we get the XDBF section?
		if (pe_xdbf) {
			// Add the fields.
			const RomFields *const xdbfFields = pe_xdbf->fields();
			if (xdbfFields) {
				d->fields.addFields_romFields(xdbfFields, RomFields::TabOffset_AddTabs);
			}
		}
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
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
	} else if (!d->isValid || (int)d->xexType < 0) {
		// XEX file isn't valid.
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
	d->metaData->reserve(2);	// Maximum of 2 metadata properties.

	// NOTE: RomMetaData ignores empty strings, so we don't need to
	// check for them here.

	// Title
	d->metaData->addMetaData_string(Property::Title, xdbf->getString(Property::Title));

	// Publisher
	d->metaData->addMetaData_string(Property::Publisher, d->getPublisher());

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[i] Image type to load.
 * @param pImage	[out] Reference to shared_ptr<const rp_image> to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int Xbox360_XEX::loadInternalImage(ImageType imageType, shared_ptr<const rp_image> &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(Xbox360_XEX);
	const Xbox360_XDBF *const pe_xdbf = d->initXDBF();
	if (pe_xdbf) {
		return const_cast<Xbox360_XDBF*>(pe_xdbf)->loadInternalImage(imageType, pImage);
	}

	return -ENOENT;
}

/**
 * Check for "viewed" achievements.
 *
 * @return Number of achievements unlocked.
 */
int Xbox360_XEX::checkViewedAchievements(void) const
{
	RP_D(const Xbox360_XEX);
	if (!d->isValid) {
		// XEX is not valid.
		return 0;
	}

	// keyInUse should have been initialized somewhere else.
	// Initializing it here won't work because the file may
	// have been closed already.
	// TODO: Initialize the PE Reader in the constructor?

	Achievements *const pAch = Achievements::instance();
	int ret = 0;

	if (d->keyInUse == 1) {
		// Debug encryption.
		pAch->unlock(Achievements::ID::ViewedDebugCryptedFile);
		ret++;
	}

	// Check the EXE for achievements.
	const EXE *exe = const_cast<Xbox360_XEX_Private*>(d)->initEXE();
	if (exe) {
		ret += exe->checkViewedAchievements();
	}

	return ret;
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
