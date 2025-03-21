/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxDisc.cpp: Microsoft Xbox disc image parser.                         *
 *                                                                         *
 * Copyright (c) 2019-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XboxDisc.hpp"

// Other rom-properties libraries
#include "librpfile/RpFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

#ifdef _WIN32
#  include "librptext/wchar.hpp"
#else /* !_WIN32 */
#  define U82T_c(str) (str)
#endif /* _WIN32 */

// XDVDFSPartition
#include "../iso_structs.h"
#include "../disc/xdvdfs_structs.h"
#include "../disc/XDVDFSPartition.hpp"

// Other RomData subclasses
#include "Media/ISO.hpp"
#include "Xbox_XBE.hpp"
#include "Xbox360_XEX.hpp"

// C++ STL classes
using std::array;
using std::tstring;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class XboxDiscPrivate final : public RomDataPrivate
{
public:
	/**
	 * Open an Xbox disc image or XDVDFS partition.
	 * @param file Xbox disc image or XDVDFS partition
	 */
	explicit XboxDiscPrivate(const IRpFilePtr &file);

	/**
	 * Open an extracted Xbox disc file system.
	 * @param path Path to extracted Xbox disc file system (UTF-8)
	 */
	explicit XboxDiscPrivate(const char *path);
#if defined(_WIN32) && defined(_UNICODE)
	/**
	 * Open an extracted Xbox disc file system.
	 * @param path Path to extracted Xbox disc file system (UTF-16)
	 */
	explicit XboxDiscPrivate(const wchar_t *path);
#endif /* defined(_WIN32) && defined(_UNICODE) */

	~XboxDiscPrivate() final;

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(XboxDiscPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 2+1> exts;
	static const array<const char*, 2+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// Disc type
	enum class DiscType : int8_t {
		Unknown	= -1,

		// Not a full disc image; no ISO PVD, etc.
		XDVDFS		= 0,	// Standalone XDVDFS partition
		Extracted	= 1,	// Extracted disc file system

		// Full disc image, with ISO PVD
		XGD1		= 2,	// XGD1 (Original Xbox)
		XGD2		= 3,	// XGD2 (Xbox 360)
		XGD3		= 4,	// XGD3 (Xbox 360)

		Max
	};
	DiscType discType;
	uint8_t wave;	// XGD2: Wave number
	bool isKreon;	// Are we using a Kreon drive?

	// Directory path [for DiscType::Extracted only]
	std::tstring path;

	// XDVDFS starting address
	off64_t xdvdfs_addr;

public:
	// XDVDFSPartition
	XDVDFSPartitionPtr xdvdfsPartition;

	// default.xbe / default.xex
	unique_ptr<RomData> defaultExeData;

	enum class ExeType {
		Unknown	= -1,

		XBE	= 0,	// Xbox XBE
		XEX	= 1,	// Xbox 360 XEX

		Max
	};
	ExeType exeType;

	/**
	 * Open a file from the disc image or extracted file system directory.
	 * @param filename Filename (ASCII only?)
	 * @return Opened file, or nullptr on error.
	 */
	IRpFilePtr open(const char *filename);

	/**
	 * Open default.xbe / default.xex.
	 * @param pExeType	[out] EXE type.
	 * @return RomData* on success; nullptr on error.
	 */
	RomData *openDefaultExe(ExeType *pExeType = nullptr);

	enum class ConsoleType {
		Unknown	= -1,

		Xbox	= 0,	// Xbox
		Xbox360	= 1,	// Xbox 360

		Max
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

	/**
	 * Unlock the Kreon drive.
	 */
	inline void unlockKreonDrive(void);

	/**
	 * Lock the Kreon drive.
	 */
	inline void lockKreonDrive(void);
};

ROMDATA_IMPL(XboxDisc)

/** XboxDiscPrivate **/

/* RomDataInfo */
const array<const char*, 2+1> XboxDiscPrivate::exts = {{
	".iso",		// ISO
	".xiso",	// Xbox ISO image
	// TODO: More?

	nullptr
}};
const array<const char*, 2+1> XboxDiscPrivate::mimeTypes = {{
	// Unofficial MIME types from FreeDesktop.org..
	"application/x-cd-image",
	"application/x-iso9660-image",

	// TODO: XDVDFS?
	nullptr
}};
const RomDataInfo XboxDiscPrivate::romDataInfo = {
	"XboxDisc", exts.data(), mimeTypes.data()
};

/**
 * Open an Xbox disc image or XDVDFS partition.
 * @param file Xbox disc image or XDVDFS partition
 */
XboxDiscPrivate::XboxDiscPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, discType(DiscType::Unknown)
	, wave(0)
	, isKreon(false)
	, xdvdfs_addr(0)
	, exeType(ExeType::Unknown)
{}

/**
 * Open an extracted Xbox disc file system.
 * @param path Path to extracted Xbox disc file system (UTF-8)
 */
XboxDiscPrivate::XboxDiscPrivate(const char *path)
	: super({}, &romDataInfo)
	, discType(DiscType::Unknown)
	, wave(0)
	, isKreon(false)
	, xdvdfs_addr(0)
	, exeType(ExeType::Unknown)
{
	if (path && path[0] != '\0') {
#ifdef _WIN32
		// Windows: Storing the path as UTF-16 internally.
		this->path.assign(U82T_c(path));
#else /* !_WIN32 */
		this->path.assign(path);
#endif /* _WIN32 */
	}
}

#if defined(_WIN32) && defined(_UNICODE)
/**
 * Open an extracted Xbox disc file system.
 * @param path Path to extracted Xbox disc file system (UTF-16)
 */
XboxDiscPrivate::XboxDiscPrivate(const wchar_t *path)
	: super({}, &romDataInfo)
	, discType(DiscType::Unknown)
	, wave(0)
	, isKreon(false)
	, xdvdfs_addr(0)
	, exeType(ExeType::Unknown)
{
	if (path && path[0] != '\0') {
		this->path.assign(path);
	}
}
#endif /* defined(_WIN32) && defined(_UNICODE) */

XboxDiscPrivate::~XboxDiscPrivate()
{
	if (isKreon) {
		lockKreonDrive();
	}
}

/**
 * Open a file from the disc image or extracted file system directory.
 * @param filename Filename (ASCII only?)
 * @return Opened file, or nullptr on error.
 */
IRpFilePtr XboxDiscPrivate::open(const char *filename)
{
	// NOTE: Cannot check discType here, since it might not be initialized yet.
	// If this is an extracted disc file system, d->file will be nullptr.
	if (likely(this->file)) {
		// Disc image or partition image.
		// Make sure the XDVDFS partition is open.
		if (!xdvdfsPartition || !xdvdfsPartition->isOpen()) {
			// XDVDFS partition is not open.
			return {};
		}

		// Open the file from the XDVDFS partition.
		return xdvdfsPartition->open(filename);
	}

	// Extracted disc file system.
	// Append the filename to the selected path and try to open it.
	tstring ts_full_filename(this->path);
	ts_full_filename += DIR_SEP_CHR;

	// Remove leading slashes, if present.
	while (*filename == _T('/')) {
		filename++;
	}
	if (*filename == '\0') {
		// Oops, no filename...
		return {};
	}

	const size_t old_sz = ts_full_filename.size();
	ts_full_filename += U82T_c(filename);
#ifdef _WIN32
	// Replace all slashes with backslashes.
	const auto start_iter = ts_full_filename.begin() + old_sz;
	std::transform(start_iter, ts_full_filename.end(), start_iter, [](TCHAR c) {
		return (c == '/') ? DIR_SEP_CHR : c;
	});
#endif /* _WIN32 */

	IRpFilePtr f = std::make_shared<RpFile>(ts_full_filename, RpFile::FM_OPEN_READ);
	if (f && f->isOpen()) {
		return f;
	}

	// Try some permutations for case-sensitive host file systems.
	// TODO: Handle subdirectories?

	// Make the first character uppercase.
	ts_full_filename[old_sz] = TOUPPER(ts_full_filename[old_sz]);
	f = std::make_shared<RpFile>(ts_full_filename, RpFile::FM_OPEN_READ);
	if (f && f->isOpen()) {
		return f;
	}

	// Make the whole thing uppercase.
	const auto toupper_iter = ts_full_filename.begin() + old_sz + 1;
	std::transform(toupper_iter, ts_full_filename.end(), toupper_iter, [](TCHAR c) {
		return TOUPPER(c);
	});
	return std::make_shared<RpFile>(ts_full_filename, RpFile::FM_OPEN_READ);;
}

/**
 * Open default.xbe / default.xex.
 * @param pExeType	[out] EXE type.
 * @return RomData* on success; nullptr on error.
 */
RomData *XboxDiscPrivate::openDefaultExe(ExeType *pExeType)
{
	if (defaultExeData) {
		// default.xbe / default.xex is already open.
		if (pExeType) {
			*pExeType = exeType;
		}
		return defaultExeData.get();
	}

	// Try to open default.xex.
	IRpFilePtr f_defaultExe(this->open("/default.xex"));
	if (f_defaultExe) {
		RomData *const xexData = new Xbox360_XEX(f_defaultExe);
		if (xexData->isValid()) {
			// default.xex is open and valid.
			defaultExeData.reset(xexData);
			exeType = ExeType::XEX;
			if (pExeType) {
				*pExeType = ExeType::XEX;
			}
			return xexData;
		}

		// Not actually an XEX.
		delete xexData;
	}

	// Try to open default.xbe.
	// TODO: What about discs that have both?
	f_defaultExe = this->open("/default.xbe");
	if (f_defaultExe) {
		RomData *const xbeData = new Xbox_XBE(f_defaultExe);
		if (xbeData->isValid()) {
			// default.xbe is open and valid.
			defaultExeData.reset(xbeData);
			exeType = ExeType::XBE;
			if (pExeType) {
				*pExeType = ExeType::XBE;
			}
			return xbeData;
		}

		// Not actually an XBE.
		delete xbeData;
	}

	// Unable to open the default executable.
	exeType = ExeType::Unknown;
	if (pExeType) {
		*pExeType = ExeType::Unknown;
	}
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
	ExeType exeType;
	RomData *const defaultExeData = const_cast<XboxDiscPrivate*>(this)->openDefaultExe(&exeType);
	if (defaultExeData) {
		// Default executable loaded.
		switch (exeType) {
			case ExeType::XBE:
				return ConsoleType::Xbox;
			case ExeType::XEX:
				return ConsoleType::Xbox360;
			default:
				break;
		}
	}

	// Unable to load the EXE.
	// Use the disc type.
	if (discType >= XboxDiscPrivate::DiscType::XGD2) {
		return ConsoleType::Xbox360;
	}

	// Assume Xbox for XGD1, XDVDFS partition, and extracted disc file system.
	return ConsoleType::Xbox;
}

/**
 * Unlock the Kreon drive.
 */
inline void XboxDiscPrivate::unlockKreonDrive(void)
{
	if (!isKreon || !this->file) {
		return;
	}

	RpFile *const rpFile = dynamic_cast<RpFile*>(this->file.get());
	if (rpFile) {
		rpFile->setKreonErrorSkipState(true);
		rpFile->setKreonLockState(RpFile::KreonLockState::State2WxRipper);
	}
}

/**
 * Lock the Kreon drive.
 */
inline void XboxDiscPrivate::lockKreonDrive(void)
{
	if (!isKreon || !this->file) {
		return;
	}

	RpFile *const rpFile = dynamic_cast<RpFile*>(this->file.get());
	if (rpFile) {
		rpFile->setKreonErrorSkipState(false);
		rpFile->setKreonLockState(RpFile::KreonLockState::Locked);
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
XboxDisc::XboxDisc(const IRpFilePtr &file)
	: super(new XboxDiscPrivate(file))
{
	init();
}

/**
 * Read an extracted Microsoft Xbox disc file system.
 *
 * NOTE: Extracted Xbox disc file systems are directories.
 * This constructor takes a local directory path.
 *
 * NOTE: Check isValid() to determine if the directory is supported by this class.
 *
 * @param path Local directory path (UTF-8)
 */
XboxDisc::XboxDisc(const char *path)
	: super(new XboxDiscPrivate(path))
{
	init();
}

#if defined(_WIN32) && defined(_UNICODE)
/**
 * Read an extracted Microsoft Xbox disc file system.
 *
 * NOTE: Extracted Xbox disc file systems are directories.
 * This constructor takes a local directory path.
 *
 * NOTE: Check isValid() to determine if the directory is supported by this class.
 *
 * @param path Local directory path (UTF-16)
 */
XboxDisc::XboxDisc(const wchar_t *path)
	: super(new XboxDiscPrivate(path))
{
	init();
}
#endif /* defined(_WIN32) && defined(_UNICODE) */

/**
 * Internal initialization function for the three constructors.
 */
void XboxDisc::init()
{
	// This class handles disc images.
	RP_D(XboxDisc);
	d->mimeType = "application/x-cd-image";	// unofficial
	d->fileType = FileType::DiscImage;

	if (!d->path.empty()) {
		// We're handling an extracted disc file system.
		// TODO: File type for "extracted file system"?
		d->mimeType = "inode/directory";
		d->fileType = FileType::ApplicationPackage;

		// NOTE: No need to call isDirSupported_static() here,
		// since we're effectively doing that by attempting to
		// open the default executable.

		// Nothing else to check other than if default.xbe/default.xex is present.

		// Check the PAL status of the executable.
		RomData *const defaultExeData = d->openDefaultExe();
		if (!defaultExeData) {
			// Could not open default.xbe/default.xex???
			return;
		}

		d->discType = XboxDiscPrivate::DiscType::Extracted;
		d->isPAL = defaultExeData->isPAL();
		d->isValid = true;
		return;
	} else if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ISO-9660 PVD.
	// NOTE: Only 2048-byte sectors, since this is DVD.
	ISO_Primary_Volume_Descriptor pvd;
	size_t size = d->file->seekAndRead(ISO_PVD_ADDRESS_2048, &pvd, sizeof(pvd));
	if (size != sizeof(pvd)) {
		d->file.reset();
		return;
	}

	// Check if this disc image is supported.
	d->discType = static_cast<XboxDiscPrivate::DiscType>(isRomSupported_static(&pvd, &d->wave));

	switch (d->discType) {
		case XboxDiscPrivate::DiscType::XGD1:
			d->xdvdfs_addr = XDVDFS_LBA_OFFSET_XGD1 * XDVDFS_BLOCK_SIZE;
			break;
		case XboxDiscPrivate::DiscType::XGD2:
			// NOTE: May be XGD3.
			// If XDVDFS is not present at the XGD2 offset, try XGD3.
			d->xdvdfs_addr = XDVDFS_LBA_OFFSET_XGD2 * XDVDFS_BLOCK_SIZE;
			break;
		case XboxDiscPrivate::DiscType::XGD3:
			d->xdvdfs_addr = XDVDFS_LBA_OFFSET_XGD3 * XDVDFS_BLOCK_SIZE;
			break;
		default:
			// This might be a standalone XDVDFS partition.
			d->xdvdfs_addr = 0;
			break;
	}

	// Is the file large enough?
	// Must have at least the first XDVDFS sector.
	const off64_t fileSize = d->file->size();
	if (fileSize < d->xdvdfs_addr + XDVDFS_BLOCK_SIZE) {
		// File is too small.
		d->file.reset();
		return;
	}

	// If this is a Kreon drive, unlock it.
	if (d->file->isDevice()) {
		RpFile *const rpFile = dynamic_cast<RpFile*>(d->file.get());
		if (rpFile && rpFile->isKreonDriveModel()) {
			// Do we have Kreon features?
			const vector<RpFile::KreonFeature> features = rpFile->getKreonFeatureList();
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

	// Open the XDVDFSPartition.
	d->xdvdfsPartition = std::make_shared<XDVDFSPartition>(d->file, d->xdvdfs_addr, d->file->size() - d->xdvdfs_addr);
	if (!d->xdvdfsPartition->isOpen()) {
		// Unable to open the XDVDFSPartition.
		d->xdvdfsPartition.reset();

		// If this is XGD2, try the XGD3 offset in case this happens
		// to be an edge case where it's an XGD3 disc that has a video
		// partition that matches an XGD2 timestamp.
		if (d->discType == XboxDiscPrivate::DiscType::XGD2) {
			static constexpr off64_t xgd3_offset = XDVDFS_LBA_OFFSET_XGD3 * XDVDFS_BLOCK_SIZE;
			d->xdvdfs_addr = XDVDFS_LBA_OFFSET_XGD3 * XDVDFS_BLOCK_SIZE;
			d->xdvdfsPartition = std::make_shared<XDVDFSPartition>(d->file,
				xgd3_offset, d->file->size() - xgd3_offset);
			if (d->xdvdfsPartition->isOpen()) {
				// It's an XGD3.
				d->discType = XboxDiscPrivate::DiscType::XGD3;
				d->wave = 0;
				d->xdvdfs_addr = xgd3_offset;
			} else {
				// It's not an XGD3.
				d->xdvdfsPartition.reset();
			}
		}

		if (!d->xdvdfsPartition) {
			// Unable to open the XDVDFSPartition.
			d->file.reset();
			d->lockKreonDrive();
			d->isKreon = false;
			return;
		}
	}

	// XDVDFS partition is open.
	if (d->discType <= XboxDiscPrivate::DiscType::Unknown) {
		// This is a standalone XDVDFS partition.
		d->discType = XboxDiscPrivate::DiscType::XDVDFS;
	}

	// Disc image is ready.
	// NOTE: Kreon drives are left unlocked until the object is deleted.
	d->isValid = true;

	// Check the PAL status from the executable.
	RomData *const defaultExeData = d->openDefaultExe();
	if (defaultExeData) {
		d->isPAL = defaultExeData->isPAL();
	}
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

	d->xdvdfsPartition.reset();

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
		return static_cast<int>(XboxDiscPrivate::DiscType::Unknown);
	}

	// Get the creation time.
	const time_t btime = RomDataPrivate::pvd_time_to_unix_time(
		pvd->btime.full, pvd->btime.tz_offset);
	if (btime == -1) {
		// Invalid creation time.
		return static_cast<int>(XboxDiscPrivate::DiscType::Unknown);
	}

	// Compare to known times.
	// TODO: Enum with XGD waves?
	// NOTE: XGD1 is no longer '1', so we're using the
	// XboxDiscPrivate::DiscType enum now.
	using DiscType = XboxDiscPrivate::DiscType;

	struct xgd_pvd_t {
		DiscType xgd;	// XGD type
		uint8_t wave;			// Manufacturing wave.

		// NOTE: Using int32_t as an optimization, since
		// there won't be any Xbox 360 games released
		// after January 2038. (probably...)
		int32_t btime;	// Creation time.
	};

	static const array<xgd_pvd_t, 21> xgd_tbl = {{
		// XGD1
		{DiscType::XGD1,  0, 1000334575},	// XGD1: 2001-09-13 10:42:55.00 '0' (+12:00)

		// XGD2
		{DiscType::XGD2,  1, 1128716326},	// XGD2 Wave 1:  2005-10-07 12:18:46.00 -08:00
		{DiscType::XGD2,  2, 1141708147},	// XGD2 Wave 2:  2006-03-06 21:09:07.00 -08:00
		{DiscType::XGD2,  3, 1231977600},	// XGD2 Wave 3:  2009-01-14 16:00:00.00 -08:00
		{DiscType::XGD2,  4, 1251158400},	// XGD2 Wave 4:  2009-08-24 17:00:00.00 -07:00
		{DiscType::XGD2,  5, 1254787200},	// XGD2 Wave 5:  2009-10-05 17:00:00.00 -07:00
		{DiscType::XGD2,  6, 1256860800},	// XGD2 Wave 6:  2009-10-29 17:00:00.00 -07:00
		{DiscType::XGD2,  7, 1266796800},	// XGD2 Wave 7:  2010-02-21 16:00:00.00 -08:00
		{DiscType::XGD2,  8, 1283644800},	// XGD2 Wave 8:  2010-09-04 17:00:00.00 -07:00
		{DiscType::XGD2,  9, 1284595200},	// XGD2 Wave 9:  2010-09-15 17:00:00.00 -07:00
		{DiscType::XGD2, 10, 1288310400},	// XGD2 Wave 10: 2010-10-28 17:00:00.00 -07:00
		{DiscType::XGD2, 11, 1295395200},	// XGD2 Wave 11: 2011-01-18 16:00:00.00 -08:00
		{DiscType::XGD2, 12, 1307923200},	// XGD2 Wave 12: 2011-06-12 17:00:00.00 -07:00
		{DiscType::XGD2, 13, 1310515200},	// XGD2 Wave 13: 2011-07-12 17:00:00.00 -07:00
		{DiscType::XGD2, 14, 1323302400},	// XGD2 Wave 14: 2011-12-07 16:00:00.00 -08:00
		{DiscType::XGD2, 15, 1329868800},	// XGD2 Wave 15: 2012-02-21 16:00:00.00 -08:00
		{DiscType::XGD2, 16, 1340323200},	// XGD2 Wave 16: 2012-06-21 17:00:00.00 -07:00
		{DiscType::XGD2, 17, 1352332800},	// XGD2 Wave 17: 2012-11-07 16:00:00.00 -08:00
		{DiscType::XGD2, 18, 1353283200},	// XGD2 Wave 18: 2012-11-18 16:00:00.00 -08:00
		{DiscType::XGD2, 19, 1377561600},	// XGD2 Wave 19: 2013-08-26 17:00:00.00 -07:00
		{DiscType::XGD2, 20, 1430092800},	// XGD2 Wave 20: 2015-04-26 17:00:00.00 -07:00

		// XGD3 does not have shared PVDs per wave,
		// but the timestamps all have a similar pattern:
		// - Year: 2011+
		// - Min, Sec, Csec: 00
		// - Hour and TZ:
		//   - 17, -07:00
		//   - 16, -08:00
	}};

	// TODO: Use std::lower_bound() instead?
	auto iter = std::find_if(xgd_tbl.cbegin(), xgd_tbl.cend(),
		[btime](const xgd_pvd_t &p) noexcept -> bool {
			return (p.btime == btime);
		});
	if (iter != xgd_tbl.cend()) {
		// Found a match!
		if (pWave) {
			*pWave = iter->wave;
		}
		return static_cast<int>(iter->xgd);
	}

	// No match in the XGD table.
	if (btime >= 1293811200) {
		// Timestamp is after 2011/01/01 00:00:00.00 -08:00.
		// Check for XGD3.
		static constexpr char xgd3_pvd_times[][9+1] = {
			"17000000\xE4",	// 17:00:00.00 -07:00
			"16000000\xE0",	// 16:00:00.00 -08:00
		};

		// TODO: Verify that this works correctly.
		if (!memcmp(&pvd->btime.full[8], xgd3_pvd_times[0], 9) ||
		    !memcmp(&pvd->btime.full[8], xgd3_pvd_times[1], 9))
		{
			// Found a match!
			if (pWave) {
				*pWave = 0;
			}
			return static_cast<int>(DiscType::XGD3);
		}
	}

	// Not XGD.
	return static_cast<int>(DiscType::Unknown);
}

/**
 * Is a directory supported by this class?
 * @param path Directory to check
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int XboxDisc::isDirSupported_static(const char *path)
{
	// Check for an extracted Xbox disc file system.
	static const array<const char*, 6> Xbox_exe_filenames = {{
		"default.xbe",	// Original Xbox
		"default.xex",	// Xbox 360

		// Case permutations for case-sensitive host file systems.
		// NOTE: Not common on Windows, except for WSL...
		// TODO: Find more variants?
		"Default.xbe", "DEFAULT.XBE",
		"Default.xex", "DEFAULT.XEX",
	}};

	if (RomDataPrivate::T_isDirSupported_anyFile_static(path, Xbox_exe_filenames)) {
		return static_cast<int>(XboxDiscPrivate::DiscType::Extracted);
	}

	// Not supported.
	return static_cast<int>(XboxDiscPrivate::DiscType::Unknown);
}

#if defined(_WIN32) && defined(_UNICODE)
/**
 * Is a directory supported by this class?
 * @param path Directory to check
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int XboxDisc::isDirSupported_static(const wchar_t *path)
{
	// Check for an extracted Xbox disc file system.
	static const array<const wchar_t*, 6> Xbox_exe_filenames = {{
		L"default.xbe",	// Original Xbox
		L"default.xex",	// Xbox 360

		// Case permutations for case-sensitive host file systems.
		// NOTE: Not common on Windows, except for WSL...
		// TODO: Find more variants?
		L"Default.xbe", L"DEFAULT.XBE",
		L"Default.xex", L"DEFAULT.XEX",
	}};

	if (XboxDiscPrivate::T_isDirSupported_anyFile_static(path, Xbox_exe_filenames)) {
		return static_cast<int>(XboxDiscPrivate::DiscType::Extracted);
	}

	// Not supported.
	return static_cast<int>(XboxDiscPrivate::DiscType::Unknown);
}
#endif /* defined(_WIN32) && defined(_UNICODE) */

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

	const XboxDiscPrivate::ConsoleType consoleType = d->getConsoleType();
	switch (consoleType) {
		default:
		case XboxDiscPrivate::ConsoleType::Xbox: {
			static const array<const char*, 4> sysNames_Xbox = {{
				"Microsoft Xbox", "Xbox", "Xbox", nullptr
			}};
			return sysNames_Xbox[type & SYSNAME_TYPE_MASK];
		}

		case XboxDiscPrivate::ConsoleType::Xbox360: {
			static const array<const char*, 4> sysNames_X360 = {{
				"Microsoft Xbox 360", "Xbox 360", "X360", nullptr
			}};
			return sysNames_X360[type & SYSNAME_TYPE_MASK];
		}
	}

	// Should not get here...
	assert(!"XboxDisc::systemName(): Invalid system name.");
	return nullptr;
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
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if ((!d->file || !d->file->isOpen()) && d->path.empty()) {
		// File/directory isn't open.
		return -EBADF;
	} else if (!d->isValid || static_cast<int>(d->discType) < 0) {
		// Unknown disc type.
		return -EIO;
	}

	const XDVDFSPartition *xdvdfsPartition = nullptr;
	if (d->file) {
		// Unlock the Kreon drive in order to read the executable.
		d->unlockKreonDrive();

		// XDVDFS partition. (NOTE: Using the raw pointer.)
		xdvdfsPartition = d->xdvdfsPartition.get();
		if (!xdvdfsPartition) {
			// XDVDFS partition isn't open.
			d->lockKreonDrive();
			return 0;
		}
		d->fields.reserve(3);	// Maximum of 3 fields.
	} else {
		// Extracted Xbox disc file system
		d->fields.reserve(1);	// Maximum of 1 field.
	}

	// Get the console name.
	const XboxDiscPrivate::ConsoleType consoleType = d->getConsoleType();
	const char *s_tab_name;
	switch (consoleType) {
		default:
		case XboxDiscPrivate::ConsoleType::Xbox:
			s_tab_name = "Xbox";
			break;
		case XboxDiscPrivate::ConsoleType::Xbox360:
			s_tab_name = "Xbox 360";
			break;
	}
	d->fields.setTabName(0, s_tab_name);

	// Disc type
	const char *const s_disc_type = C_("XboxDisc", "Disc Type");
	// NOTE: Not translating "Xbox Game Disc".
	switch (d->discType) {
		case XboxDiscPrivate::DiscType::XDVDFS:
			d->fields.addField_string(s_disc_type,
				C_("XboxDisc", "XDVDFS Partition"));
			break;
		case XboxDiscPrivate::DiscType::Extracted:
			d->fields.addField_string(s_disc_type,
				C_("XboxDisc", "Extracted Disc File System"));
			break;
		case XboxDiscPrivate::DiscType::XGD1:
			d->fields.addField_string(s_disc_type, "Xbox Game Disc 1");
			break;
		case XboxDiscPrivate::DiscType::XGD2:
			d->fields.addField_string(s_disc_type,
				fmt::format(FSTR("Xbox Game Disc 2 (Wave {:d})"), d->wave));
			break;
		case XboxDiscPrivate::DiscType::XGD3:
			d->fields.addField_string(s_disc_type, "Xbox Game Disc 3");
			break;
		default:
			d->fields.addField_string(s_disc_type,
				fmt::format(FRUN(C_("RomData", "Unknown ({:d})")), d->wave));
			break;
	}

	// Timestamp (from the XDVDFS partition)
	if (xdvdfsPartition) {
		d->fields.addField_dateTime(C_("XboxDisc", "Disc Timestamp"),
			xdvdfsPartition->xdvdfsTimestamp(),
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_HAS_TIME);
	}

	// Do we have an XBE or XEX?
	// If so, add it to the current tab.
	XboxDiscPrivate::ExeType exeType;
	RomData *const defaultExeData = d->openDefaultExe(&exeType);
	if (defaultExeData) {
		// Boot filename.
		const char *s_boot_filename;
		switch (exeType) {
			case XboxDiscPrivate::ExeType::XBE:
				s_boot_filename = "default.xbe";
				break;
			case XboxDiscPrivate::ExeType::XEX:
				s_boot_filename = "default.xex";
				break;
			default:
				s_boot_filename = nullptr;
				break;
		}
		d->fields.addField_string(C_("XboxDisc", "Boot Filename"),
			(s_boot_filename ? s_boot_filename : C_("RomData", "Unknown")));

		// Add the fields.
		// NOTE: Adding tabs manually so we can show the disc info in
		// the primary tab.
		const RomFields *const exeFields = defaultExeData->fields();
		if (exeFields) {
			const int exeTabCount = exeFields->tabCount();
			for (int i = 1; i < exeTabCount; i++) {
				d->fields.setTabName(i, exeFields->tabName(i));
			}
			d->fields.setTabIndex(0);
			d->fields.addFields_romFields(exeFields, 0);
			d->fields.setTabIndex(exeTabCount - 1);
		}
	}

	// ISO object for ISO-9660 PVD
	if (d->discType >= XboxDiscPrivate::DiscType::XGD1) {
		ISO *const isoData = new ISO(d->file);
		if (isoData->isOpen()) {
			// Add the fields.
			const RomFields *const isoFields = isoData->fields();
			if (isoFields) {
				d->fields.addFields_romFields(isoFields,
					RomFields::TabOffset_AddTabs);
			}
		}
		delete isoData;
	}

	if (d->file) {
		// Re-lock the Kreon drive.
		d->lockKreonDrive();
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int XboxDisc::loadMetaData(void)
{
	RP_D(XboxDisc);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if ((!d->file || !d->file->isOpen()) && d->path.empty()) {
		// File/directory isn't open.
		return -EBADF;
	} else if (!d->isValid || static_cast<int>(d->discType) < 0) {
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

	// Add metadata properties from the default executable.
	// ISO PVD is skipped because it's the same for all discs
	// of a given XGD wave.
	d->metaData.addMetaData_metaData(defaultExeData->metaData());

	// Re-lock the Kreon drive.
	d->lockKreonDrive();

	// Finished reading the metadata.
	return static_cast<int>(d->metaData.count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int XboxDisc::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
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

/**
 * Check for "viewed" achievements.
 *
 * @return Number of achievements unlocked.
 */
int XboxDisc::checkViewedAchievements(void) const
{
	RP_D(const XboxDisc);
	if (!d->isValid) {
		// Disc is not valid.
		return 0;
	}

	RomData *const exe = const_cast<XboxDiscPrivate*>(d)->openDefaultExe();
	if (!exe) {
		// Unable to open the EXE.
		return 0;
	}

	// Check the EXE for the viewed achievements.
	return exe->checkViewedAchievements();
}

} // namespace LibRomData
