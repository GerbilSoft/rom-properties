/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiWAD.cpp: Nintendo Wii WAD file reader.                               *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"

#include "WiiWAD.hpp"
#include "WiiWAD_p.hpp"

#include "data/NintendoLanguage.hpp"
#include "data/WiiSystemMenuVersion.hpp"
#include "GameCubeRegions.hpp"
#include "WiiCommon.hpp"

// Other rom-properties libraries
#include "librpbase/Achievements.hpp"
#include "librpbase/SystemRegion.hpp"
#include "librpfile/MemFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// Decryption
#ifdef ENABLE_DECRYPTION
#  include "librpbase/crypto/AesCipherFactory.hpp"
#  include "librpbase/crypto/IAesCipher.hpp"
#  include "librpbase/disc/CBCReader.hpp"
// For sections delegated to other RomData subclasses.
#  include "librpbase/disc/PartitionFile.hpp"
#endif /* ENABLE_DECRYPTION */

// RomData subclasses.
#include "WiiWIBN.hpp"
#include "Handheld/NintendoDS.hpp"

// C++ STL classes
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(WiiWAD)

/** WiiWADPrivate **/

/* RomDataInfo */
const char *const WiiWADPrivate::exts[] = {
	".wad",		// Nintendo WAD Format
	".bwf",		// BroadOn WAD Format
	".tad",		// DSi TAD (similar to Nintendo WAD)

	nullptr
};
const char *const WiiWADPrivate::mimeTypes[] = {
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-wii-wad",

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-nintendo-dsi-tad",

	// for compatibility
	"application/x-doom-wad",

	nullptr
};
const RomDataInfo WiiWADPrivate::romDataInfo = {
	"WiiWAD", exts, mimeTypes
};

WiiWADPrivate::WiiWADPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, wadType(WadType::Unknown)
	, data_offset(0)
	, data_size(0)
	, pIMETContent(nullptr)
	, imetContentOffset(0)
	, key_idx(WiiTicket::EncryptionKeys::Unknown)
	, key_status(KeyManager::VerifyResult::Unknown)
{
	// Clear the various structs.
	memset(&wadHeader, 0, sizeof(wadHeader));
	memset(&ticket, 0, sizeof(ticket));
	memset(&tmdHeader, 0, sizeof(tmdHeader));

#ifdef ENABLE_DECRYPTION
	memset(dec_title_key, 0, sizeof(dec_title_key));
	memset(&imet, 0, sizeof(imet));
#endif /* ENABLE_DECRYPTION */
}

/**
 * Get the game information string from the banner.
 * @return Game information string, or empty string on error.
 */
string WiiWADPrivate::getGameInfo(void)
{
	// TODO: Check for DSi SRL.

#ifdef ENABLE_DECRYPTION
	// IMET header.
	// TODO: Read on demand instead of always reading in the constructor.
	if (imet.magic != cpu_to_be32(WII_IMET_MAGIC)) {
		// Not valid.
		return {};
	}

	// TODO: Combine with GameCubePrivate::wii_getBannerName()?

	// Get the system language.
	// TODO: Verify against the region code somehow?
	int lang = NintendoLanguage::getWiiLanguage();

	// If the language-specific name is empty,
	// revert to English.
	if (imet.names[lang][0][0] == 0) {
		// Revert to English.
		lang = WII_LANG_ENGLISH;
	}

	// NOTE: The banner may have two lines.
	// Each line is a maximum of 21 characters.
	// Convert from UTF-16 BE and split into two lines at the same time.
	string info = utf16be_to_utf8(imet.names[lang][0], 21);
	if (imet.names[lang][1][0] != 0) {
		info += '\n';
		info += utf16be_to_utf8(imet.names[lang][1], 21);
	}

	return info;
#else /* !ENABLE_DECRYPTION */
	// Unable to decrypt the IMET header.
	return {};
#endif /* ENABLE_DECRYPTION */
}

#ifdef ENABLE_DECRYPTION
/**
 * Open the SRL if it isn't already opened.
 * This operation only works for DSi TAD packages.
 * @return 0 on success; non-zero on error.
 */
int WiiWADPrivate::openSRL(void)
{
	if (be16_to_cpu(tmdHeader.title_id.sysID) != NINTENDO_SYSID_TWL) {
		// Not a DSi TAD package.
		return -ENOENT;
	} else if (mainContent) {
		// Something's already loaded.
		if (mainContent->isOpen()) {
			// File is still open.
			return 0;
		}
		// File is no longer open.
		// unref() and reopen it.
		mainContent.reset();
	}

	if (!file || !file->isOpen()) {
		// Can't open the SRL.
		return -EIO;
	}

	assert(pIMETContent != nullptr);
	if (!pIMETContent) {
		return -EIO;
	}

	// If the CBCReader is closed, reopen it.
	if (!cbcReader) {
		// Data area IV:
		// - First two bytes are the big-endian content index.
		// - Remaining bytes are zero.
		uint8_t iv[16];
		memcpy(iv, &pIMETContent->index, sizeof(pIMETContent->index));
		memset(&iv[2], 0, sizeof(iv)-2);

		cbcReader = std::make_shared<CBCReader>(file,
			data_offset, data_size, dec_title_key, iv);
		if (!cbcReader->isOpen()) {
			// Unable to open a CBC reader.
			int ret = -cbcReader->lastError();
			if (ret == 0) {
				ret = -EIO;
			}
			cbcReader.reset();
			return ret;
		}
	}

	int ret = 0;
	PartitionFilePtr ptFile = std::make_shared<PartitionFile>(cbcReader,
		imetContentOffset, be64_to_cpu(pIMETContent->size));
	if (ptFile->isOpen()) {
		// Open the SRL.
		RomDataPtr srl = std::make_shared<NintendoDS>(ptFile);
		if (srl->isOpen()) {
			// Opened successfully.
			mainContent = std::move(srl);
		} else {
			// Unable to open the SRL.
			ret = -EIO;
		}
	} else {
		ret = -ptFile->lastError();
		if (ret != 0) {
			ret = -EIO;
		}
	}
	return ret;
}
#endif /* ENABLE_DECRYPTION */

