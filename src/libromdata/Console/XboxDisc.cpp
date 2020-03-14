/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxDisc.cpp: Microsoft Xbox disc image parser.                         *
 *                                                                         *
 * Copyright (c) 2019-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XboxDisc.hpp"

// librpbase, librpbase, librptexture
#include "librpfile/RpFile.hpp"
using namespace LibRpBase;
using LibRpTexture::rp_image;

// XDVDFSPartition
#include "../iso_structs.h"
#include "../disc/xdvdfs_structs.h"
#include "../disc/XDVDFSPartition.hpp"

// Other RomData subclasses
#include "Other/ISO.hpp"
#include "Xbox_XBE.hpp"
#include "Xbox360_XEX.hpp"

// C++ STL classes.
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(XboxDisc)

class XboxDiscPrivate : public LibRpBase::RomDataPrivate
{
	public:
		XboxDiscPrivate(XboxDisc *q, LibRpBase::IRpFile *file);
		virtual ~XboxDiscPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(XboxDiscPrivate)

	public:
		// Disc type.
		enum DiscType {
			DISC_UNKNOWN = -1,	// Unknown disc type.

			DISC_TYPE_EXTRACTED	= 0,	// Extracted XDVDFS
			DISC_TYPE_XGD1		= 1,	// XGD1 (Original Xbox)
			DISC_TYPE_XGD2		= 2,	// XGD2 (Xbox 360)
			DISC_TYPE_XGD3		= 3,	// XGD3 (Xbox 360)

			DISC_TYPE_MAX
		};
		int discType;
		uint8_t wave;

		// XDVDFS starting address.
		off64_t xdvdfs_addr;

		// XDVDFSPartition
		DiscReader *discReader;
		XDVDFSPartition *xdvdfsPartition;

		// default.xbe / default.xex
		RomData *defaultExeData;

		enum ExeType {
			EXE_TYPE_UNKNOWN	= -1,

			EXE_TYPE_XBE		= 0,	// Xbox XBE
			EXE_TYPE_XEX		= 1,	// Xbox 360 XEX

			EXE_TYPE_MAX
		};
		int exeType;

		/**
		 * Open default.xbe / default.xex.
		 * @param pExeType	[out] EXE type.
		 * @return RomData* on success; nullptr on error.
		 */
		RomData *openDefaultExe(int *pExeType = nullptr);

		enum ConsoleType {
			CONSOLE_TYPE_UNKNOWN	= -1,

			CONSOLE_TYPE_XBOX	= 0,	// Xbox
			CONSOLE_TYPE_XBOX_360	= 1,	// Xbox 360

			CONSOLE_TYPE_MAX
		};

		/**
		 * Get the console type.
		 *
		 * This is based on the EXE type, or disc type
		 * if the EXE cannot be loaded for some reason.
		 *
		 * @return Console type.
		 */
		ConsoleType getConsoleType(void) const;

		// Are we using a Kreon drive?
		bool isKreon;

		/**
		 * Unlock the Kreon drive.
		 */
		inline void unlockKreonDrive(void);

		/**
		 * Lock the Kreon drive.
		 */
		inline void lockKreonDrive(void);
};

/** XboxDiscPrivate **/

XboxDiscPrivate::XboxDiscPrivate(XboxDisc *q, IRpFile *file)
	: super(q, file)
	, discType(DISC_UNKNOWN)
	, wave(0)
	, xdvdfs_addr(0)
	, discReader(nullptr)
	, xdvdfsPartition(nullptr)
	, defaultExeData(nullptr)
	, exeType(EXE_TYPE_UNKNOWN)
	, isKreon(false)
{
}

XboxDiscPrivate::~XboxDiscPrivate()
{
	if (isKreon) {
		lockKreonDrive();
	}

	if (defaultExeData) {
		defaultExeData->unref();
	}
	delete xdvdfsPartition;
	delete discReader;
}

/**
 * Open default.xbe / default.xex.
 * @param pExeType	[out] EXE type.
 * @return RomData* on success; nullptr on error.
 */
