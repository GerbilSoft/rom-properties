/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PSP.hpp: PlayStation Portable disc image reader.                        *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "config.libromdata.h"
#include "PSP.hpp"

// Other rom-properties libraries
#include "librpbase/img/RpPng.hpp"
using namespace LibRpBase;
using namespace LibRpText;
using namespace LibRpTexture;
using LibRpFile::IRpFile;

// DiscReader
#include "cdrom_structs.h"
#include "iso_structs.h"
#include "disc/IsoPartition.hpp"
#include "disc/CisoPspReader.hpp"
#include "disc/PartitionFile.hpp"

// Other RomData subclasses
#include "Other/ISO.hpp"
#include "Other/ELF.hpp"

// C++ STL classes.
using std::string;
using std::vector;

namespace LibRomData {

class PSPPrivate final : public LibRpBase::RomDataPrivate
{
	public:
		PSPPrivate(PSP *q, IRpFile *file);
		~PSPPrivate() final;

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(PSPPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		enum class DiscType {
			Unknown	= -1,

			PspGame		= 0,	// PSP game
			UmdVideo	= 1,	// UMD video

			Max
		};
		DiscType discType;

		ISO_Primary_Volume_Descriptor pvd;

		// IsoPartition
		IDiscReader *discReader;
		IsoPartition *isoPartition;

		// Icon.
		rp_image *img_icon;

		/**
		 * Load the icon.
		 * @return Icon, or nullptr on error.
		 */
		const rp_image *loadIcon(void);

		// Boot executable (EBOOT.BIN)
		RomData *bootExeData;

		/**
		 * Open the boot executable.
		 * @return RomData* on success; nullptr on error.
		 */
		RomData *openBootExe(void);
};

ROMDATA_IMPL(PSP)
ROMDATA_IMPL_IMG_TYPES(PSP)

/** PSPPrivate **/

/* RomDataInfo */
const char *const PSPPrivate::exts[] = {
	".iso",			// ISO
	".img",			// USER_L0.IMG on PSP dev DVD-Rs
	".dax",			// DAX
	".ciso", ".cso",	// CISO

#ifdef HAVE_LZ4
	".ziso", ".zso",		// ZISO
#endif /* HAVE_LZ4 */

	".jiso", ".jso",	// JISO (TODO)

	nullptr
};
const char *const PSPPrivate::mimeTypes[] = {
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-cd-image",
	"application/x-iso9660-image",

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-psp-ciso-image",
	"application/x-cso",		// technically a different format...
	"application/x-compressed-iso",	// KDE detects CISO as this
	"application/x-psp-dax-image",
	"application/x-psp-jiso-image",
	"application/x-psp-ziso-image",

	// TODO: PSP?
	nullptr
};
const RomDataInfo PSPPrivate::romDataInfo = {
	"PSP", exts, mimeTypes
};

PSPPrivate::PSPPrivate(PSP *q, IRpFile *file)
	: super(q, file, &romDataInfo)
	, discType(DiscType::Unknown)
	, discReader(nullptr)
	, isoPartition(nullptr)
	, img_icon(nullptr)
	, bootExeData(nullptr)
{
	// Clear the structs.
	memset(&pvd, 0, sizeof(pvd));
}

PSPPrivate::~PSPPrivate()
{
	UNREF(bootExeData);
	UNREF(isoPartition);
	UNREF(discReader);

	UNREF(img_icon);
}

/**
 * Load the icon.
 * @return Icon, or nullptr on error.
 */
const rp_image *PSPPrivate::loadIcon(void)
{
	if (img_icon) {
		// Icon has already been loaded.
		return img_icon;
	} else if (!this->isValid || !this->isoPartition) {
		// Can't load the icon.
		return nullptr;
	}

	// Icon is located on disc as a regular PNG image.
	const char *const icon_filename =
		(unlikely(discType == DiscType::UmdVideo)
			? "/UMD_VIDEO/ICON0.PNG"
			: "/PSP_GAME/ICON0.PNG");
	IRpFile *const f_icon = isoPartition->open(icon_filename);
	if (!f_icon || !f_icon->isOpen()) {
		// Unable to open the icon file.
		UNREF(f_icon);
		return nullptr;
	}

	// Decode the image.
	// TODO: For rpcli, shortcut to extract the PNG directly.
	this->img_icon = RpPng::load(f_icon);
	f_icon->unref();
	return this->img_icon;
}

/**
 * Open the boot executable.
 * @return RomData* on success; nullptr on error.
 */
RomData *PSPPrivate::openBootExe(void)
{
	if (bootExeData) {
		// The boot executable is already open.
		return bootExeData;
	}

	if (!isoPartition || !isoPartition->isOpen()) {
		// ISO partition is not open.
		return nullptr;
	}

	// Open the boot file.
	// FIXME: This is normally encrypted, but some games have
	// an unencrypted EBOOT.BIN.
	IRpFile *f_bootExe = isoPartition->open("/PSP_GAME/SYSDIR/EBOOT.BIN");
	if (f_bootExe) {
		RomData *const exeData = new ELF(f_bootExe);
		f_bootExe->unref();
		if (exeData->isOpen() && exeData->isValid()) {
			// Boot executable is open and valid.
			bootExeData = exeData;
			return exeData;
		}

		// Unable to open the executable.
		UNREF(exeData);
	}

	// Unable to open the default executable.
	return nullptr;
}

/** PSP **/

/**
 * Read a Sony PlayStation Portable disc image.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
PSP::PSP(IRpFile *file)
	: super(new PSPPrivate(this, file))
{
	// This class handles disc images.
	RP_D(PSP);
	d->mimeType = "application/x-cd-image";	// unofficial
	d->fileType = FileType::DiscImage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// UMD is based on the DVD specification and therefore only has 2048-byte sectors.
	IDiscReader *discReader = nullptr;

	// Check if this is a supported compressed disc image.
	uint8_t header[256];
	d->file->rewind();
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header)) {
		// Read error.
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}
	if (CisoPspReader::isDiscSupported_static(header, sizeof(header)) >= 0) {
		discReader = new CisoPspReader(d->file);
		if (!discReader->isOpen()) {
			// Not CISO.
			UNREF_AND_NULL_NOCHK(discReader);
		}
	}

	if (!discReader) {
		// Not a supported compressed disc image.
		// Try opening as uncompressed.
		discReader = new DiscReader(d->file);
	}

	if (!discReader->isOpen()) {
		// Error opening the DiscReader.
		UNREF(discReader);
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check the ISO PVD and system ID.
	size = discReader->seekAndRead(ISO_PVD_ADDRESS_2048, &d->pvd, sizeof(d->pvd));
	if (size != sizeof(d->pvd)) {
		UNREF(discReader);
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}
	if (ISO::checkPVD(reinterpret_cast<const uint8_t*>(&d->pvd)) < 0) {
		// Not ISO-9660.
		UNREF(discReader);
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Verify the system ID.
	d->discType = static_cast<PSPPrivate::DiscType>(isRomSupported_static(&d->pvd));
	if ((int)d->discType < 0) {
		// Incorrect system ID.
		UNREF(discReader);
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Try to open the ISO partition.
	IsoPartition *const isoPartition = new IsoPartition(discReader, 0, 0);
	if (!isoPartition->isOpen()) {
		// Error opening the ISO partition.
		UNREF(isoPartition);
		UNREF(discReader);
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Disc image is ready.
	d->discReader = discReader;
	d->isoPartition = isoPartition;
	d->isValid = true;
}

/**
 * Close the opened file.
 */
void PSP::close(void)
{
	RP_D(PSP);

	// NOTE: Don't delete these. They have rp_image objects
	// that may be used by the UI later.
	if (d->bootExeData) {
		d->bootExeData->close();
	}

	UNREF_AND_NULL(d->isoPartition);
	UNREF_AND_NULL(d->discReader);

	// Call the superclass function.
	super::close();
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int PSP::isRomSupported_static(const DetectInfo *info)
{
	// NOTE: This version is only supported for compressed disc images.
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 256)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return (int)PSPPrivate::DiscType::Unknown;
	}