/** WiiWAD **/

/**
 * Read a Nintendo Wii WAD file.
 *
 * A WAD file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the WAD file.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open disc image.
 */
WiiWAD::WiiWAD(const IRpFilePtr &file)
	: super(new WiiWADPrivate(file))
{
	// This class handles application packages.
	RP_D(WiiWAD);
	d->mimeType = "application/x-wii-wad";	// unofficial
	d->fileType = FileType::ApplicationPackage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the WAD header.
	d->file->rewind();
	size_t size = d->file->read(&d->wadHeader, sizeof(d->wadHeader));
	if (size != sizeof(d->wadHeader)) {
		d->file.reset();
		return;
	}

	// Check if this WAD file is supported.
	const DetectInfo info = {
		{0, sizeof(d->wadHeader), reinterpret_cast<const uint8_t*>(&d->wadHeader)},
		nullptr,	// ext (not needed for WiiWAD)
		d->file->size()	// szFile
	};
	d->wadType = static_cast<WiiWADPrivate::WadType>(isRomSupported_static(&info));
	if ((int)d->wadType < 0) {
		d->file.reset();
		return;
	}

	// Determine the addresses.
	uint32_t ticket_addr, tmd_addr;
	switch (d->wadType) {
		case WiiWADPrivate::WadType::WAD:
			// Standard WAD.
			// Sections are 64-byte aligned.
			ticket_addr = WiiWADPrivate::toNext64(be32_to_cpu(d->wadHeader.wad.header_size)) +
				      WiiWADPrivate::toNext64(be32_to_cpu(d->wadHeader.wad.cert_chain_size));
			tmd_addr = ticket_addr +
				      WiiWADPrivate::toNext64(be32_to_cpu(d->wadHeader.wad.ticket_size));

			// Data offset is after the TMD.
			// Data size is taken from the header.
			d->data_size = be32_to_cpu(d->wadHeader.wad.data_size);
			d->data_offset = tmd_addr +
				      WiiWADPrivate::toNext64(be32_to_cpu(d->wadHeader.wad.tmd_size));
			break;

		case WiiWADPrivate::WadType::BWF: {
			// BroadOn WAD Format.
			// Sections are NOT 64-byte aligned,
			// and there's an extra "name" section after the TMD.
			ticket_addr = be32_to_cpu(d->wadHeader.bwf.header_size) +
				      be32_to_cpu(d->wadHeader.bwf.cert_chain_size);
			tmd_addr = ticket_addr + be32_to_cpu(d->wadHeader.bwf.ticket_size);

			// Data offset is explicitly specified.
			// Data size is assumed to be the rest of the file.
			d->data_offset = be32_to_cpu(d->wadHeader.bwf.data_offset);
			d->data_size = (uint32_t)d->file->size() - d->data_offset;

			// Read the name here, since it's only present in early WADs.
			const uint32_t name_size = be32_to_cpu(d->wadHeader.bwf.name_size);
			if (name_size > 0 && name_size <= 1024) {
				const uint32_t name_addr = tmd_addr + be32_to_cpu(d->wadHeader.bwf.tmd_size);
				unique_ptr<char[]> namebuf(new char[name_size]);
				size = d->file->seekAndRead(name_addr, namebuf.get(), name_size);
				if (size == name_size) {
					// TODO: Trim NULLs?
					d->wadName.assign(namebuf.get(), name_size);
				}
			}
			break;
		}

		default:
			assert(!"Should not get here...");
			d->file.reset();
			d->wadType = WiiWADPrivate::WadType::Unknown;
			return;
	}

	// Verify that the data section is within range for the file.
	const off64_t data_end_offset = (off64_t)d->data_offset + (off64_t)d->data_size;
	if (data_end_offset > d->file->size()) {
		// Out of range.
		d->file.reset();
		d->wadType = WiiWADPrivate::WadType::Unknown;
		return;
	}

	// Read the ticket and TMD.
	// TODO: Verify ticket/TMD sizes.
	size = d->file->seekAndRead(ticket_addr, &d->ticket, sizeof(d->ticket));
	if (size != sizeof(d->ticket)) {
		// Seek and/or read error.
		d->file.reset();
		d->wadType = WiiWADPrivate::WadType::Unknown;
		return;
	}
	size = d->file->seekAndRead(tmd_addr, &d->tmdHeader, sizeof(d->tmdHeader));
	if (size != sizeof(d->tmdHeader)) {
		// Seek and/or read error.
		d->file.reset();
		d->wadType = WiiWADPrivate::WadType::Unknown;
		return;
	}

	// Read the TMD contents table.
	// FIXME: Is alignment needed?
	d->tmdContentsTbl.resize(be16_to_cpu(d->tmdHeader.nbr_cont));
	const size_t expCtTblSize = d->tmdContentsTbl.size() * sizeof(RVL_Content_Entry);
	size = d->file->read(d->tmdContentsTbl.data(), expCtTblSize);
	if (size == expCtTblSize) {
		// The first content has the IMET and/or WIBN,
		// or for DSiWare TADs, the SRL.
		if (!d->tmdContentsTbl.empty()) {
			const RVL_Content_Entry *const pEntry = &d->tmdContentsTbl[0];

			// Make sure it's in range.
			const off64_t content_end_offset =
				(off64_t)d->data_offset +
				be64_to_cpu(pEntry->size);
			if (content_end_offset <= data_end_offset) {
				// In range. It's valid!
				d->pIMETContent = pEntry;
				d->imetContentOffset = 0;
			}
		}
	} else {
		// Unable to read the content table.
		d->tmdContentsTbl.clear();
	}

	// Attempt to parse the ticket.
	MemFilePtr memFile = std::make_shared<MemFile>(
		reinterpret_cast<const uint8_t*>(&d->ticket), sizeof(d->ticket));
	if (!memFile->isOpen()) {
		// Failed to open a MemFile.
		d->file.reset();
		d->wadType = WiiWADPrivate::WadType::Unknown;
		return;
	}

	// NOTE: WiiTicket requires a ".tik" file extension.
	// TODO: Have WiiTicket use dynamic_cast<> to determine if this is a MemFile?
	memFile->setFilename("title.tik");

	WiiTicket *const wiiTicket = new WiiTicket(memFile);
	if (!wiiTicket->isValid()) {
		// Not a valid ticket?
		delete wiiTicket;
		d->file.reset();
		d->wadType = WiiWADPrivate::WadType::Unknown;
		return;
	}
	d->wiiTicket.reset(wiiTicket);

	// Get the key in use.
	d->key_idx = d->wiiTicket->encKey();

	// Main header is valid.
	d->isValid = true;

#ifdef ENABLE_DECRYPTION
	// Initialize the CBC reader for the main data area.

	// First, decrypt the title key.
	int ret = d->wiiTicket->decryptTitleKey(d->dec_title_key, sizeof(d->dec_title_key));
	d->key_status = d->wiiTicket->verifyResult();
	if (ret != 0) {
		// Failed to decrypt the title key.
		return;
	}

	if (!d->pIMETContent) {
		// No boot content...
		return;
	}

	// Data area IV:
	// - First two bytes are the big-endian content index.
	// - Remaining bytes are zero.
	uint8_t iv[16];
	memcpy(iv, &d->pIMETContent->index, sizeof(d->pIMETContent->index));
	memset(&iv[2], 0, sizeof(iv)-2);

	// Create a CBC reader to decrypt the data section.
	// TODO: Verify some known data?
	d->cbcReader = std::make_shared<CBCReader>(d->file,
		d->data_offset, d->data_size, d->dec_title_key, iv);

	if (d->tmdHeader.title_id.sysID != cpu_to_be16(3)) {
		// Wii: Contents may be one of the following:
		// - IMET header: Most common.
		// - WIBN header: DLC titles.
		size = d->cbcReader->seekAndRead(d->imetContentOffset, &d->imet, sizeof(d->imet));
		if (size == sizeof(d->imet) &&
		    d->imet.magic == cpu_to_be32(WII_IMET_MAGIC))
		{
			// This is an IMET header.
			// TODO: Do something here?
		} else if (size >= (offsetof(Wii_IMET_t, magic) + sizeof(d->imet.magic)) &&
			   d->imet.magic == cpu_to_be32(WII_WIBN_MAGIC))
		{
			// This is a WIBN header.
			// Create the PartitionFile and WiiWIBN subclass.
			// NOTE: Not sure how big the WIBN data is, so we'll
			// allow it to read the rest of the file.
			PartitionFilePtr ptFile = std::make_shared<PartitionFile>(d->cbcReader,
				offsetof(Wii_IMET_t, magic),
				be64_to_cpu(d->pIMETContent->size) - offsetof(Wii_IMET_t, magic));
			if (ptFile->isOpen()) {
				// Open the WiiWIBN.
				RomDataPtr wibn = std::make_shared<WiiWIBN>(ptFile);
				if (wibn->isOpen()) {
					// Opened successfully.
					d->mainContent = std::move(wibn);
				}
			}
		} else {
			// Sometimes the IMET header has a 64-byte offset.
			// FIXME: Figure out why.
			size = d->cbcReader->seekAndRead(d->imetContentOffset + 64, &d->imet, sizeof(d->imet));
			if (size == sizeof(d->imet) &&
			    d->imet.magic == cpu_to_be32(WII_IMET_MAGIC))
			{
				// This is an IMET header.
				// TODO: Do something here?
			}
		}
	} else {
		// Nintendo DSi: Main content is an SRL.
		d->openSRL();
	}
#else /* !ENABLE_DECRYPTION */
	// Cannot decrypt anything...
	d->key_status = KeyManager::VerifyResult::NoSupport;
#endif /* ENABLE_DECRYPTION */
}