RomData *XboxDiscPrivate::openDefaultExe(int *pExeType)
{
	if (defaultExeData) {
		// default.xbe / default.xex is already open.
		if (pExeType) {
			*pExeType = exeType;
		}
		return defaultExeData;
	}

	if (!xdvdfsPartition || !xdvdfsPartition->isOpen()) {
		// XDVDFS partition is not open.
		return nullptr;
	}

	// Try to open default.xex.
	IRpFile *f_defaultExe = xdvdfsPartition->open("/default.xex");
	if (f_defaultExe) {
		RomData *const xexData = new Xbox360_XEX(f_defaultExe);
		f_defaultExe->unref();
		if (xexData->isValid()) {
			// default.xex is open and valid.
			defaultExeData = xexData;
			exeType = EXE_TYPE_XEX;
			if (pExeType) {
				*pExeType = EXE_TYPE_XEX;
			}
			return xexData;
		}
	}

	// Try to open default.xbe.
	// TODO: What about discs that have both?
	f_defaultExe = xdvdfsPartition->open("/default.xbe");
	if (f_defaultExe) {
		RomData *const xbeData = new Xbox_XBE(f_defaultExe);
		f_defaultExe->unref();
		if (xbeData->isValid()) {
			// default.xex is open and valid.
			defaultExeData = xbeData;
			exeType = EXE_TYPE_XBE;
			if (pExeType) {
				*pExeType = EXE_TYPE_XBE;
			}
			return xbeData;
		}
		f_defaultExe->unref();
		exeType = EXE_TYPE_XBE;
		if (pExeType) {
			*pExeType = EXE_TYPE_XBE;
		}
	}

	// Unable to open the default executable.
	return nullptr;
}

/**
 * Get the console type.
 *
 * This is based on the EXE type, or disc type
 * if the EXE cannot be loaded for some reason.
 *
 * @return Console type.
 */
XboxDiscPrivate::ConsoleType XboxDiscPrivate::getConsoleType(void) const
{
	// Check for the default executable.
	int exeType;
	RomData *const defaultExeData = const_cast<XboxDiscPrivate*>(this)->openDefaultExe(&exeType);
	if (defaultExeData) {
		// Default executable loaded.
		switch (exeType) {
			case EXE_TYPE_XBE:
				return CONSOLE_TYPE_XBOX;
			case EXE_TYPE_XEX:
				return CONSOLE_TYPE_XBOX_360;
			default:
				break;
		}
	}

	// Unable to load the EXE.
	// Use the disc type.
	if (discType >= XboxDiscPrivate::DISC_TYPE_XGD2) {
		return CONSOLE_TYPE_XBOX_360;
	}

	// Assume Xbox for XGD1 and extracted XDVDFS.
	return CONSOLE_TYPE_XBOX;
}

/**
 * Unlock the Kreon drive.
 */
inline void XboxDiscPrivate::unlockKreonDrive(void)
{
	if (!isKreon)
		return;

	RpFile *const rpFile = dynamic_cast<RpFile*>(this->file);
	if (rpFile) {
		rpFile->setKreonErrorSkipState(true);
		rpFile->setKreonLockState(RpFile::KREON_STATE_2_WXRIPPER);
	}
}

/**
 * Lock the Kreon drive.
 */
inline void XboxDiscPrivate::lockKreonDrive(void)
{
	if (!isKreon)
		return;

	RpFile *const rpFile = dynamic_cast<RpFile*>(this->file);
	if (rpFile) {
		rpFile->setKreonErrorSkipState(false);
		rpFile->setKreonLockState(RpFile::KREON_STATE_LOCKED);
	}
}

/** XboxDisc **/