	// Check if it's supported by the CISO reader.
	if (CisoPspReader::isDiscSupported_static(
		info->header.pData, info->header.size) >= 0)
	{
		// Supported by CISO.
		// NOTE: The constructor will determine the actual disc type.
		return (int)PSPPrivate::DiscType::PspGame;
	}

	// Not a supported compressed disc image.
	return (int)PSPPrivate::DiscType::Unknown;
}

/**
 * Is a ROM image supported by this class?
 * @param pvd ISO-9660 Primary Volume Descriptor.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int PSP::isRomSupported_static(
	const ISO_Primary_Volume_Descriptor *pvd)
{
	PSPPrivate::DiscType discType = PSPPrivate::DiscType::Unknown;

	assert(pvd != nullptr);
	if (!pvd) {
		// Bad.
		return (int)discType;
	}

	// PlayStation Portable game discs have the system ID "PSP GAME".
	// UMD video discs have the system ID "UMD VIDEO".
	int pos = -1;
	if (!strncmp(pvd->sysID, "PSP GAME ", 9)) {
		discType = PSPPrivate::DiscType::PspGame;
		pos = 9;
	} else if (!strncmp(pvd->sysID, "UMD VIDEO ", 10)) {
		discType = PSPPrivate::DiscType::UmdVideo;
		pos = 10;
	}

	if (pos < 0) {
		// Not valid.
		return (int)PSPPrivate::DiscType::Unknown;
	}

	// Make sure the rest of the system ID is either spaces or NULLs.
	const char *p = &pvd->sysID[pos];
	const char *const p_end = &pvd->sysID[sizeof(pvd->sysID)];
	bool isOK = true;
	for (; p < p_end; p++) {
		if (*p != ' ' && *p != '\0') {
			isOK = false;
			break;
		}
	}

	if (isOK) {
		// Valid PVD.
		return (int)discType;
	}

	// Not a PlayStation Portable disc.
	return (int)PSPPrivate::DiscType::Unknown;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *PSP::systemName(unsigned int type) const
{
	RP_D(const PSP);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// PSP has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"PSP::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[2][4] = {
		{"Sony PlayStation Portable", "PlayStation Portable", "PSP", nullptr},
		{"Universal Media Disc", "Universal Media Disc", "UMD", nullptr},
	};
	return sysNames[(unsigned int)(d->discType) & 1][type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t PSP::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> PSP::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	RP_D(const PSP);
	if (!d->isValid || imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported.
		return vector<ImageSizeDef>();
	}

	// TODO: Actually check the icon size.
	// Assuming 144x80 for now.
	static const ImageSizeDef sz_INT_ICON[] = {
		{nullptr, 144, 80, 0},
	};
	return vector<ImageSizeDef>(sz_INT_ICON,
		sz_INT_ICON + ARRAY_SIZE(sz_INT_ICON));
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
vector<RomData::ImageSizeDef> PSP::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_ICON) {
		// Only icons are supported.
		return vector<ImageSizeDef>();
	}

	// NOTE: Assuming the icon is 144x80.
	static const ImageSizeDef sz_INT_ICON[] = {
		{nullptr, 144, 80, 0},
	};
	return vector<ImageSizeDef>(sz_INT_ICON,
		sz_INT_ICON + ARRAY_SIZE(sz_INT_ICON));
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int PSP::loadFieldData(void)
{
	RP_D(PSP);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->discType < 0) {
		// Unknown disc type.
		return -EIO;
	}

	d->fields->reserve(6);	// Maximum of 6 fields.
	d->fields->setTabName(0, (unlikely(d->discType == PSPPrivate::DiscType::UmdVideo) ? "UMD" : "PSP"));

	// Show UMD_DATA.BIN fields.
	// FIXME: Figure out what the fields are.
	// - '|'-terminated fields.
	// - Field 0: Game ID
	// - Field 1: Encryption key?
	// - Field 2: Revision?
	// - Field 3: Age rating?
	IRpFile *const umdDataBin = d->isoPartition->open("/UMD_DATA.BIN");
	if (umdDataBin->isOpen()) {
		// Read up to 128 bytes.
		char buf[129];
		size_t size = umdDataBin->read(buf, sizeof(buf)-1);
		buf[size] = 0;

		// Find the first '|'.
		const char *p = static_cast<const char*>(memchr(buf, '|', sizeof(buf)));
		if (p) {
			// Game ID field on UMD Video discs is the video title.
			const char *const gameID_title =
				(unlikely(d->discType == PSPPrivate::DiscType::UmdVideo)
					? C_("RomData", "Video Title")
					: C_("RomData", "Game ID"));
			d->fields->addField_string(gameID_title,
				latin1_to_utf8(buf, static_cast<int>(p - buf)));
		}
	}
	UNREF(umdDataBin);

	// TODO: Add fields from PARAM.SFO.

	// Show a tab for the boot file.
	RomData *const bootExeData = d->openBootExe();
	if (bootExeData) {
		// Add the fields.
		// NOTE: Adding tabs manually so we can show the disc info in
		// the primary tab.
		// TODO: Move to an "EBOOT" tab once PARAM.SFO is added.
		const RomFields *const exeFields = bootExeData->fields();
		if (exeFields) {
			int exeTabCount = exeFields->tabCount();
			for (int i = 1; i < exeTabCount; i++) {
				d->fields->setTabName(i, exeFields->tabName(i));
			}
			d->fields->setTabIndex(0);
			d->fields->addFields_romFields(exeFields, 0);
			d->fields->setTabIndex(exeTabCount - 1);
		}
	}

	// TODO: Parse firmware update PARAM.SFO and EBOOT.BIN?

	// ISO object for ISO-9660 PVD
	// TODO: DiscReader overload for ISO.
	PartitionFile *const ptFile = new PartitionFile(d->discReader, 0, d->discReader->size());
	ISO *const isoData = new ISO(ptFile);
	ptFile->unref();
	if (isoData->isOpen()) {
		// Add the fields.
		const RomFields *const isoFields = isoData->fields();
		if (isoFields) {
			d->fields->addFields_romFields(isoFields,
				RomFields::TabOffset_AddTabs);
		}
	}
	isoData->unref();

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
int PSP::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(PSP);
	ROMDATA_loadInternalImage_single(
		IMG_INT_ICON,	// ourImageType
		d->file,	// file
		d->isValid,	// isValid
		d->discType,	// romType
		d->img_icon,	// imgCache
		d->loadIcon);	// func
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int PSP::loadMetaData(void)
{
	RP_D(PSP);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->isValid || (int)d->discType < 0) {
		// Unknown disc image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(3);	// Maximum of 3 metadata properties.

	// Add the PVD metadata.
	ISO::addMetaData_PVD(d->metaData, &d->pvd);

	// Add the disc ID and/or title from UMD_DATA.BIN.
	// The PVD title is useless in most cases.
	// TODO: Split this into a separate function?
	// Show UMD_DATA.BIN fields.
	// FIXME: Figure out what the fields are.
	// - '|'-terminated fields.
	// - Field 0: Game ID
	// - Field 1: Encryption key?
	// - Field 2: Revision?
	// - Field 3: Age rating?
	IRpFile *const umdDataBin = d->isoPartition->open("/UMD_DATA.BIN");
	if (umdDataBin->isOpen()) {
		// Read up to 128 bytes.
		char buf[129];
		size_t size = umdDataBin->read(buf, sizeof(buf)-1);
		buf[size] = 0;

		// Find the first '|'.
		const char *p = static_cast<const char*>(memchr(buf, '|', sizeof(buf)));
		if (p) {
			// Game ID field on UMD Video discs is the video title.
			d->metaData->addMetaData_string(Property::Title,
				latin1_to_utf8(buf, static_cast<int>(p - buf)));
		}
	}
	UNREF(umdDataBin);

	// TODO: More PSP-specific metadata?

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

}