/**
 * Close the opened file.
 */
void WiiWAD::close(void)
{
#ifdef ENABLE_DECRYPTION
	RP_D(WiiWAD);

	// Close any child RomData subclasses.
	if (d->mainContent) {
		d->mainContent->close();
	}

	// Close associated files used with child RomData subclasses.
	d->cbcReader.reset();
#endif /* ENABLE_DECRYPTION */

	// Call the superclass function.
	super::close();
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiWAD::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(Wii_WAD_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(WiiWADPrivate::WadType::Unknown);
	}

	// Check for the correct header fields.
	const Wii_WAD_Header *wadHeader = reinterpret_cast<const Wii_WAD_Header*>(info->header.pData);
	if (wadHeader->header_size != cpu_to_be32(sizeof(*wadHeader))) {
		// WAD header size is incorrect.
		return static_cast<int>(WiiWADPrivate::WadType::Unknown);
	}

	// Check WAD type.
	if (wadHeader->type != cpu_to_be32(WII_WAD_TYPE_Is) &&
	    wadHeader->type != cpu_to_be32(WII_WAD_TYPE_ib) &&
	    wadHeader->type != cpu_to_be32(WII_WAD_TYPE_Bk))
	{
		// WAD type is incorrect.
		// NOTE: This may be a BroadOn WAD.
		const Wii_BWF_Header *bwf = reinterpret_cast<const Wii_BWF_Header*>(wadHeader);
		if (bwf->ticket_size == cpu_to_be32(sizeof(RVL_Ticket))) {
			// Ticket size is correct.
			// This is probably in BroadOn WAD Format.
			return static_cast<int>(WiiWADPrivate::WadType::BWF);
		}

		// Not supported.
		return static_cast<int>(WiiWADPrivate::WadType::Unknown);
	}

	// Verify the ticket size.
	// TODO: Also the TMD size.
	if (be32_to_cpu(wadHeader->ticket_size) < sizeof(RVL_Ticket)) {
		// Ticket is too small.
		return static_cast<int>(WiiWADPrivate::WadType::Unknown);
	}
	
	// Check the file size to ensure we have at least the IMET section.
	const unsigned int expected_size =
		WiiWADPrivate::toNext64(be32_to_cpu(wadHeader->header_size)) +
		WiiWADPrivate::toNext64(be32_to_cpu(wadHeader->cert_chain_size)) +
		WiiWADPrivate::toNext64(be32_to_cpu(wadHeader->ticket_size)) +
		WiiWADPrivate::toNext64(be32_to_cpu(wadHeader->tmd_size));
	if (expected_size > info->szFile) {
		// File is too small.
		return static_cast<int>(WiiWADPrivate::WadType::Unknown);
	}

	// This appears to be a Wii WAD file.
	return static_cast<int>(WiiWADPrivate::WadType::WAD);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *WiiWAD::systemName(unsigned int type) const
{
	RP_D(const WiiWAD);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Wii has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"WiiWAD::systemName() array index optimization needs to be updated.");

	// TODO: Enum for Nintendo system IDs.
	type &= SYSNAME_TYPE_MASK;
	switch (be16_to_cpu(d->tmdHeader.title_id.sysID)) {
		default:
		case NINTENDO_SYSID_IOS:
		case NINTENDO_SYSID_RVL: {
			// Wii
			static const char *const sysNames_Wii[4] = {
				"Nintendo Wii", "Wii", "Wii", nullptr
			};
			return sysNames_Wii[type];
		}

		case NINTENDO_SYSID_TWL: {
			// DSi
			// TODO: iQue DSi for China?
			static const char *const sysNames_DSi[4] = {
				"Nintendo DSi", "DSi", "DSi", nullptr
			};
			return sysNames_DSi[type];
		}
	}

	// Should not get here...
	return nullptr;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t WiiWAD::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON | IMGBF_INT_BANNER |
	       IMGBF_EXT_COVER | IMGBF_EXT_COVER_3D |
	       IMGBF_EXT_COVER_FULL |
	       IMGBF_EXT_TITLE_SCREEN;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t WiiWAD::supportedImageTypes(void) const
{
	uint32_t ret;
#ifdef ENABLE_DECRYPTION
	RP_D(const WiiWAD);
	if (d->mainContent) {
		// TODO: Verify external types?
		ret = d->mainContent->supportedImageTypes() |
		      IMGBF_EXT_COVER | IMGBF_EXT_COVER_3D |
		      IMGBF_EXT_COVER_FULL |
		      IMGBF_EXT_TITLE_SCREEN;
	} else
#endif /* ENABLE_DECRYPTION */
	{
		ret = IMGBF_EXT_COVER | IMGBF_EXT_COVER_3D |
		      IMGBF_EXT_COVER_FULL |
		      IMGBF_EXT_TITLE_SCREEN;
	}
	return ret;
}

/**
 * Get a list of all available image sizes for the specified image type.
 *
 * The first item in the returned vector is the "default" size.
 * If the width/height is 0, then an image exists, but the size is unknown.
 *
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> WiiWAD::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	// NOTE: Can't check for DSiWare here.
	// TODO: Use WiiWIBN::supportedImageSizes_static() if decryption is enabled?
	switch (imageType) {
#ifdef ENABLE_DECRYPTION
		case IMG_INT_ICON:
			return {{nullptr, BANNER_WIBN_ICON_W, BANNER_WIBN_ICON_H, 0}};
		case IMG_INT_BANNER:
			return {{nullptr, BANNER_WIBN_IMAGE_W, BANNER_WIBN_IMAGE_H, 0}};
#else /* !ENABLE_DECRYPTION */
		case IMG_INT_ICON:
		case IMG_INT_BANNER:
			return WiiWIBN::supportedImageSizes_static(imageType);
#endif /* ENABLE_DECRYPTION */

		case IMG_EXT_COVER:
			return {{nullptr, 160, 224, 0}};
		case IMG_EXT_COVER_3D:
			return {{nullptr, 176, 248, 0}};
		case IMG_EXT_COVER_FULL:
			return {
				{nullptr, 512, 340, 0},
				{"HQ", 1024, 680, 1},
			};
		case IMG_EXT_TITLE_SCREEN:
			return {{nullptr, 192, 112, 0}};
		default:
			break;
	}

	// Unsupported image type.
	return {};
}