/**
 * Read a Microsoft Xbox disc image.
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
XboxDisc::XboxDisc(IRpFile *file)
	: super(new XboxDiscPrivate(this, file))
{
	// This class handles disc images.
	RP_D(XboxDisc);
	d->className = "XboxDisc";
	d->mimeType = "application/x-cd-image";	// unofficial
	d->fileType = FTYPE_DISC_IMAGE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// TODO: Also check for trimmed XDVDFS. (offset == 0)

	// Read the ISO-9660 PVD.
	// NOTE: Only 2048-byte sectors, since this is DVD.
	ISO_Primary_Volume_Descriptor pvd;
	size_t size = d->file->seekAndRead(ISO_PVD_ADDRESS_2048, &pvd, sizeof(pvd));
	if (size != sizeof(pvd)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this disc image is supported.
	d->discType = isRomSupported_static(&pvd, &d->wave);

	switch (d->discType) {
		case XboxDiscPrivate::DISC_TYPE_XGD1:
			d->xdvdfs_addr = XDVDFS_LBA_OFFSET_XGD1 * XDVDFS_BLOCK_SIZE;
			break;
		case XboxDiscPrivate::DISC_TYPE_XGD2:
			d->xdvdfs_addr = XDVDFS_LBA_OFFSET_XGD2 * XDVDFS_BLOCK_SIZE;
			break;
		case XboxDiscPrivate::DISC_TYPE_XGD3:
			d->xdvdfs_addr = XDVDFS_LBA_OFFSET_XGD3 * XDVDFS_BLOCK_SIZE;
			break;
		default:
			// This might be an extracted XDVDFS.
			d->xdvdfs_addr = 0;
			break;
	}

	// If this is a Kreon drive, unlock it.
	if (d->file->isDevice()) {
		RpFile *const rpFile = dynamic_cast<RpFile*>(d->file);
		if (rpFile) {
			// Check if this is a supported drive model.
			if (rpFile->isKreonDriveModel()) {
				// Do we have Kreon features?
				vector<uint16_t> features = rpFile->getKreonFeatureList();
				if (!features.empty()) {
					// Found Kreon features.
					// TODO: Check the feature list?
					d->isKreon = true;

					// Unlock the drive.
					d->unlockKreonDrive();

					// Re-read the device size.
					// Windows doesn't return the full device size
					// while the drive is locked, but Linux does.
					rpFile->rereadDeviceSizeScsi();
				}
			}
		}
	}

	// Create the DiscReader and XDVDFSPartition.
	d->discReader = new DiscReader(d->file);
	if (!d->discReader->isOpen()) {
		// Unable to open the discReader.
		delete d->discReader;
		d->discReader = nullptr;
		d->lockKreonDrive();
		d->isKreon = false;
		return;
	}
	d->xdvdfsPartition = new XDVDFSPartition(d->discReader, d->xdvdfs_addr, d->file->size() - d->xdvdfs_addr);
	if (!d->xdvdfsPartition->isOpen()) {
		// Unable to open the XDVDFSPartition.
		delete d->xdvdfsPartition;
		delete d->discReader;
		d->xdvdfsPartition = nullptr;
		d->discReader = nullptr;
		d->lockKreonDrive();
		d->isKreon = false;
		return;
	}

	// XDVDFS partition is open.
	if (d->discType <= XboxDiscPrivate::DISC_UNKNOWN) {
		// This is an extracted XDVDFS.
		d->discType = XboxDiscPrivate::DISC_TYPE_EXTRACTED;
	}

	// Disc image is ready.
	// NOTE: Kreon drives are left unlocked until the object is deleted.
	d->isValid = true;
}

/**
 * Close the opened file.
 */
