/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DSFirm.hpp: Nintendo 3DS firmware reader.                      *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "librpbase/config.librpbase.h"

#include "Nintendo3DSFirm.hpp"
#include "RomData_p.hpp"

#include "n3ds_firm_structs.h"
#include "data/Nintendo3DSFirmData.hpp"

// Other rom-properties libraries
#include "librpbase/crypto/Hash.hpp"
#ifdef ENABLE_DECRYPTION
#  include "librpbase/crypto/AesCipherFactory.hpp"
#  include "librpbase/crypto/IAesCipher.hpp"
#  include "librpbase/crypto/KeyManager.hpp"
#endif /* ENABLE_DECRYPTION */
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// for memmem() if it's not available in <string.h>
#include "librptext/libc.h"

// C includes
#include "ctypex.h"

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;

// Uninitialized vector class
#include "uvector.h"

namespace LibRomData {

class Nintendo3DSFirmPrivate final : public RomDataPrivate
{
public:
	explicit Nintendo3DSFirmPrivate(const IRpFilePtr &file, Nintendo3DSFirm::StorageType type);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(Nintendo3DSFirmPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 2+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// Firmware header
	// NOTE: Must be byteswapped on access.
	N3DS_FIRM_Header_t firmHeader;

	// Storage type
	Nintendo3DSFirm::StorageType type;

public:
	static constexpr off64_t FIRM_MAX_SIZE = 4*1024*1024;

	/**
	 * Load the firmware binary. (excluding header)
	 *
	 * For NTR/SPI, the binary will also be decrypted
	 * if the keys are available.
	 *
	 * @param firmBuf Buffer for the firmware binary
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadFirmBin(rp::uvector<uint8_t> &firmBuf);
};

ROMDATA_IMPL(Nintendo3DSFirm)

/** Nintendo3DSFirmPrivate **/

/* RomDataInfo */
const array<const char*, 2+1> Nintendo3DSFirmPrivate::exts = {{
	".firm",	// boot9strap
	".bin",		// older

	nullptr
}};
const array<const char*, 1+1> Nintendo3DSFirmPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-nintendo-3ds-firm",

	nullptr
}};
const RomDataInfo Nintendo3DSFirmPrivate::romDataInfo = {
	"Nintendo3DSFirm", exts.data(), mimeTypes.data()
};

Nintendo3DSFirmPrivate::Nintendo3DSFirmPrivate(const IRpFilePtr &file, Nintendo3DSFirm::StorageType type)
	: super(file, &romDataInfo)
	, type(type)
{
	// Clear the various structs.
	memset(&firmHeader, 0, sizeof(firmHeader));
}

/**
 * Load the firmware binary. (excluding header)
 *
 * For NTR/SPI, the binary will also be decrypted
 * if the keys are available.
 *
 * @param firmBuf Buffer for the firmware binary
 * @return 0 on success; negative POSIX error code on error.
 */
int Nintendo3DSFirmPrivate::loadFirmBin(rp::uvector<uint8_t> &firmBuf)
{
	firmBuf.clear();

	off64_t szFile64 = file->size();
	if (szFile64 < static_cast<off64_t>(sizeof(N3DS_FIRM_Header_t))) {
		// Firmware file is too small...
		return -EIO;
	}
	szFile64 -= sizeof(N3DS_FIRM_Header_t);
	if (szFile64 > FIRM_MAX_SIZE) {
		// Firmware file is too big.
		return -E2BIG;	// FIXME: Probably wrong...
	}

	// Firmware binary is 4 MB or less.
	const unsigned int szFile = static_cast<unsigned int>(szFile64);
	firmBuf.resize(szFile);
	size_t size = file->seekAndRead(sizeof(N3DS_FIRM_Header_t), firmBuf.data(), firmBuf.size());
	if (size != szFile) {
		// Error reading the firmware binary.
		firmBuf.clear();
		int ret = file->lastError();
		if (ret == 0) {
			ret = -EIO;
		}
		return ret;
	}

#ifdef ENABLE_DECRYPTION
	if (type > Nintendo3DSFirm::StorageType::NAND) {
		// Decrypt the sections.
		// - IV is [offset, load_addr, size, size] (all are packed as 32-bit little-endian)
		// - Key depends on prod/devel and NTR/SPI.
		// TODO: Heuristic for prod/devel. Assuming devel for now.
		// Check for NCCH for official images, or maybe "has at least one 32-bit 00000000" for unofficial?
		const char *keyName;
		switch (type) {
			default:
				assert(!"Key not supported?!?!");
				// TODO: Some other error?
				return -ENOENT;
			case Nintendo3DSFirm::StorageType::NTR:
				keyName = "ctr-dev-ntr-boot";
				break;
			case Nintendo3DSFirm::StorageType::SPI:
				keyName = "ctr-dev-spi-boot";
				break;
		}

		KeyManager *const keyManager = KeyManager::instance();
		assert(keyManager != nullptr);
		if (!keyManager) {
			// TODO: Some other error?
			return -EIO;
		}

		KeyManager::KeyData_t keyData;
		KeyManager::VerifyResult res = keyManager->get(keyName, &keyData);
		if (res != KeyManager::VerifyResult::OK) {
			// TODO: Some other error?
			return -EIO;
		}

		// Initialize an AesCipher.
		unique_ptr<IAesCipher> cipher(AesCipherFactory::create());
		assert((bool)cipher);
		if (!cipher) {
			// No AES cipher is available...
			// TODO: Some other error?
			return -ENOTSUP;
		}

		// Set decryption parameters.
		cipher->setChainingMode(IAesCipher::ChainingMode::CBC);

		// Decrypt each section.
		for (const N3DS_FIRM_Section_Header_t &section : firmHeader.sections) {
			if (section.size == 0) {
				// Empty section. Skip it.
				continue;
			}

			uint32_t addr = le32_to_cpu(section.offset);
			assert(addr >= sizeof(N3DS_FIRM_Header_t));
			if (addr < sizeof(N3DS_FIRM_Header_t)) {
				// Invalid starting address.
				// TODO: Return an error.
				continue;
			}
			addr -= sizeof(N3DS_FIRM_Header_t);

			uint32_t size = le32_to_cpu(section.size);
			assert(addr + size <= firmBuf.size());
			if (addr + size > firmBuf.size()) {
				// Out of bounds?
				// TODO: Return an error.
				continue;
			}

			// Section sizes must be a mutiple of 16.
			assert(size % 16 == 0);
			if (size % 16 != 0) {
				// Round it up...
				size &= ~15;
				size += 16;
			}

			// Set the key and IV.
			union {
				uint32_t u32[4];
				uint8_t u8[16];
			} iv;
			iv.u32[0] = section.offset;
			iv.u32[1] = section.load_addr;
			iv.u32[2] = section.size;
			iv.u32[3] = section.size;
			cipher->setKey(keyData.key, keyData.length);
			cipher->setIV(iv.u8, sizeof(iv.u8));

			// Decrypt the data.
			// TODO: Check for errors.
			cipher->decrypt(&firmBuf[addr], size);
		}
	}
#endif /* ENABLE_DECRYPTION */

	// TODO: Decrypt the firmware binary, if necessary.
	return 0;
}

/** Nintendo3DSFirm **/

/**
 * Read a Nintendo 3DS firmware binary.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image
 * @param type Storage type; used to determine if decryption is needed.
 */
Nintendo3DSFirm::Nintendo3DSFirm(const IRpFilePtr &file, StorageType type)
	: super(new Nintendo3DSFirmPrivate(file, type))
{
	RP_D(Nintendo3DSFirm);
	d->mimeType = "application/x-nintendo-3ds-firm";	// unofficial, not on fd.o
	d->fileType = FileType::FirmwareBinary;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the firmware header.
	d->file->rewind();
	size_t size = d->file->read(&d->firmHeader, sizeof(d->firmHeader));
	if (size != sizeof(d->firmHeader)) {
		d->file.reset();
		return;
	}

	// Check if this firmware binary is supported.
	const DetectInfo info = {
		{0, sizeof(d->firmHeader), reinterpret_cast<const uint8_t*>(&d->firmHeader)},
		nullptr,	// ext (not needed for Nintendo3DSFirm)
		0		// szFile (not needed for Nintendo3DSFirm)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Nintendo3DSFirm::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(N3DS_FIRM_Header_t))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check the FIRM magic.
	// TODO: Other checks?
	const N3DS_FIRM_Header_t *const firmHeader =
		reinterpret_cast<const N3DS_FIRM_Header_t*>(info->header.pData);
	if (firmHeader->magic == cpu_to_be32(N3DS_FIRM_MAGIC)) {
		// This is a FIRM binary.
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
const char *Nintendo3DSFirm::systemName(unsigned int type) const
{
	RP_D(const Nintendo3DSFirm);
	if (!d->isValid || !isSystemNameTypeValid(type)) {
		return nullptr;
	}

	// Nintendo 3DS has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Nintendo3DSFirm::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	// TODO: *New* Nintendo 3DS for N3DS-exclusive titles; iQue for China.
	static const array<const char*, 4> sysNames = {{
		"Nintendo 3DS", "Nintendo 3DS", "3DS", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Nintendo3DSFirm::loadFieldData(void)
{
	RP_D(Nintendo3DSFirm);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Firmware binary isn't valid.
		return -EIO;
	}

	// Nintendo 3DS firmware binary header.
	const N3DS_FIRM_Header_t *const firmHeader = &d->firmHeader;
	d->fields.reserve(6);	// Maximum of 6 fields.

	// Read the firmware binary.
	// NOTE: firmBuf does *not* include the FIRM header.
	rp::uvector<uint8_t> firmBuf;
	int ret = d->loadFirmBin(firmBuf);
	// TODO: If decryption failed, show a warning.

	// If both ARM11 and ARM9 entry points are non-zero,
	// check if this is an official 3DS firmware binary.
	const Nintendo3DSFirmData::FirmBin_t *firmBin = nullptr;
	const uint32_t arm11_entrypoint = le32_to_cpu(firmHeader->arm11_entrypoint);
	const uint32_t arm9_entrypoint = le32_to_cpu(firmHeader->arm9_entrypoint);
	const char *firmBinDesc = nullptr;
	bool checkCustomFIRM = false;	// Check for a custom FIRM, e.g. Boot9Strap.
	bool checkARM9 = false;		// Check for ARM9 homebrew.
	if (arm11_entrypoint != 0 && arm9_entrypoint != 0) {
		// Calculate the CRC32 and look it up.
		// TODO: Check firmBuf before initializing CRC32?
		Hash crc32Hash(Hash::Algorithm::CRC32);
		if (crc32Hash.isUsable() && !firmBuf.empty()) {
			// NOTE: The CRC32s include the FIRM header.
			crc32Hash.process(firmHeader, sizeof(*firmHeader));
			crc32Hash.process(firmBuf.data(), firmBuf.size());
			const uint32_t crc = crc32Hash.getHash32();
			firmBin = Nintendo3DSFirmData::lookup_firmBin(crc);
			if (firmBin != nullptr) {
				// Official firmware binary.
				if (firmBin->flags & Nintendo3DSFirmData::FLAG_New3DS) {
					// New3DS
					if (firmBin->flags & Nintendo3DSFirmData::FLAG_AGB_FIRM) {
						firmBinDesc = "New3DS AGB_FIRM";
					} else if (firmBin->flags & Nintendo3DSFirmData::FLAG_TWL_FIRM) {
						firmBinDesc = "New3DS TWL_FIRM";
					} else {
						firmBinDesc = "New3DS NATIVE_FIRM";
					}
				} else {
					// Old3DS
					if (firmBin->flags & Nintendo3DSFirmData::FLAG_AGB_FIRM) {
						firmBinDesc = "Old3DS AGB_FIRM";
					} else if (firmBin->flags & Nintendo3DSFirmData::FLAG_TWL_FIRM) {
						firmBinDesc = "Old3DS TWL_FIRM";
					} else {
						firmBinDesc = "Old3DS NATIVE_FIRM";
					}
				}
			} else {
				// Check for a custom FIRM.
				checkCustomFIRM = true;

				// NOTE: Luma3DS v9.1 has an ARM11 entry point set,
				// so we should check for ARM9 homebrew as well.
				checkARM9 = true;
			}
		}
	} else if (arm11_entrypoint == 0 && arm9_entrypoint != 0) {
		// ARM9 homebrew
		firmBinDesc = C_("Nintendo3DSFirm", "ARM9 Homebrew");
		checkARM9 = true;
	} else if (arm11_entrypoint != 0 && arm9_entrypoint == 0) {
		// ARM11 homebrew (Not a thing...?)
		firmBinDesc = C_("Nintendo3DSFirm", "ARM11 Homebrew");
	}

	if (checkCustomFIRM) {
		// TODO: Split into a separate function?

		// Check for "B9S" at 0x3D.
		if (!memcmp(&firmHeader->reserved[0x2D], "B9S", 3)) {
			// This is Boot9Strap.
			firmBinDesc = "Boot9Strap";
		} else if (!firmBuf.empty()) {
			// Check for sighax installer.
			// NOTE: String has a NULL terminator.
			static constexpr char sighax_magic[] = "3DS BOOTHAX INS";
			if (!memcmp(&firmBuf[0x208], sighax_magic, sizeof(sighax_magic))) {
				// Found derrek's sighax installer.
				firmBinDesc = "sighax installer";
			}
		}
	}

	if (firmBin) {
		// Official firmware binary fields
		d->fields.addField_string(C_("RomData", "Type"),
			(firmBinDesc ? firmBinDesc : C_("RomData", "Unknown")));

		// FIRM version
		d->fields.addField_string(C_("Nintendo3DSFirm", "FIRM Version"),
			fmt::format(FSTR("{:d}.{:d}-{:d}"), firmBin->kernel.major,
				firmBin->kernel.minor, firmBin->kernel.revision));

		// System version
		d->fields.addField_string(C_("Nintendo3DSFirm", "System Version"),
			fmt::format(FSTR("{:d}.{:d}"), firmBin->sys.major, firmBin->sys.minor));

		// TODO: Check sections for NCCH headers?
	} else if (!firmBuf.empty() && checkARM9) {
		// Check for ARM9 homebrew

		// Version strings
		struct arm9VerStr_t {
			const char *title;	// Application title.
			const char *searchstr;	// Search string.
			unsigned int searchlen;	// Search string length, without the NULL terminator.
		};
		static constexpr array<arm9VerStr_t, 9> arm9VerStr_tbl = {{
			{"Luma3DS",		"Luma3DS v", 9},
			{"GodMode9",		"GodMode9 Explorer v", 19},	// Older versions
			{"GodMode9",		"GodMode9 v", 10},		// Newer versions (v1.9.1; TODO check for first one?)
			{"Decrypt9WIP",		"Decrypt9WIP (", 13},
			{"Hourglass9",		"Hourglass9 v", 12},
			{"ntrboot_flasher",	"ntrboot_flasher: %s", 19},	// version info isn't hard-coded
			{"SafeB9SInstaller",	"SafeB9SInstaller v", 18},
			{"OpenFirmInstaller",	"OpenFirmInstaller v", 19},
			{"fastboot3DS",		"fastboot3DS v", 13},
		}};

		const char *arm9VerStr_title = nullptr;
		string s_verstr;
		for (const auto &p : arm9VerStr_tbl) {
			const char *verstr = static_cast<const char*>(memmem(
				firmBuf.data(), firmBuf.size(), p.searchstr, p.searchlen));
			if (!verstr)
				continue;

			arm9VerStr_title = p.title;

			// Version does NOT include the 'v' character.
			verstr += p.searchlen;
			const char *const end = reinterpret_cast<const char*>(firmBuf.data()) + firmBuf.size();
			int count = 0;
			while (verstr < end && count < 32 && verstr[count] != 0 &&
			       !ISSPACE(verstr[count]) && verstr[count] != ')')
			{
				count++;
			}
			if (count > 0) {
				// Make sure this is labeled as ARM9 homebrew.
				firmBinDesc = C_("Nintendo3DSFirm", "ARM9 Homebrew");

				// NOTE: Most ARM9 homebrew uses UTF-8 strings.
				s_verstr.assign(verstr, count);
			}

			// We're done here.
			break;
		}

		// Sighax status.
		// TODO: If it's SPI, we need to decrypt the FIRM contents.
		// Reference: https://github.com/TuxSH/firmtool/blob/master/firmtool/__main__.py
		const uint32_t first4 = be32_to_cpu(firmHeader->signature32[0]);
		struct sighaxStatus_tbl_t {
			uint32_t first4;
			char status[16];
		};
		static constexpr array<sighaxStatus_tbl_t, 7> sighaxStatus_tbl = {{
			{0xB6724531,	"NAND retail"},		// SciresM
			{0x6EFF209C,	"NAND retail"},		// sighax.com
			{0x88697CDC,	"NAND debug"},		// SciresM

			{0x6CF52F89,	"NCSD retail"},
			{0x53CB0E4E,	"NCSD debug"},

			{0x37E96B10,	"NTR/SPI retail"},
			{0x18722BC7,	"NTR/SPI debug"},
		}};

		const char *s_sighax_status = nullptr;
		auto iter = std::find_if(sighaxStatus_tbl.cbegin(), sighaxStatus_tbl.cend(),
			[first4](const sighaxStatus_tbl_t &p) noexcept -> bool {
				return (p.first4 == first4);
			});
		if (iter != sighaxStatus_tbl.cend()) {
			s_sighax_status = iter->status;
		}

		if (s_sighax_status) {
			// Sighaxed. Assume it's ARM9 homebrew.
			firmBinDesc = C_("Nintendo3DSFirm", "ARM9 Homebrew");
		} else {
			// Not sighaxed.
			s_sighax_status = C_("Nintendo3DSFirm", "Not sighaxed");
		}

		// Add the firmware type field.
		d->fields.addField_string(C_("RomData", "Type"),
			(firmBinDesc ? firmBinDesc : C_("RomData", "Unknown")));

		if (arm9VerStr_title) {
			d->fields.addField_string(C_("RomData", "Title"), arm9VerStr_title);
		}

		// If the version was found, add it.
		if (!s_verstr.empty()) {
			d->fields.addField_string(C_("RomData", "Version"), s_verstr);
		}

		d->fields.addField_string(C_("Nintendo3DSFirm", "Sighax Status"), s_sighax_status);
	} else {
		// Add the firmware type field.
		d->fields.addField_string(C_("RomData", "Type"),
			(firmBinDesc ? firmBinDesc : C_("RomData", "Unknown")));
	}

	// Entry Points
	if (arm11_entrypoint != 0) {
		d->fields.addField_string_numeric(C_("Nintendo3DSFirm", "ARM11 Entry Point"),
			arm11_entrypoint, RomFields::Base::Hex, 8, RomFields::STRF_MONOSPACE);
	}
	if (arm9_entrypoint != 0) {
		d->fields.addField_string_numeric(C_("Nintendo3DSFirm", "ARM9 Entry Point"),
			arm9_entrypoint, RomFields::Base::Hex, 8, RomFields::STRF_MONOSPACE);
	}

	// Finished reading the field data.
	return d->fields.count();
}

} // namespace LibRomData