/**
 * Get a list of all available image sizes for the specified image type.
 *
 * The first item in the returned vector is the "default" size.
 * If the width/height is 0, then an image exists, but the size is unknown.
 *
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> WiiWAD::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);
	RP_D(const WiiWAD);

	if (d->tmdHeader.title_id.sysID != cpu_to_be16(3)) {
		// WiiWare
		// TODO: Use d->mainContent->supportedImageSizes() if decryption is enabled?
		switch (imageType) {
#ifdef ENABLE_DECRYPTION
			case IMG_INT_ICON:
				if (d->mainContent) {
					return {{nullptr, BANNER_WIBN_ICON_W, BANNER_WIBN_ICON_H, 0}};
				}
				break;
			case IMG_INT_BANNER:
				if (d->mainContent) {
					return {{nullptr, BANNER_WIBN_IMAGE_W, BANNER_WIBN_IMAGE_H, 0}};
				}
				break;
#else /* !ENABLE_DECRYPTION */
			case IMG_INT_ICON:
			case IMG_INT_BANNER:
				return WiiWIBN::supportedImageSizes_static(imageType);
#endif /* ENABLE_DECRYPTION */
			case IMG_EXT_COVER:
				return {{nullptr, 160, 224, 0}};
			case IMG_EXT_COVER_3D:
				return {{nullptr, 176, 248, 0}};
			case IMG_EXT_COVER_FULL:
				return {
					{nullptr, 512, 340, 0},
					{"HQ", 1024, 680, 1},
				};
			case IMG_EXT_TITLE_SCREEN:
				return {{nullptr, 192, 112, 0}};
			default:
				break;
		}
	} else {
		// DSiWare. Use the NintendoDS parser.
#ifdef ENABLE_DECRYPTION
		if (d->mainContent) {
			return d->mainContent->supportedImageSizes(imageType);
		} else
#endif /* ENABLE_DECRYPTION */
		{
			return NintendoDS::supportedImageSizes_static(imageType);
		}
	}

	// Unsupported image type.
	return {};
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
uint32_t WiiWAD::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

