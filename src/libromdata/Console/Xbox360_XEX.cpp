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

#include "Xbox360_XEX.hpp"
#include "Xbox360_XDBF.hpp"
#include "librpbase/RomData_p.hpp"

#include "xbox360_xex_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/disc/CBCReader.hpp"
#include "librpbase/disc/PartitionFile.hpp"
#include "librpbase/file/IRpFile.hpp"
using namespace LibRpBase;

#ifdef ENABLE_DECRYPTION
# include "librpbase/crypto/IAesCipher.hpp"
# include "librpbase/crypto/AesCipherFactory.hpp"
# include "librpbase/crypto/KeyManager.hpp"
#endif /* ENABLE_DECRYPTION */

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
		// NOTE: **NOT** byteswapped!
		ao::uvector<XEX2_Optional_Header_Tbl> optHdrTbl;

		/**
		 * Get the specified optional header table entry.
		 * @param header_id Optional header ID.
		 * @return Optional header table entry, or nullptr if not found.
		 */
		const XEX2_Optional_Header_Tbl *getOptHdrTblEntry(uint32_t header_id) const;

	public:
		// CBC reader for encrypted PE executables.
		// Also used for unencrypted executables.
		CBCReader *peReader;
		PartitionFile *peFile;	// uses peReader
		Xbox360_XDBF *pe_xdbf;	// uses file

		/**
		 * Initialize the PE executable reader.
		 * @return peReader on success; nullptr on error.
		 */
		CBCReader *initPeReader(void);

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
	, peReader(nullptr)
	, peFile(nullptr)
	, pe_xdbf(nullptr)
{
	// Clear the headers.
	memset(&xex2Header, 0, sizeof(xex2Header));
	memset(&xex2Security, 0, sizeof(xex2Security));
}

Xbox360_XEX_Private::~Xbox360_XEX_Private()
{
	if (pe_xdbf) {
		pe_xdbf->unref();
	}
	delete peFile;
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
 * Initialize the PE executable reader.
 * @return peReader on success; nullptr on error.
 */
CBCReader *Xbox360_XEX_Private::initPeReader(void)
{
	if (peReader) {
		// PE Reader is already initialized.
		return peReader;
	}

	// Get the file format info.
	const XEX2_Optional_Header_Tbl *const entry = getOptHdrTblEntry(XEX2_OPTHDR_FILE_FORMAT_INFO);
	if (!entry) {
		// No file format info.
		return nullptr;
	}

	XEX2_File_Format_Info ffi;
	size_t size = file->seekAndRead(be32_to_cpu(entry->offset), &ffi, sizeof(ffi));
	if (size != sizeof(ffi)) {
		// Seek and/or read error.
		return nullptr;
	}

	// TODO: Verify structure size.
	// FIXME: Sonic'06 has compression type "normal", but it isn't compressed?
	// Unless that's the section padding compression...
#if 0
	if (ffi.compression_type != cpu_to_be16(XEX2_COMPRESSION_TYPE_NONE)) {
		// Compressed PE executables aren't supported at the moment.
		return nullptr;
	}
#endif

	const size_t pe_length = (size_t)file->size() - xex2Header.pe_offset;
	CBCReader *reader;
	if (ffi.encryption_type == cpu_to_be16(XEX2_ENCRYPTION_TYPE_NONE)) {
		// No encryption.
		reader = new CBCReader(file, xex2Header.pe_offset, pe_length, nullptr, nullptr);
	} else {
		// Decrypt the title key.
		// Get the Key Manager instance.
		KeyManager *const keyManager = KeyManager::instance();
		assert(keyManager != nullptr);

		// Get the common key.
		// TODO: Test debug vs. retail.
		// Assuming retail for now.
		KeyManager::KeyData_t keyData;
		KeyManager::VerifyResult verifyResult = keyManager->getAndVerify(
			EncryptionKeyNames[0], &keyData,
			EncryptionKeyVerifyData[0], 16);
		if (verifyResult != KeyManager::VERIFY_OK) {
			// An error occurred loading while the common key.
			return nullptr;
		}

		// Load the common key. (CBC mode)
		unique_ptr<IAesCipher> cipher(AesCipherFactory::create());
		int ret = cipher->setKey(keyData.key, keyData.length);
		ret |= cipher->setChainingMode(IAesCipher::CM_CBC);
		if (ret != 0) {
			// Error initializing the cipher.
			return nullptr;
		}

		// Zero IV.
		uint8_t iv_zero[16];
		memset(iv_zero, 0, sizeof(iv_zero));

		// Decrypt the title key.
		uint8_t title_key[16];
		memcpy(title_key, xex2Security.title_key, sizeof(title_key));
		if (cipher->decrypt(title_key, sizeof(title_key), iv_zero, sizeof(iv_zero)) != sizeof(title_key)) {
			// Error decrypting the title key.
			return nullptr;
		}

		// Initialize the CBCReader.
		reader = new CBCReader(file, xex2Header.pe_offset, pe_length, title_key, iv_zero);
	}

	if (!reader->isOpen()) {
		// Unable to open the CBCReader.
		delete reader;
		return nullptr;
	}

	// CBCReader is open.
	this->peReader = reader;
	return reader;
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
		d->xex2Header.magic = 0;
		delete d->file;
		d->file = nullptr;
		return;
	}

	// Attempt to open the PE executable.
	if (d->initPeReader()) {
		// FIXME: This is a hard-coded value for Sonic'06.
		// Need to handle padding properly to decode the resource vaddr.
		static const uint32_t xdbf_address = 0xD36900;
		static const uint32_t xdbf_length = 235264;
		PartitionFile *const peFile_tmp = new PartitionFile(d->peReader, xdbf_address, xdbf_length);
		if (peFile_tmp->isOpen()) {
			Xbox360_XDBF *const pe_xdbf_tmp = new Xbox360_XDBF(peFile_tmp);
			if (pe_xdbf_tmp->isOpen()) {
				d->peFile = peFile_tmp;
				d->pe_xdbf = pe_xdbf_tmp;
			} else {
				pe_xdbf_tmp->unref();
				delete peFile_tmp;
			}
		} else {
			delete peFile_tmp;
		}
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

	delete d->peFile;
	delete d->peReader;
	d->peFile = nullptr;
	d->peReader = nullptr;

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

	// FIXME: Load the PE section if it isn't already loaded.
	if (d->pe_xdbf) {
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

	// FIXME: Load the PE section if it isn't already loaded.
	if (d->pe_xdbf) {
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

	// FIXME: Load the PE section if it isn't already loaded.
	if (d->pe_xdbf) {
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

	// Maximum of 8 fields.
	d->fields->reserve(8);
	d->fields->setTabName(0, "XEX");

	// TODO: Game name from XDBF.

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

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
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
