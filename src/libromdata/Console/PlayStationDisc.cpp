/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PlayStationDisc.cpp: PlayStation 1 and 2 disc image reader.             *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"

#include "PlayStationDisc.hpp"
#include "ps2_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// IsoPartition
#include "../cdrom_structs.h"
#include "../iso_structs.h"
#include "../disc/Cdrom2352Reader.hpp"
#include "../disc/IsoPartition.hpp"

// Other RomData subclasses
#include "Other/ISO.hpp"
#include "Console/PlayStationEXE.hpp"
#include "Other/ELF.hpp"

// inih for SYSTEM.CNF
#include "ini.h"

// C++ STL classes
using std::string;
using std::unordered_map;
using std::vector;

namespace LibRomData {

class PlayStationDiscPrivate final : public RomDataPrivate
{
	public:
		PlayStationDiscPrivate(const IRpFilePtr &file);
		~PlayStationDiscPrivate() final = default;

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(PlayStationDiscPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		ISO_Primary_Volume_Descriptor pvd;

		// SYSTEM.CNF contents
		// Keys are stored in upper-case.
		unordered_map<string, string> system_cnf;

		/**
		 * ini.h callback for parsing SYSTEM.CNF.
		 * @param user		[in] User data parameter (this)
		 * @param section	[in] Section name
		 * @param name		[in] Value name
		 * @param value		[in] Value
		 * @return 0 to continue; 1 to stop.
		 */
		static int parse_system_cnf(void *user, const char *section, const char *name, const char *value);

		/**
		 * Load SYSTEM.CNF.
		 * @param pt IPartition containing SYSTEM.CNF.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadSystemCnf(const IsoPartitionPtr &pt);

		// IsoPartition
		IDiscReaderPtr discReader;
		IsoPartitionPtr isoPartition;

		// Boot executable
		RomDataPtr bootExeData;

		// Boot filename.
		// Normalized:
		// - "cdrom:\" (PS1) or "cdrom0:\" (PS2) removed.
		// - ISO version number removed.
		string boot_filename;

		// Optional boot argument.
		string boot_argument;

		/**
		 * Open the boot executable.
		 * @return RomData* on success; nullptr on error.
		 */
		RomDataPtr openBootExe(void);

		enum class ConsoleType {
			Unknown	= -1,

			PS1	= 0,	// PlayStation
			PS2	= 1,	// PlayStation 2

			Max
		};
		ConsoleType consoleType;
};

ROMDATA_IMPL(PlayStationDisc)

/** PlayStationDiscPrivate **/

/* RomDataInfo */
const char *const PlayStationDiscPrivate::exts[] = {
	".iso",		// ISO
	".bin",		// BIN/CUE
	".img",		// CCD/IMG
	// TODO: More?

	nullptr
};
const char *const PlayStationDiscPrivate::mimeTypes[] = {
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-cd-image",
	"application/x-iso9660-image",

	// Custom, for 2352-byte sector disc images
	"application/x-raw-cd-image",

	// TODO: PS1/PS2?
	nullptr
};
const RomDataInfo PlayStationDiscPrivate::romDataInfo = {
	"PlayStationDisc", exts, mimeTypes
};

PlayStationDiscPrivate::PlayStationDiscPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, consoleType(ConsoleType::Unknown)
{
	// Clear the structs.
	memset(&pvd, 0, sizeof(pvd));
}

/**
 * ini.h callback for parsing SYSTEM.CNF.
 * @param user		[in] User data parameter (this)
 * @param section	[in] Section name
 * @param name		[in] Value name
 * @param value		[in] Value
 * @return 0 to continue; 1 to stop.
 */
int PlayStationDiscPrivate::parse_system_cnf(void *user, const char *section, const char *name, const char *value)
{
	if (section[0] != '\0') {
		// Sections aren't expected here...
		return 1;
	}

	// Save the value for later.
	string s_name(name);
	std::transform(s_name.begin(), s_name.end(), s_name.begin(),
		[](unsigned char c) noexcept -> char { return std::toupper(c); });

	PlayStationDiscPrivate *const d = static_cast<PlayStationDiscPrivate*>(user);
	auto ret = d->system_cnf.emplace(std::move(s_name), value);
	return (ret.second ? 0 : 1);
}

/**
 * Load SYSTEM.CNF.
 * @param pt IPartition containing SYSTEM.CNF.
 * @return 0 on success; negative POSIX error code on error.
 */
int PlayStationDiscPrivate::loadSystemCnf(const IsoPartitionPtr &pt)
{
	if (!system_cnf.empty()) {
		// Already loaded.
		return 0;
	}

	const IRpFilePtr f_system_cnf(pt->open("SYSTEM.CNF"));
	if (!f_system_cnf) {
		// SYSTEM.CNF might not be present.
		// If it isn't, but PSX.EXE is present, use default values.
		int ret = pt->lastError();
		if (ret == ENOENT) {
			// SYSTEM.CNF not found. Check for PSX.EXE.
			const IRpFilePtr f_psx_exe(pt->open("PSX.EXE"));
			if (f_psx_exe && f_psx_exe->isOpen()) {
				// Found PSX.EXE.
				boot_filename = "PSX.EXE";
				system_cnf.emplace("BOOT", boot_filename);
				// Pretend that we did find SYSTEM.CNF.
				return 0;
			} else {
				// Not found.
				return -ENOENT;
			}
		} else {
			return (ret == 0 ? -EIO : -ret);
		}
	} else if (!f_system_cnf->isOpen()) {
		int ret = -f_system_cnf->lastError();
		if (ret == 0) {
			ret = -EIO;
		}
		return ret;
	}

	// CNF file should be less than 2048 bytes.
	const off64_t fileSize = f_system_cnf->size();
	if (fileSize > 2048) {
		return -ENOMEM;
	}

	// Read the entire file into memory.
	char buf[2049];
	size_t size = f_system_cnf->read(buf, 2048);
	if (size != static_cast<size_t>(fileSize)) {
		// Short read.
		return -EIO;
	}
	buf[static_cast<size_t>(fileSize)] = '\0';

	// Process the file.
	// TODO: Fail on error?
	ini_parse_string(buf, parse_system_cnf, this);

	return (!system_cnf.empty() ? 0 : -EIO);
}

/**
 * Open the boot executable.
 * @return RomData* on success; nullptr on error.
 */
RomDataPtr PlayStationDiscPrivate::openBootExe(void)
{
	// FIXME: Returning `const RomDataPtr &` would be better,
	// but the compiler is complaining that the nullptrs end up
	// returning a reference to a local temporary object.

	if (bootExeData) {
		// The boot executable is already open.
		return bootExeData;
	}

	if (!isoPartition || !isoPartition->isOpen()) {
		// ISO partition is not open.
		return nullptr;
	}

	if (boot_filename.empty()) {
		// No boot filename...
		return nullptr;
	}

	// Open the boot file.
	// TODO: Do we need a leading slash?
	const IRpFilePtr f_bootExe(isoPartition->open(boot_filename.c_str()));
	if (!f_bootExe) {
		// Unable to open the default executable.
		return nullptr;
	}

	RomDataPtr exeData;
	switch (consoleType) {
		case ConsoleType::PS1: {
			// Check if we have a STACK override in SYSTEM.CNF.
			uint32_t sp_override = 0;
			auto iter = system_cnf.find("STACK");
			if (iter != system_cnf.end() && !iter->second.empty()) {
				// Validate the value.
				char *endptr = nullptr;
				sp_override = strtoul(iter->second.c_str(), &endptr, 16);
				if (*endptr != '\0') {
					sp_override = 0;
				}
			}
			exeData = std::make_shared<PlayStationEXE>(f_bootExe, sp_override);
			break;
		}
		case ConsoleType::PS2:
			exeData = std::make_shared<ELF>(f_bootExe);
			break;
		default:
			assert(!"Console type not supported.");
			break;
	}

	if (exeData && exeData->isValid()) {
		// Boot executable is open and valid.
		bootExeData = exeData;
		return exeData;
	}

	// Unable to open the executable.
	return nullptr;
}

/** PlayStationDisc **/

/**
 * Read a Sony PlayStation 1 or 2 disc image.
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
PlayStationDisc::PlayStationDisc(const IRpFilePtr &file)
	: super(new PlayStationDiscPrivate(file))
{
	// This class handles disc images.
	RP_D(PlayStationDisc);
	d->mimeType = "application/x-cd-image";	// unofficial
	d->fileType = FileType::DiscImage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	IDiscReaderPtr discReader;

	// Check for a PVD with 2048-byte sectors.
	size_t size = d->file->seekAndRead(ISO_PVD_ADDRESS_2048, &d->pvd, sizeof(d->pvd));
	if (size != sizeof(d->pvd)) {
		d->file.reset();
		return;
	}
	if (ISO::checkPVD(reinterpret_cast<const uint8_t*>(&d->pvd)) >= 0) {
		// Disc has 2048-byte sectors.
		discReader = std::make_shared<DiscReader>(d->file);
	} else {
		// Check for a PVD with 2352-byte or 2448-byte sectors.
		static const unsigned int sector_sizes[] = {2352, 2448};
		CDROM_2352_Sector_t sector;

		for (const unsigned int p : sector_sizes) {
			size_t size = d->file->seekAndRead(p * ISO_PVD_LBA, &sector, sizeof(sector));
			if (size != sizeof(sector)) {
				d->file.reset();
				return;
			}

			const uint8_t *const pData = cdromSectorDataPtr(&sector);
			if (ISO::checkPVD(pData) >= 0) {
				// Found the correct sector size.
				memcpy(&d->pvd, pData, sizeof(d->pvd));
				discReader = std::make_shared<Cdrom2352Reader>(d->file, p);
				break;
			}
		}
	}

	if (!discReader || !discReader->isOpen()) {
		// Error opening the DiscReader.
		d->file.reset();
		return;
	}

	// Try to open the ISO partition.
	IsoPartitionPtr isoPartition = std::make_shared<IsoPartition>(discReader, 0, 0);
	if (!isoPartition->isOpen()) {
		// Error opening the ISO partition.
		d->file.reset();
		return;
	}

	// ISO-9660 partition is open.
	// Load SYSTEM.CNF.
	int ret = d->loadSystemCnf(isoPartition);
	if (ret != 0) {
		// Error loading SYSTEM.CNF.
		d->file.reset();
		return;
	}

	// Check if we have a boot filename.
	PlayStationDiscPrivate::ConsoleType consoleType = PlayStationDiscPrivate::ConsoleType::Unknown;
	auto iter = d->system_cnf.find("BOOT2");
	if (iter != d->system_cnf.end()) {
		// Found BOOT2. (PS2)
		consoleType = PlayStationDiscPrivate::ConsoleType::PS2;
	} else {
		iter = d->system_cnf.find("BOOT");
		if (iter != d->system_cnf.end()) {
			// Found BOOT. (PS1)
			consoleType = PlayStationDiscPrivate::ConsoleType::PS1;
		} else {
			// Not valid.
			d->file.reset();
			return;
		}
	}

	// Boot filename should start with:
	// - cdrom: (PS1)
	// - cdrom0: (PS2)
	// There's usually a backslash after the colon, but some
	// prototypes don't have it.
	const auto &bf_str = iter->second;
	if (!bf_str.empty()) {
		size_t pos = 0;
		if (!strncasecmp(bf_str.c_str(), "cdrom", 5)) {
			pos = 5;
			if (bf_str[pos] == '0') {
				// "cdrom0"
				pos++;
			}
			if (bf_str[pos] == ':') {
				// "cdrom:" / "cdrom0:"
				pos++;
				// Remove one or more backslashes.
				// "cdrom:\\", "cdrom0:\\"
				while (bf_str[pos] == '\\') {
					pos++;
				}
			}
		}
		d->boot_filename = bf_str.substr(pos);
	}
	if (d->boot_filename.empty()) {
		// No boot filename specified.
		// Use the console-specific default.
		switch (consoleType) {
			default:
			case PlayStationDiscPrivate::ConsoleType::PS1:
				d->boot_filename = "PSX.EXE";
				break;
			case PlayStationDiscPrivate::ConsoleType::PS2:
				// TODO: Default PS2 boot filename?
				break;
		}
	}

	// Check if there is a space.
	const size_t pos = bf_str.find(' ');
	if (pos != string::npos && pos > 0) {
		// Found a space.
		// Everything after the space is a boot argument.
		d->boot_argument = d->boot_filename.substr(pos+1);
		d->boot_filename.resize(pos-1);
	}

	// Remove the ISO version number.
	const size_t len = d->boot_filename.size();
	if (len > 2) {
		if (ISDIGIT(d->boot_filename[len-1]) && d->boot_filename[len-2] == ';') {
			d->boot_filename.resize(len-2);
		}
	}

	// Disc image is ready.
	d->consoleType = consoleType;
	d->discReader = std::move(discReader);
	d->isoPartition = std::move(isoPartition);
	d->isValid = true;
}

/**
 * Close the opened file.
 */
void PlayStationDisc::close(void)
{
	RP_D(PlayStationDisc);

	// NOTE: Don't delete these. They have rp_image objects
	// that may be used by the UI later.
	if (d->bootExeData) {
		d->bootExeData->close();
	}

	d->isoPartition.reset();
	d->discReader.reset();

	// Call the superclass function.
	super::close();
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int PlayStationDisc::isRomSupported_static(const DetectInfo *info)
{
	// NOTE: This version is NOT supported for PlayStationDisc.
	// Use the ISO-9660 PVD check instead.
	RP_UNUSED(info);
	assert(!"Use the ISO-9660 PVD check instead.");
	return -1;
}

/**
 * Is a ROM image supported by this class?
 * @param pvd ISO-9660 Primary Volume Descriptor.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int PlayStationDisc::isRomSupported_static(
	const ISO_Primary_Volume_Descriptor *pvd)
{
	assert(pvd != nullptr);
	if (!pvd) {
		// Bad.
		return -1;
	}

	// PlayStation 1 and 2 discs have the system ID "PLAYSTATION".
	// NOTE: Some PS2 prototypes have incorrect system IDs. We'll
	// check for those here, and then verify SYSTEM.CNF later.
	int pos = -1;
	if (!strncmp(pvd->sysID, "PLAYSTATION ", 12)) {
		pos = 12;
	} else if (!strncmp(pvd->sysID, "CD-RTOS CD-BRIDGE ", 18)) {
		// CD-i system ID
		// Some PS2 prototypes have this for some reason.
		pos = 18;
	} else if (!strncmp(pvd->sysID, "Win32 ", 6)) {
		// No idea why some PS2 prototypes have this one...
		pos = 6;
	}

	if (pos < 0) {
		// Not valid.
		return -1;
	}

	// Make sure the rest of the system ID is either spaces or NULLs.
	bool isOK = true;
	const char *const p_end = &pvd->sysID[sizeof(pvd->sysID)];
	for (const char *p = &pvd->sysID[pos]; p < p_end; p++) {
		if (*p != ' ' && *p != '\0') {
			isOK = false;
			break;
		}
	}

	if (isOK) {
		// Valid PVD.
		// Caller will need to check for the sector size.
		return 0;
	}

	// Not a PlayStation 1 or 2 disc.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *PlayStationDisc::systemName(unsigned int type) const
{
	RP_D(const PlayStationDisc);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// PlayStationDisc has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"PlayStationDisc::systemName() array index optimization needs to be updated.");

	switch (d->consoleType) {
		default:
		case PlayStationDiscPrivate::ConsoleType::PS1: {
			static const char *const sysNames_PS1[4] = {
				"Sony PlayStation", "PlayStation", "PS1", nullptr
			};
			return sysNames_PS1[type & SYSNAME_TYPE_MASK];
		}

		case PlayStationDiscPrivate::ConsoleType::PS2: {
			static const char *const sysNames_PS2[4] = {
				"Sony PlayStation 2", "PlayStation 2", "PS2", nullptr
			};
			return sysNames_PS2[type & SYSNAME_TYPE_MASK];
		}
	}

	// Should not get here...
	assert(!"PlayStationDisc::systemName(): Invalid system name.");
	return nullptr;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t PlayStationDisc::supportedImageTypes_static(void)
{
#ifdef HAVE_JPEG
	return IMGBF_EXT_COVER;
#else /* !HAVE_JPEG */
	return 0;
#endif /* HAVE_JPEG */
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t PlayStationDisc::supportedImageTypes(void) const
{
	return supportedImageTypes_static();
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
vector<RomData::ImageSizeDef> PlayStationDisc::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	// NOTE: Can't check for system type here.
	// Assuming PS1 disc images.
	switch (imageType) {
#ifdef HAVE_JPEG
		case IMG_EXT_COVER:
			return {{nullptr, 500, 500, 0}};
#endif /* HAVE_JPEG */
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
vector<RomData::ImageSizeDef> PlayStationDisc::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
#ifdef HAVE_JPEG
		case IMG_EXT_COVER: {
			RP_D(const PlayStationDisc);
			switch (d->consoleType) {
				default:
					assert(!"Invalid ConsoleType.");
					break;
				case PlayStationDiscPrivate::ConsoleType::PS1:
					return {{nullptr, 500, 500, 0}};
				case PlayStationDiscPrivate::ConsoleType::PS2:
					return {{nullptr, 512, 736, 0}};
			}
			break;
		}
#endif /* HAVE_JPEG */
		default:
			break;
	}

	// Unsupported image type.
	return {};
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int PlayStationDisc::loadFieldData(void)
{
	RP_D(PlayStationDisc);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown disc type.
		return -EIO;
	}

	d->fields.reserve(6);	// Maximum of 6 fields.

	const char *s_tab_name;
	switch (d->consoleType) {
		default:
		case PlayStationDiscPrivate::ConsoleType::PS1:
			s_tab_name = "PS1";
			break;
		case PlayStationDiscPrivate::ConsoleType::PS2:
			s_tab_name = "PS2";
			break;
	}
	d->fields.setTabName(0, s_tab_name);

	// Boot filename
	d->fields.addField_string(C_("PlayStationDisc", "Boot Filename"), d->boot_filename);

	// Boot argument, if present
	if (!d->boot_argument.empty()) {
		d->fields.addField_string(C_("PlayStationDisc", "Boot Argument"), d->boot_argument);
	}

	// Console-specific fields
	switch (d->consoleType) {
		default:
		case PlayStationDiscPrivate::ConsoleType::PS1: {
			// Max thread count
			auto iter = d->system_cnf.find("TCB");
			if (iter != d->system_cnf.end() && !iter->second.empty()) {
				d->fields.addField_string(C_("PlayStationDisc", "Max Thread Count"), iter->second);
			}

			// Max event count
			iter = d->system_cnf.find("EVENT");
			if (iter != d->system_cnf.end() && !iter->second.empty()) {
				d->fields.addField_string(C_("PlayStationDisc", "Max Event Count"), iter->second);
			}
			break;
		}

		case PlayStationDiscPrivate::ConsoleType::PS2: {
			// Version
			auto iter = d->system_cnf.find("VER");
			if (iter != d->system_cnf.end() && !iter->second.empty()) {
				d->fields.addField_string(C_("PlayStationDisc", "Version"), iter->second);
			}

			// Video mode
			// TODO: Validate this?
			iter = d->system_cnf.find("VMODE");
			if (iter != d->system_cnf.end() && !iter->second.empty()) {
				d->fields.addField_string(C_("PlayStationDisc", "Video Mode"), iter->second);
			}
			break;
		}
	}

	// Boot file timestamp
	time_t boot_file_timestamp = -1;
	if (!d->boot_filename.empty()) {
		// TODO: Do we need a leading slash?
		boot_file_timestamp = d->isoPartition->get_mtime(d->boot_filename.c_str());
	}
	d->fields.addField_dateTime(C_("PlayStationDisc", "Boot File Time"),
		boot_file_timestamp,
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME);

	// Show a tab for the boot file.
	const RomDataPtr bootExeData = d->openBootExe();
	if (bootExeData) {
		// Add the fields.
		// NOTE: Adding tabs manually so we can show the disc info in
		// the primary tab.
		const RomFields *const exeFields = bootExeData->fields();
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

	// Check for CDVDGEN.
	PS2_CDVDGEN_t cdvdgen;
	size_t size = d->discReader->seekAndRead(
		PS2_CDVDGEN_LBA * ISO_SECTOR_SIZE_MODE1_COOKED, &cdvdgen, sizeof(cdvdgen));
	if (size == sizeof(cdvdgen) && !memcmp(cdvdgen.sw_version, "CDVDGEN ", 8)) {
		// CDVDGEN data found.
		d->fields.addTab("CDVDGEN");
		d->fields.reserve(d->fields.count() + 9);

		// CDVDGEN version
		d->fields.addField_string(C_("PlayStationDisc", "CDVDGEN Version"),
			cp1252_to_utf8(&cdvdgen.sw_version[8], sizeof(cdvdgen.disc_name)-8),
			RomFields::STRF_TRIM_END);

		// Disc name
		d->fields.addField_string(C_("PlayStationDisc", "Disc Name"),
			cp1252_to_utf8(cdvdgen.disc_name, sizeof(cdvdgen.disc_name)),
			RomFields::STRF_TRIM_END);

		// Producer name
		d->fields.addField_string(C_("PlayStationDisc", "Producer Name"),
			cp1252_to_utf8(cdvdgen.producer_name, sizeof(cdvdgen.producer_name)),
			RomFields::STRF_TRIM_END);

		// Copyright holder
		d->fields.addField_string(C_("PlayStationDisc", "Copyright Holder"),
			cp1252_to_utf8(cdvdgen.copyright_holder, sizeof(cdvdgen.copyright_holder)),
			RomFields::STRF_TRIM_END);

		// Creation date
		// NOTE: Marking as UTC because there's no timezone information.
		d->fields.addField_dateTime(C_("PlayStationDisc", "Creation Date"),
			d->ascii_yyyymmdd_to_unix_time(cdvdgen.creation_date),
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_IS_UTC);

		// TODO: Show the master disc ID?

		// Disc drive information
		// TODO: Hide if empty?
		d->fields.addField_string(C_("PlayStationDisc", "Drive Vendor"),
			cp1252_to_utf8(cdvdgen.drive.vendor, sizeof(cdvdgen.drive.vendor)),
			RomFields::STRF_TRIM_END);
		d->fields.addField_string(C_("PlayStationDisc", "Drive Model"),
			cp1252_to_utf8(cdvdgen.drive.model, sizeof(cdvdgen.drive.model)),
			RomFields::STRF_TRIM_END);
		d->fields.addField_string(C_("PlayStationDisc", "Drive Firmware"),
			cp1252_to_utf8(cdvdgen.drive.revision, sizeof(cdvdgen.drive.revision)),
			RomFields::STRF_TRIM_END);
		d->fields.addField_string(C_("PlayStationDisc", "Drive Notes"),
			cp1252_to_utf8(cdvdgen.drive.notes, sizeof(cdvdgen.drive.notes)),
			RomFields::STRF_TRIM_END);
	}

	// ISO object for ISO-9660 PVD
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

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int PlayStationDisc::loadMetaData(void)
{
	RP_D(PlayStationDisc);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->isValid) {
		// Unknown disc image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(3);	// Maximum of 3 metadata properties.

	// Add the PVD metadata.
	// TODO: PlayStationDisc-specific metadata?
	ISO::addMetaData_PVD(d->metaData, &d->pvd);

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
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
int PlayStationDisc::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	RP_D(const PlayStationDisc);
	if (!d->isValid || (int)d->consoleType < 0) {
		// Disc image isn't valid.
		return -EIO;
	}

	// Get the disc serial number. (boot filename)
	if (d->boot_filename.empty() || d->boot_filename == "PSX.EXE") {
		// No boot filename, or it's the default filename.
		return -ENOENT;
	}

	// System ID
	static const char sys_tbl[][4] = {
		"ps1", "ps2"
	};
	if (d->consoleType >= PlayStationDiscPrivate::ConsoleType::Max)
		return -ENOENT;
	const char *const sys = sys_tbl[(int)d->consoleType];

	// Game ID format: SLUS-20718
	// Boot filename format: SLUS_207.18
	// Using the first part as the region code.
	string gameID = d->boot_filename;
	std::transform(gameID.begin(), gameID.end(), gameID.begin(), ::toupper);
	string region_code;
	size_t pos = gameID.find('_');
	if (pos != string::npos) {
		region_code = gameID.substr(0, pos);
		gameID[pos] = '-';
	} else if (d->boot_filename.size() > 4) {
		// No underscore. Use the first four characters as the region.
		region_code = gameID.substr(0, 4);
	} else {
		// Too short to be a game ID.
		return -ENOENT;
	}

	// Remove the dot.
	pos = gameID.rfind('.');
	if (pos != string::npos) {
		gameID.erase(pos, 1);
	}

	// NOTE: We only have one size for MegaDrive right now.
	// TODO: Determine the actual image size.
	RP_UNUSED(size);
	vector<ImageSizeDef> sizeDefs = supportedImageSizes(imageType);
	assert(sizeDefs.size() == 1);
	if (sizeDefs.empty()) {
		// No image sizes.
		return -ENOENT;
	}

	// NOTE: RPDB's cover database only has one size.
	// There's no need to check image sizes, but we need to
	// get the image size for the extURLs struct.

	// Determine the image type name.
	const char *imageTypeName;
	const char *ext;
	switch (imageType) {
#ifdef HAVE_JPEG
		case IMG_EXT_COVER:
			imageTypeName = "cover";
			ext = ".jpg";
			break;
#endif /* HAVE_JPEG */
		default:
			// Unsupported image type.
			return -ENOENT;
	}

	// Add the URLs.
	pExtURLs->resize(1);
	auto extURL_iter = pExtURLs->begin();
	extURL_iter->url = d->getURL_RPDB(sys, imageTypeName, region_code.c_str(), gameID.c_str(), ext);
	extURL_iter->cache_key = d->getCacheKey_RPDB(sys, imageTypeName, region_code.c_str(), gameID.c_str(), ext);
	extURL_iter->width = sizeDefs[0].width;
	extURL_iter->height = sizeDefs[0].height;
	extURL_iter->high_res = (sizeDefs[0].index >= 2);

	// All URLs added.
	return 0;
}

}