#ifdef ENABLE_DECRYPTION
	RP_D(const WiiWAD);
	if (d->mainContent) {
		// Get imgpf from the main content object.
		return d->mainContent->imgpf(imageType);
	}
#endif /* ENABLE_DECRYPTION */

	// No image processing flags here.
	return 0;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int WiiWAD::loadFieldData(void)
{
	RP_D(WiiWAD);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->wadType < 0) {
		// Unknown file type.
		return -EIO;
	}

	// WAD headers are read in the constructor.
	const RVL_TMD_Header *const tmdHeader = &d->tmdHeader;
	const uint16_t sys_id = be16_to_cpu(tmdHeader->title_id.sysID);
	d->fields.reserve(12);	// Maximum of 12 fields.
	d->fields.setTabName(0, (sys_id != NINTENDO_SYSID_TWL) ? "WAD" : "TAD");

	if (d->key_status != KeyManager::VerifyResult::OK) {
		// Unable to get the decryption key.
		const char *err = KeyManager::verifyResultToString(d->key_status);
		if (!err) {
			err = C_("RomData", "Unknown error. (THIS IS A BUG!)");
		}
		d->fields.addField_string(C_("WiiWAD", "Warning"),
			err, RomFields::STRF_WARNING);
	}

	// Type.
	string s_wadType;
	switch (d->wadType) {
		case WiiWADPrivate::WadType::WAD: {
			switch (be32_to_cpu(d->wadHeader.wad.type)) {
				case WII_WAD_TYPE_Is:
					s_wadType = "Installable";
					break;
				case WII_WAD_TYPE_ib:
					s_wadType = "Boot2";
					break;
				case WII_WAD_TYPE_Bk:
					s_wadType = "Backup";
					break;
				default: {
					char buf[4];
					memcpy(buf, &d->wadHeader.wad.type, sizeof(buf));
					buf[2] = '\0';
					buf[3] = '\0';
					s_wadType.assign(buf, 2);
					break;
				}
			}
			break;
		};

		case WiiWADPrivate::WadType::BWF:
			s_wadType = C_("WiiWAD", "BroadOn WAD Format");
			break;

		default:
			s_wadType = C_("RomData", "Unknown");
			break;
	}
	d->fields.addField_string(C_("RomData", "Type"), s_wadType);

	// Internal name. (BroadOn WADs only)
	// FIXME: This is the same "meta" section as Nintendo WADs...
	if (!d->wadName.empty()) {
		d->fields.addField_string(C_("RomData", "Name"), d->wadName);
	}

	// Title ID.
	// TODO: Make sure the ticket title ID matches the TMD title ID.
	d->fields.addField_string(C_("Nintendo", "Title ID"),
		rp_sprintf("%08X-%08X",
			be32_to_cpu(tmdHeader->title_id.hi),
			be32_to_cpu(tmdHeader->title_id.lo)));

	// Game ID.
	// NOTE: Only displayed if TID lo is all alphanumeric characters.
	// TODO: Only for certain TID hi?
	if (ISALNUM(tmdHeader->title_id.u8[4]) &&
	    ISALNUM(tmdHeader->title_id.u8[5]) &&
	    ISALNUM(tmdHeader->title_id.u8[6]) &&
	    ISALNUM(tmdHeader->title_id.u8[7]))
	{
		// Print the game ID.
		// TODO: Is the publisher code available anywhere?
		d->fields.addField_string(C_("RomData", "Game ID"),
			rp_sprintf("%.4s", reinterpret_cast<const char*>(&tmdHeader->title_id.u8[4])));
	}

	// Title version.
	const unsigned int title_version = be16_to_cpu(tmdHeader->title_version);
	d->fields.addField_string(C_("Nintendo", "Title Version"),
		rp_sprintf("%u.%u (v%u)", title_version >> 8, title_version & 0xFF, title_version));

	// Wii-specific
	unsigned int gcnRegion = ~0U;
	const char id4_region = (char)tmdHeader->title_id.u8[7];
	const uint32_t tid_hi = be32_to_cpu(tmdHeader->title_id.hi);
	if (sys_id <= NINTENDO_SYSID_RVL) {
		// Region code
		if (tid_hi == 0x00000001) {
			// IOS and/or System Menu.
			if (tmdHeader->title_id.lo == cpu_to_be32(0x00000002)) {
				// System Menu.
				const char *ver = WiiSystemMenuVersion::lookup(title_version);
				if (ver) {
					switch (ver[3]) {
						case 'J':
							gcnRegion = GCN_REGION_JPN;
							break;
						case 'U':
							gcnRegion = GCN_REGION_USA;
							break;
						case 'E':
							gcnRegion = GCN_REGION_EUR;
							break;
						case 'K':
							gcnRegion = GCN_REGION_KOR;
							break;
						case 'C':
							gcnRegion = GCN_REGION_CHN;
							break;
						case 'T':
							gcnRegion = GCN_REGION_TWN;
							break;
						default:
							gcnRegion = 255;
							break;
					}
				} else {
					gcnRegion = 255;
				}
			} else {
				// IOS, BC, or MIOS. No region.
				gcnRegion = GCN_REGION_ALL;
			}
		} else {
			gcnRegion = be16_to_cpu(tmdHeader->region_code);
		}

		bool isDefault;
		const char *const region =
			GameCubeRegions::gcnRegionToString(gcnRegion, id4_region, &isDefault);
		const char *const region_code_title = C_("RomData", "Region Code");
		if (region) {
			// Append the GCN region name (USA/JPN/EUR/KOR) if
			// the ID4 value differs.
			const char *suffix = nullptr;
			if (!isDefault) {
				suffix = GameCubeRegions::gcnRegionToAbbrevString(gcnRegion);
			}

			string s_region;
			if (suffix) {
				// tr: %1$s == full region name, %2$s == abbreviation
				s_region = rp_sprintf_p(C_("Wii", "%1$s (%2$s)"), region, suffix);
			} else {
				s_region = region;
			}

			d->fields.addField_string(region_code_title, s_region);
		} else {
			d->fields.addField_string(region_code_title,
				rp_sprintf(C_("RomData", "Unknown (0x%02X)"), gcnRegion));
		}

		// Required IOS version.
		if (sys_id <= NINTENDO_SYSID_RVL) {
			const char *const ios_version_title = C_("Wii", "IOS Version");
			const uint32_t ios_lo = be32_to_cpu(tmdHeader->sys_version.lo);
			if (tmdHeader->sys_version.hi == cpu_to_be32(0x00000001) &&
			    ios_lo > 2 && ios_lo < 0x300)
			{
				// Standard IOS slot.
				d->fields.addField_string(ios_version_title,
					rp_sprintf("IOS%u", ios_lo));
			} else if (tmdHeader->sys_version.id != 0) {
				// Non-standard IOS slot.
				// Print the full title ID.
				d->fields.addField_string(ios_version_title,
					rp_sprintf("%08X-%08X",
						be32_to_cpu(tmdHeader->sys_version.hi),
						be32_to_cpu(tmdHeader->sys_version.lo)));
			}
		}

		// Access rights.
		vector<string> *const v_access_rights_hdr = new vector<string>();
		v_access_rights_hdr->reserve(2);
		v_access_rights_hdr->emplace_back("AHBPROT");
		v_access_rights_hdr->emplace_back(C_("Wii", "DVD Video"));
		d->fields.addField_bitfield(C_("Wii", "Access Rights"),
			v_access_rights_hdr, 0, be32_to_cpu(tmdHeader->access_rights));

		if (sys_id == NINTENDO_SYSID_RVL) {
			// Get age rating(s).
			// TODO: Combine with GameCube::addFieldData()'s code.
			// Note that not all 16 fields are present on Wii,
			// though the fields do match exactly, so no
			// mapping is necessary.
			RomFields::age_ratings_t age_ratings;
			// Valid ratings: 0-1, 3-9
			static constexpr uint16_t valid_ratings = 0x3FB;

			for (int i = static_cast<int>(age_ratings.size())-1; i >= 0; i--) {
				if (!(valid_ratings & (1U << i))) {
					// Rating is not applicable for Wii.
					age_ratings[i] = 0;
					continue;
				}

				// Wii ratings field:
				// - 0x1F: Age rating.
				// - 0x20: Has online play if set.
				// - 0x80: Unused if set.
				const uint8_t rvl_rating = tmdHeader->ratings[i];
				if (rvl_rating & 0x80) {
					// Rating is unused.
					age_ratings[i] = 0;
					continue;
				}
				// Set active | age value.
				age_ratings[i] = RomFields::AGEBF_ACTIVE | (rvl_rating & 0x1F);

				// Is "rating may change during online play" set?
				if (rvl_rating & 0x20) {
					age_ratings[i] |= RomFields::AGEBF_ONLINE_PLAY;
				}
			}
			d->fields.addField_ageRatings(C_("RomData", "Age Ratings"), age_ratings);
		}
	}

	// Encryption key
	const char *const s_key_name = (d->wiiTicket) ? d->wiiTicket->encKeyName() : nullptr;
	if (s_key_name) {
		d->fields.addField_string(C_("RomData", "Encryption Key"), s_key_name);
	} else {
		d->fields.addField_string(C_("RomData", "Warning"),
			C_("RomData", "Could not determine the required encryption key."),
			RomFields::STRF_WARNING);
	}

	// Console ID.
	// TODO: Hide the "0x" prefix?
	d->fields.addField_string_numeric(C_("Nintendo", "Console ID"),
		be32_to_cpu(d->ticket.console_id), RomFields::Base::Hex, 8,
		RomFields::STRF_MONOSPACE);