void XboxDisc::close(void)
{
	RP_D(XboxDisc);

	// NOTE: Don't delete these. They have rp_image objects
	// that may be used by the UI later.
	if (d->defaultExeData) {
		d->defaultExeData->close();
	}
#if 0
	// TODO: Add close() functions?
	if (d->xdvdfsPartition) {
		d->xdvdfsPartition->close();
	}
	if (d->discReader) {
		d->discReader->close();
	}
#endif

	// Call the superclass function.
	super::close();
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int XboxDisc::isRomSupported_static(const DetectInfo *info)
{
	// NOTE: This version is NOT supported for XboxDisc.
	// Use the ISO-9660 PVD check instead.
	RP_UNUSED(info);
	assert(!"Use the ISO-9660 PVD check instead.");
	return -1;
}

/**
 * Is a ROM image supported by this class?
 * @param pvd ISO-9660 Primary Volume Descriptor.
 * @param pWave If non-zero, receives the wave number. (0 if none; non-zero otherwise.)
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int XboxDisc::isRomSupported_static(
	const ISO_Primary_Volume_Descriptor *pvd, uint8_t *pWave)
{
	// Xbox PVDs from the same manufacturing wave
	// match, so we can check the PVD timestamp
	// to determine if this is an Xbox disc.
	// TODO: Other ISO fields?
	assert(pvd != nullptr);
	if (!pvd) {
		// Bad.
		return -1;
	}

	// Get the creation time.
	const time_t btime = RomDataPrivate::pvd_time_to_unix_time(
		pvd->btime.full, pvd->btime.tz_offset);
	if (btime == -1) {
		// Invalid creation time.
		return -1;
	}

	// Compare to known times.
	// TODO: Enum with XGD waves?

	struct xgd_pvd_t {
		uint8_t xgd;	// XGD type. (1, 2, 3)
		uint8_t wave;	// Manufacturing wave.

		// NOTE: Using int32_t as an optimization, since
		// there won't be any Xbox 360 games released
		// after January 2038. (probably...)
		int32_t btime;	// Creation time.
	};

	static const xgd_pvd_t xgd_tbl[] = {
		// XGD1
		{1,  0, 1000334575},	// XGD1: 2001-09-13 10:42:55.00 '0' (+12:00)

		// XGD2
		{2,  1, 1128716326},	// XGD2 Wave 1:  2005-10-07 12:18:46.00 -08:00
		{2,  2, 1141708147},	// XGD2 Wave 2:  2006-03-06 21:09:07.00 -08:00
		{2,  3, 1231977600},	// XGD2 Wave 3:  2009-01-14 16:00:00.00 -08:00
		{2,  4, 1251158400},	// XGD2 Wave 4:  2009-08-24 17:00:00.00 -07:00
		{2,  5, 1254787200},	// XGD2 Wave 5:  2009-10-05 17:00:00.00 -07:00
		{2,  6, 1256860800},	// XGD2 Wave 6:  2009-10-29 17:00:00.00 -07:00
		{2,  7, 1266796800},	// XGD2 Wave 7:  2010-02-21 16:00:00.00 -08:00
		{2,  8, 1283644800},	// XGD2 Wave 8:  2010-09-04 17:00:00.00 -07:00
		{2,  9, 1284595200},	// XGD2 Wave 9:  2010-09-15 17:00:00.00 -07:00
		{2, 10, 1288310400},	// XGD2 Wave 10: 2010-10-28 17:00:00.00 -07:00
		{2, 11, 1295395200},	// XGD2 Wave 11: 2011-01-18 16:00:00.00 -08:00
		{2, 12, 1307923200},	// XGD2 Wave 12: 2011-06-12 17:00:00.00 -07:00
		{2, 13, 1310515200},	// XGD2 Wave 13: 2011-07-12 17:00:00.00 -07:00
		{2, 14, 1323302400},	// XGD2 Wave 14: 2011-12-07 16:00:00.00 -08:00
		{2, 15, 1329868800},	// XGD2 Wave 15: 2012-02-21 16:00:00.00 -08:00
		{2, 16, 1340323200},	// XGD2 Wave 16: 2012-06-21 17:00:00.00 -07:00
		{2, 17, 1352332800},	// XGD2 Wave 17: 2012-11-07 16:00:00.00 -08:00
		{2, 18, 1353283200},	// XGD2 Wave 18: 2012-11-18 16:00:00.00 -08:00
		{2, 19, 1377561600},	// XGD2 Wave 19: 2013-08-26 17:00:00.00 -07:00
		{2, 20, 1430092800},	// XGD2 Wave 20: 2015-04-26 17:00:00.00 -07:00

		// XGD3 does not have shared PVDs per wave,
		// but the timestamps all have a similar pattern:
		// - Year: 2011+
		// - Min, Sec, Csec: 00
		// - Hour and TZ:
		//   - 17, -07:00
		//   - 16, -08:00

		{0, 0, 0}
	};

	// TODO: Use a binary search instead of linear?
	for (const xgd_pvd_t *pXgd = xgd_tbl; pXgd->xgd != 0; pXgd++) {
		if (pXgd->btime == btime) {
			// Found a match!
			if (pWave) {
				*pWave = pXgd->wave;
			}
			return pXgd->xgd;
		}
	}

	// No match in the XGD table.
	if (btime >= 1293811200) {
		// Timestamp is after 2011/01/01 00:00:00.00 -08:00.
		// Check for XGD3.
		static const char xgd3_pvd_times[][9+1] = {
			"17000000\xE4",	// 17:00:00.00 -07:00
			"16000000\xE0",	// 16:00:00.00 -08:00
		};

		// TODO: Verify that this works correctly.
		if (!memcmp(&pvd->btime.full[8], xgd3_pvd_times[0], 9) ||
		     memcmp(&pvd->btime.full[8], xgd3_pvd_times[1], 9))
		{
			// Found a match!
			if (pWave) {
				*pWave = 0;
			}
			return XboxDiscPrivate::DISC_TYPE_XGD3;
		}
	}

	// Not XGD.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *XboxDisc::systemName(unsigned int type) const
{
	RP_D(const XboxDisc);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// XboxDisc has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"XboxDisc::systemName() array index optimization needs to be updated.");

	XboxDiscPrivate::ConsoleType consoleType = d->getConsoleType();
	switch (consoleType) {
		default:
		case XboxDiscPrivate::CONSOLE_TYPE_XBOX: {
			static const char *const sysNames_Xbox[4] = {
				"Microsoft Xbox", "Xbox", "Xbox", nullptr
			};
			return sysNames_Xbox[type & SYSNAME_TYPE_MASK];
		}

		case XboxDiscPrivate::CONSOLE_TYPE_XBOX_360: {
			static const char *const sysNames_X360[4] = {
				"Microsoft Xbox 360", "Xbox 360", "X360", nullptr
			};
			return sysNames_X360[type & SYSNAME_TYPE_MASK];
		}
	}

	// Should not get here...
	assert(!"XboxDisc::systemName(): Invalid system name.");
	return nullptr;
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
const char *const *XboxDisc::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".iso",		// ISO
		".xiso",	// Xbox ISO image
		// TODO: More?

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
const char *const *XboxDisc::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types from FreeDesktop.org..
		"application/x-cd-image",
		"application/x-iso9660-image",

		// TODO: XDVDFS?
		nullptr
	};
	return mimeTypes;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t XboxDisc::supportedImageTypes(void) const
{
	RP_D(const XboxDisc);
	RomData *const defaultExeData = const_cast<XboxDiscPrivate*>(d)->openDefaultExe();
	if (defaultExeData) {
		return defaultExeData->supportedImageTypes();
	}

	return 0;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> XboxDisc::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	RP_D(const XboxDisc);
	RomData *const defaultExeData = const_cast<XboxDiscPrivate*>(d)->openDefaultExe();
	if (defaultExeData) {
		return defaultExeData->supportedImageSizes(imageType);
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
uint32_t XboxDisc::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	RP_D(const XboxDisc);
	RomData *const defaultExeData = const_cast<XboxDiscPrivate*>(d)->openDefaultExe();
	if (defaultExeData) {
		return defaultExeData->imgpf(imageType);
	}

	return 0;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int XboxDisc::loadFieldData(void)
{
	RP_D(XboxDisc);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->discType < 0) {
		// Unknown disc type.
		return -EIO;
	}

	// Unlock the Kreon drive in order to read the executable.
	d->unlockKreonDrive();

	// XDVDFS partition.
	const XDVDFSPartition *const xdvdfsPartition = d->xdvdfsPartition;
	if (!xdvdfsPartition) {
		// XDVDFS partition isn't open.
		d->lockKreonDrive();
		return 0;
	}
	d->fields->reserve(3);	// Maximum of 3 fields.

	// Get the console name.
	XboxDiscPrivate::ConsoleType consoleType = d->getConsoleType();
	const char *s_tab_name;
	switch (consoleType) {
		default:
		case XboxDiscPrivate::CONSOLE_TYPE_XBOX:
			s_tab_name = "Xbox";
			break;
		case XboxDiscPrivate::CONSOLE_TYPE_XBOX_360:
			s_tab_name = "Xbox 360";
			break;
	}
	d->fields->setTabName(0, s_tab_name);

	// Disc type
	const char *const s_disc_type = C_("XboxDisc", "Disc Type");
	// NOTE: Not translating "Xbox Game Disc".
	switch (d->discType) {
		case XboxDiscPrivate::DISC_TYPE_EXTRACTED:
			d->fields->addField_string(s_disc_type,
				C_("XboxDisc", "Extracted XDVDFS"));
			break;
		case XboxDiscPrivate::DISC_TYPE_XGD1:
			d->fields->addField_string(s_disc_type, "Xbox Game Disc 1");
			break;
		case XboxDiscPrivate::DISC_TYPE_XGD2:
			d->fields->addField_string(s_disc_type,
				rp_sprintf("Xbox Game Disc 2 (Wave %u)", d->wave));
			break;
		case XboxDiscPrivate::DISC_TYPE_XGD3:
			d->fields->addField_string(s_disc_type, "Xbox Game Disc 3");
			break;
		default:
			d->fields->addField_string(s_disc_type,
				rp_sprintf(C_("RomData", "Unknown (%u)"), d->wave));
			break;
	}

	// Timestamp
	d->fields->addField_dateTime(C_("XboxDisc", "Disc Timestamp"),
		xdvdfsPartition->xdvdfsTimestamp(),
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME);

	// Do we have an XBE or XEX?
	// If so, add it to the current tab.
	int exeType;
	RomData *const defaultExeData = d->openDefaultExe(&exeType);
	if (defaultExeData) {
		// Boot filename.
		const char *s_boot_filename;
		switch (exeType) {
			case XboxDiscPrivate::EXE_TYPE_XBE:
				s_boot_filename = "default.xbe";
				break;
			case XboxDiscPrivate::EXE_TYPE_XEX:
				s_boot_filename = "default.xex";
				break;
			default:
				s_boot_filename = nullptr;
				break;
		}
		d->fields->addField_string(C_("XboxDisc", "Boot Filename"),
			(s_boot_filename ? s_boot_filename : C_("RomData", "Unknown")));

		// Add the fields.
		// NOTE: Adding tabs manually so we can show the disc info in
		// the primary tab.
		const RomFields *const exeFields = defaultExeData->fields();
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

	// ISO object for ISO-9660 PVD
	if (d->discType >= XboxDiscPrivate::DISC_TYPE_XGD1) {
		ISO *const isoData = new ISO(d->file);
		if (isoData->isOpen()) {
			// Add the fields.
			const RomFields *const isoFields = isoData->fields();
			if (isoFields) {
				d->fields->addFields_romFields(isoFields,
					RomFields::TabOffset_AddTabs);
			}
		}
		isoData->unref();
	}

	// Re-lock the Kreon drive.
	d->lockKreonDrive();

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int XboxDisc::loadMetaData(void)
{
	RP_D(XboxDisc);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->discType < 0) {
		// Unknown disc type.
		return -EIO;
	}

	// Unlock the Kreon drive in order to read the executable.
	d->unlockKreonDrive();

	// Make sure the default executable is loaded.
	const RomData *const defaultExeData = d->openDefaultExe();
	if (!defaultExeData) {
		// Unable to load the default executable.
		d->lockKreonDrive();
		return 0;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();

	// Add metadata properties from the default executable.
	// TODO: Also ISO PVD?
	d->metaData->addMetaData_metaData(defaultExeData->metaData());

	// Re-lock the Kreon drive.
	d->lockKreonDrive();

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
int XboxDisc::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(XboxDisc);
	const RomData *const defaultExeData = d->openDefaultExe();
	if (defaultExeData) {
		d->unlockKreonDrive();
		const int ret = const_cast<RomData*>(defaultExeData)->loadInternalImage(imageType, pImage);
		d->lockKreonDrive();
		return ret;
	}

	return -ENOENT;
}

}