#ifdef ENABLE_DECRYPTION
	// Do we have a main content object?
	// If so, we don't have IMET data.
	// TODO: Decrypt Wii content.bin for more stuff?
	if (d->mainContent) {
		// Add the main content data.
		const RomFields *const mainContentFields = d->mainContent->fields();
		assert(mainContentFields != nullptr);
		if (mainContentFields) {
			// For Wii, add the fields to the same tab.
			// For DSi, add the fields to new tabs.
			const int tabOffset = (sys_id == NINTENDO_SYSID_TWL ? RomFields::TabOffset_AddTabs : 0);
			d->fields.addFields_romFields(mainContentFields, tabOffset);
		}
	} else if (sys_id != NINTENDO_SYSID_TWL) {
		// No main content object.
		// Get the IMET data if it's available.
		RomFields::StringMultiMap_t *const pMap_bannerName = WiiCommon::getWiiBannerStrings(
			&d->imet, gcnRegion, id4_region);
		if (pMap_bannerName) {
			// Add the field.
			const uint32_t def_lc = NintendoLanguage::getWiiLanguageCode(
				NintendoLanguage::getWiiLanguage());
			d->fields.addField_string_multi(C_("WiiWAD", "Game Info"), pMap_bannerName, def_lc);
		}
	}
#endif /* ENABLE_DECRYPTION */

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int WiiWAD::loadMetaData(void)
{
	RP_D(WiiWAD);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->wadType < 0) {
		// Unknown file type.
		return -EIO;
	}

#ifdef ENABLE_DECRYPTION
	if (be16_to_cpu(d->tmdHeader.title_id.sysID) == NINTENDO_SYSID_TWL) {
		// DSi TAD package.
		// Get the metadata from the SRL if it's available.
		if (d->mainContent) {
			const RomMetaData *const srlMetaData = d->mainContent->metaData();
			if (srlMetaData && !srlMetaData->empty()) {
				// Create the metadata object.
				d->metaData = new RomMetaData();

				// Add the SRL metadata.
				return d->metaData->addMetaData_metaData(srlMetaData) + 1;
			}
		}
	}
#endif /* ENABLE_DECRYPTION */

	// TODO: Game title from WIBN if it's available.

	// NOTE: We can only get the title if the encryption key is valid.
	// If we can't get the title, don't bother creating RomMetaData.
	// TODO: Use WiiCommon for multi-language strings?
	string gameInfo = d->getGameInfo();
	if (gameInfo.empty()) {
		return -EIO;
	}
	const size_t nl_pos = gameInfo.find('\n');
	if (nl_pos != string::npos) {
		gameInfo.resize(nl_pos);
	}
	if (gameInfo.empty()) {
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// Title. (first line of game info)
	d->metaData->addMetaData_string(Property::Title, gameInfo);

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiWAD::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(WiiWAD);
	if (!d->isValid) {
		// Banner file isn't valid.
		pImage.reset();
		return -EIO;
	}

#ifdef ENABLE_DECRYPTION
	// Forward this call to the main content object.
	if (d->mainContent) {
		return d->mainContent->loadInternalImage(imageType, pImage);
	}
#endif /* ENABLE_DECRYPTION */

	// No main content object.
	pImage.reset();
	return -ENOENT;
}

/**
 * Get the animated icon data.
 *
 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
 * object has an animated icon.
 *
 * @return Animated icon data, or nullptr if no animated icon is present.
 */
IconAnimDataConstPtr WiiWAD::iconAnimData(void) const
{
#ifdef ENABLE_DECRYPTION
	// Forward this call to the main content object.
	RP_D(const WiiWAD);
	if (d->mainContent) {
		return d->mainContent->iconAnimData();
	}
#endif /* ENABLE_DECRYPTION */

	// No main content object.
	return nullptr;
}

/**
 * Get a list of URLs for an external image type.
 *
 * A thumbnail size may be requested from the shell.
 * If the subclass supports multiple sizes, it should
 * try to get the size that most closely matches the
 * requested size.
 *
 * @param imageType	[in]     Image type.
 * @param pExtURLs	[out]    Output vector.
 * @param size		[in,opt] Requested image size. This may be a requested
 *                               thumbnail size in pixels, or an ImageSizeType
 *                               enum value.
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiWAD::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	// Check if the main content is present.
	// If it is, and this is a Wii WAD, then this is a
	// DLC WAD, so the title ID won't match anything on
	// GameTDB.
	RP_D(const WiiWAD);
	if (!d->isValid || (int)d->wadType < 0) {
		// WAD isn't valid.
		return -EIO;
	}

	// TMD Header
	const RVL_TMD_Header *const tmdHeader = &d->tmdHeader;

	const uint16_t sys_id = be16_to_cpu(tmdHeader->title_id.sysID);
	if (sys_id != NINTENDO_SYSID_TWL) {
#ifdef ENABLE_DECRYPTION
		if (d->mainContent) {
			// Main content is present.
			// The boxart is not available on GameTDB, since it's a DLC WAD.
			return -ENOENT;
		} else
#endif /* ENABLE_DECRYPTION */
		{
			// If the first letter of the ID4 is lowercase,
			// that means it's a DLC title. GameTDB doesn't
			// have artwork for DLC titles.
			const char firstID4 = be32_to_cpu(tmdHeader->title_id.lo) >> 24;
			if (ISLOWER(firstID4)) {
				// It's lowercase.
				return -ENOENT;
			}
		}
	}

	// Check for a valid TID hi.
	const char *sysDir;
	switch (sys_id) {
		case NINTENDO_SYSID_RVL:	// Wii
			// Check for a valid LOWORD.
			switch (be16_to_cpu(tmdHeader->title_id.catID)) {
				case NINTENDO_CATID_RVL_DISC:
				case NINTENDO_CATID_RVL_DOWNLOADED:
				case NINTENDO_CATID_RVL_SYSTEM:
				case NINTENDO_CATID_RVL_DISC_WITH_CHANNEL:
				case NINTENDO_CATID_RVL_DLC:
				case NINTENDO_CATID_RVL_HIDDEN:
					// TID hi is valid.
					break;
				default:
					// No GameTDB artwork is available.
					return -ENOENT;
			}
			sysDir = "wii";
			break;
		case NINTENDO_SYSID_TWL:
			// TODO: DSiWare on GameTDB.
			//sysDir = "ds";
			return -ENOENT;
		default:
			// Unsupported system ID.
			return -ENOENT;
	}

	// Get the image sizes and sort them based on the
	// requested image size.
	vector<ImageSizeDef> sizeDefs = supportedImageSizes(imageType);
	if (sizeDefs.empty()) {
		// No image sizes.
		return -ENOENT;
	}

	// Select the best size.
	const ImageSizeDef *const sizeDef = d->selectBestSize(sizeDefs, size);
	if (!sizeDef) {
		// No size available...
		return -ENOENT;
	}

	// NOTE: Only downloading the first size as per the
	// sort order, since GameTDB basically guarantees that
	// all supported sizes for an image type are available.
	// TODO: Add cache keys for other sizes in case they're
	// downloaded and none of these are available?

	// Determine the image type name.
	// TODO: Extend for DSiWare.
	const char *imageTypeName_base;
	const char *ext;
	switch (imageType) {
		case IMG_EXT_COVER:
			imageTypeName_base = "cover";
			ext = ".png";
			break;
		case IMG_EXT_COVER_3D:
			imageTypeName_base = "cover3D";
			ext = ".png";
			break;
		case IMG_EXT_COVER_FULL:
			imageTypeName_base = "coverfull";
			ext = ".png";
			break;
		case IMG_EXT_TITLE_SCREEN:
			imageTypeName_base = "wwtitle";
			ext = ".png";
			break;
		default:
			// Unsupported image type.
			return -ENOENT;
	}

	// Game ID. (GameTDB uses ID4 for WiiWare.)
	// The ID4 cannot have non-printable characters.
	// NOTE: Must be NULL-terminated.
	char id4[5];
	memcpy(id4, &tmdHeader->title_id.u8[4], 4);
	id4[4] = 0;
	for (int i = 4-1; i >= 0; i--) {
		if (!ISPRINT(id4[i])) {
			// Non-printable character found.
			return -ENOENT;
		}
	}

	// Determine the GameTDB language code(s).
	const unsigned int gcnRegion = be16_to_cpu(tmdHeader->region_code);
	const char id4_region = (char)tmdHeader->title_id.u8[7];
	const vector<uint16_t> tdb_lc = GameCubeRegions::gcnRegionToGameTDB(gcnRegion, id4_region);

	// If we're downloading a "high-resolution" image (M or higher),
	// also add the default image to ExtURLs in case the user has
	// high-resolution image downloads disabled.
	const ImageSizeDef *szdefs_dl[2];
	szdefs_dl[0] = sizeDef;
	unsigned int szdef_count;
	if (sizeDef->index >= 2) {
		// M or higher.
		szdefs_dl[1] = &sizeDefs[0];
		szdef_count = 2;
	} else {
		// Default or S.
		szdef_count = 1;
	}

	// Add the URLs.
	pExtURLs->resize(szdef_count * tdb_lc.size());
	auto extURL_iter = pExtURLs->begin();
	const auto tdb_lc_cend = tdb_lc.cend();
	for (unsigned int i = 0; i < szdef_count; i++) {
		// Current image type.
		char imageTypeName[16];
		snprintf(imageTypeName, sizeof(imageTypeName), "%s%s",
			 imageTypeName_base, (szdefs_dl[i]->name ? szdefs_dl[i]->name : ""));

		// Add the images.
		for (auto tdb_iter = tdb_lc.cbegin();
		     tdb_iter != tdb_lc_cend; ++tdb_iter, ++extURL_iter)
		{
			const string lc_str = SystemRegion::lcToStringUpper(*tdb_iter);
			extURL_iter->url = d->getURL_GameTDB(sysDir, imageTypeName, lc_str.c_str(), id4, ext);
			extURL_iter->cache_key = d->getCacheKey_GameTDB(sysDir, imageTypeName, lc_str.c_str(), id4, ext);
			extURL_iter->width = szdefs_dl[i]->width;
			extURL_iter->height = szdefs_dl[i]->height;
			extURL_iter->high_res = (szdefs_dl[i]->index >= 2);
		}
	}

	// All URLs added.
	return 0;
}

/**
 * Check for "viewed" achievements.
 *
 * @return Number of achievements unlocked.
 */
int WiiWAD::checkViewedAchievements(void) const
{
	RP_D(const WiiWAD);
	if (!d->isValid) {
		// WAD is not valid.
		return 0;
	}

	Achievements *const pAch = Achievements::instance();
	int ret = 0;

	if (d->key_idx == WiiTicket::EncryptionKeys::Key_RVT_Debug) {
		// Debug encryption.
		pAch->unlock(Achievements::ID::ViewedDebugCryptedFile);
		ret++;
	}

	if (d->wadType == WiiWADPrivate::WadType::BWF) {
		// BroadOn WAD format.
		pAch->unlock(Achievements::ID::ViewedBroadOnWADFile);
		ret++;
	}

	return ret;
}

}
