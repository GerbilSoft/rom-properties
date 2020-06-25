/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PlayStationDisc.cpp: PlayStation 1 and 2 disc image reader.             *
 *                                                                         *
 * Copyright (c) 2019-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "PlayStationDisc.hpp"
#include "ps2_structs.h"

// librpbase, librpfile
#include "librpfile/RpFile.hpp"
using namespace LibRpBase;
using LibRpFile::IRpFile;
using LibRpFile::RpFile;

// IsoPartition
#include "../cdrom_structs.h"
#include "../iso_structs.h"
#include "../disc/Cdrom2352Reader.hpp"
#include "../disc/IsoPartition.hpp"

// Other RomData subclasses
#include "Other/ISO.hpp"
#include "Console/PlayStationEXE.hpp"
#include "Other/ELF.hpp"

// inih for system.cnf
#include "ini.h"

// C++ STL classes.
using std::string;
using std::unordered_map;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(PlayStationDisc)

class PlayStationDiscPrivate : public LibRpBase::RomDataPrivate
{
	public:
		PlayStationDiscPrivate(PlayStationDisc *q, LibRpFile::IRpFile *file);
		virtual ~PlayStationDiscPrivate();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(PlayStationDiscPrivate)

	public:
		// Disc type.
		enum class DiscType {
			Unknown	= -1,

			Iso2048	= 0,	// CD (2048-byte scetors) or DVD
			Iso2352	= 1,	// CD (2352-byte sectors)

			Max
		};
		DiscType discType;

		ISO_Primary_Volume_Descriptor pvd;

		// system.cnf contents
		// Keys are stored in upper-case.
		unordered_map<string, string> system_cnf;

		/**
		 * ini.h callback for parsing system.cnf.
		 * @param user		[in] User data parameter (this)
		 * @param section	[in] Section name
		 * @param name		[in] Value name
		 * @param value		[in] Value
		 * @return 0 to continue; 1 to stop.
		 */
		static int INIHCALL parse_system_cnf(void *user, const char *section, const char *name, const char *value);

		/**
		 * Load system.cnf.
		 * @param pt IPartition containing system.cnf.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadSystemCnf(IsoPartition *pt);

		// IsoPartition
		IDiscReader *discReader;
		IsoPartition *isoPartition;

		// Boot executable
		RomData *bootExeData;

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
		RomData *openBootExe(void);

		enum class ConsoleType {
			Unknown	= -1,

			PS1	= 0,	// PlayStation
			PS2	= 1,	// PlayStation 2

			Max
		};
		ConsoleType consoleType;
};

/** PlayStationDiscPrivate **/

PlayStationDiscPrivate::PlayStationDiscPrivate(PlayStationDisc *q, IRpFile *file)
	: super(q, file)
	, discType(DiscType::Unknown)
	, discReader(nullptr)
	, isoPartition(nullptr)
	, bootExeData(nullptr)
	, consoleType(ConsoleType::Unknown)
{
	// Clear the structs.
	memset(&pvd, 0, sizeof(pvd));
}

PlayStationDiscPrivate::~PlayStationDiscPrivate()
{
	UNREF(bootExeData);
	UNREF(isoPartition);
	UNREF(discReader);
}

/**
 * ini.h callback for parsing system.cnf.
 * @param user		[in] User data parameter (this)
 * @param section	[in] Section name
 * @param name		[in] Value name
 * @param value		[in] Value
 * @return 0 to continue; 1 to stop.
 */
int INIHCALL PlayStationDiscPrivate::parse_system_cnf(void *user, const char *section, const char *name, const char *value)
{
	if (section[0] != '\0') {
		// Sections aren't expected here...
		return 1;
	}

	// Save the value for later.
	string s_name(name);
	std::transform(s_name.begin(), s_name.end(), s_name.begin(),
		[](unsigned char c) { return std::toupper(c); });

	PlayStationDiscPrivate *const d = static_cast<PlayStationDiscPrivate*>(user);
	auto ret = d->system_cnf.emplace(std::make_pair(std::move(s_name), value));
	return (ret.second ? 0 : 1);
}

/**
 * Load system.cnf.
 * @param pt IPartition containing system.cnf.
 * @return 0 on success; negative POSIX error code on error.
 */
int PlayStationDiscPrivate::loadSystemCnf(IsoPartition *pt)
{
	IRpFile *f_system_cnf = pt->open("system.cnf");
	if (!f_system_cnf) {
		int ret = -pt->lastError();
		if (ret == 0) {
			ret = -EIO;
		}
		return ret;
	} else if (!f_system_cnf->isOpen()) {
		int ret = -f_system_cnf->lastError();
		if (ret == 0) {
			ret = -EIO;
		}
		f_system_cnf->unref();
		return ret;
	}

	// CNF file should be less than 2048 bytes.
	const int64_t fileSize = f_system_cnf->size();
	if (fileSize > 2048) {
		f_system_cnf->unref();
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
RomData *PlayStationDiscPrivate::openBootExe(void)
{
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
	IRpFile *f_bootExe = isoPartition->open(boot_filename.c_str());
	if (f_bootExe) {
		RomData *exeData = nullptr;
		switch (consoleType) {
			case ConsoleType::PS1: {
				// Check if we have a STACK override in system.cnf.
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
				exeData = new PlayStationEXE(f_bootExe, sp_override);
				break;
			}
			case ConsoleType::PS2:
				exeData = new ELF(f_bootExe);
				break;
			default:
				assert(!"Console type not supported.");
				break;
		}
		f_bootExe->unref();
		if (exeData && exeData->isValid()) {
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
PlayStationDisc::PlayStationDisc(IRpFile *file)
	: super(new PlayStationDiscPrivate(this, file))
{
	// This class handles disc images.
	RP_D(PlayStationDisc);
	d->className = "PlayStationDisc";
	d->mimeType = "application/x-cd-image";	// unofficial
	d->fileType = FileType::DiscImage;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	IDiscReader *discReader = nullptr;
	PlayStationDiscPrivate::DiscType discType = PlayStationDiscPrivate::DiscType::Unknown;

	// Check for a PVD with 2048-byte sectors.
	size_t size = d->file->seekAndRead(ISO_PVD_ADDRESS_2048, &d->pvd, sizeof(d->pvd));
	if (size != sizeof(d->pvd)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}
	if (ISO::checkPVD(reinterpret_cast<const uint8_t*>(&d->pvd)) >= 0) {
		// Disc has 2048-byte sectors.
		discType = PlayStationDiscPrivate::DiscType::Iso2048;
		discReader = new DiscReader(d->file);
	} else {
		// Check for a PVD with 2352-byte sectors.
		CDROM_2352_Sector_t sector;
		size_t size = d->file->seekAndRead(ISO_PVD_ADDRESS_2352, &sector, sizeof(sector));
		if (size != sizeof(sector)) {
			UNREF_AND_NULL_NOCHK(d->file);
			return;
		}

		const uint8_t *const pData = cdromSectorDataPtr(&sector);
		if (ISO::checkPVD(sector.m2xa_f1.data) >= 0) {
			// Disc has 2352-byte sectors.
			memcpy(&d->pvd, pData, sizeof(d->pvd));
			discType = PlayStationDiscPrivate::DiscType::Iso2352;
			discReader = new Cdrom2352Reader(d->file);
		} else {
			// Valid PVD not found.
			UNREF_AND_NULL_NOCHK(d->file);
			return;
		}
	}

	if (!discReader || !discReader->isOpen()) {
		// Error opening the DiscReader.
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
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// ISO-9660 partition is open.
	// Load system.cnf.
	int ret = d->loadSystemCnf(isoPartition);
	if (ret != 0) {
		// Error loading system.cnf.
		UNREF(isoPartition);
		UNREF(discReader);
		UNREF_AND_NULL_NOCHK(d->file);
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
			UNREF(isoPartition);
			UNREF(discReader);
			UNREF_AND_NULL_NOCHK(d->file);
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
				if (bf_str[pos] == '\\') {
					// "cdrom:\\" / "cdrom0:\\"
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
	size_t pos = bf_str.find(' ');
	if (pos != string::npos && pos > 0) {
		// Found a space.
		// Everything after the space is a boot argument.
		d->boot_argument = d->boot_filename.substr(pos+1);
		d->boot_filename.resize(pos-1);
	}

	// Remove the ISO version number.
	size_t len = d->boot_filename.size();
	if (len > 2) {
		if (ISDIGIT(d->boot_filename[len-1]) && d->boot_filename[len-2] == ';') {
			d->boot_filename.resize(len-2);
		}
	}

	// Disc image is ready.
	d->discType = discType;
	d->consoleType = consoleType;
	d->discReader = discReader;
	d->isoPartition = isoPartition;
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
		return static_cast<int>(PlayStationDiscPrivate::DiscType::Unknown);
	}

	// PlayStation 1 and 2 discs have the system ID "PLAYSTATION".
	// NOTE: Some PS2 prototypes have "CD-RTOS CD-BRIDGE", which is
	// normally used for CD-i discs. We'll verify that system.cnf
	// is present regardless.
	int pos = -1;
	if (!strncmp(pvd->sysID, "PLAYSTATION ", 12)) {
		pos = 12;
	} else if (!strncmp(pvd->sysID, "CD-RTOS CD-BRIDGE ", 18)) {
		pos = 18;
	}

	if (pos < 0) {
		// Not valid.
		return static_cast<int>(PlayStationDiscPrivate::DiscType::Unknown);
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
		// Caller will need to check for 2048 vs. 2352.
		return 0;
	}

	// Not a PlayStation 1 or 2 disc.
	return static_cast<int>(PlayStationDiscPrivate::DiscType::Unknown);
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
const char *const *PlayStationDisc::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".iso",		// ISO
		".bin",		// BIN/CUE
		".img",		// CCD/IMG
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
const char *const *PlayStationDisc::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types from FreeDesktop.org..
		"application/x-cd-image",
		"application/x-iso9660-image",

		// TODO: PS1/PS2?
		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int PlayStationDisc::loadFieldData(void)
{
	RP_D(PlayStationDisc);
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
	d->fields->setTabName(0, s_tab_name);

	// Boot filename
	d->fields->addField_string(C_("PlayStationDisc", "Boot Filename"), d->boot_filename);

	// Boot argument, if present
	if (!d->boot_argument.empty()) {
		d->fields->addField_string(C_("PlayStationDisc", "Boot Argument"), d->boot_argument);
	}

	// Console-specific fields
	switch (d->consoleType) {
		default:
		case PlayStationDiscPrivate::ConsoleType::PS1: {
			// Max thread count
			auto iter = d->system_cnf.find("TCB");
			if (iter != d->system_cnf.end() && !iter->second.empty()) {
				d->fields->addField_string(C_("PlayStationDisc", "Max Thread Count"), iter->second);
			}

			// Max event count
			iter = d->system_cnf.find("EVENT");
			if (iter != d->system_cnf.end() && !iter->second.empty()) {
				d->fields->addField_string(C_("PlayStationDisc", "Max Event Count"), iter->second);
			}
			break;
		}

		case PlayStationDiscPrivate::ConsoleType::PS2: {
			// Version
			auto iter = d->system_cnf.find("VER");
			if (iter != d->system_cnf.end() && !iter->second.empty()) {
				d->fields->addField_string(C_("PlayStationDisc", "Version"), iter->second);
			}

			// Video mode
			// TODO: Validate this?
			iter = d->system_cnf.find("VMODE");
			if (iter != d->system_cnf.end() && !iter->second.empty()) {
				d->fields->addField_string(C_("PlayStationDisc", "Video Mode"), iter->second);
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
	d->fields->addField_dateTime(C_("PlayStationDisc", "Boot File Time"),
		boot_file_timestamp,
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME);

	// Show a tab for the boot file.
	RomData *const bootExeData = d->openBootExe();
	if (bootExeData) {
		// Add the fields.
		// NOTE: Adding tabs manually so we can show the disc info in
		// the primary tab.
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

	// Check for CDVDGEN.
	PS2_CDVDGEN_t cdvdgen;
	size_t size = d->discReader->seekAndRead(
		PS2_CDVDGEN_LBA * ISO_SECTOR_SIZE_MODE1_COOKED, &cdvdgen, sizeof(cdvdgen));
	if (size == sizeof(cdvdgen) && !memcmp(cdvdgen.sw_version, "CDVDGEN ", 8)) {
		// CDVDGEN data found.
		d->fields->addTab("CDVDGEN");
		d->fields->reserve(d->fields->count() + 9);

		// CDVDGEN version
		d->fields->addField_string(C_("PlayStationDisc", "CDVDGEN Version"),
			cp1252_to_utf8(&cdvdgen.sw_version[8], sizeof(cdvdgen.disc_name)-8),
			RomFields::STRF_TRIM_END);

		// Disc name
		d->fields->addField_string(C_("PlayStationDisc", "Disc Name"),
			cp1252_to_utf8(cdvdgen.disc_name, sizeof(cdvdgen.disc_name)),
			RomFields::STRF_TRIM_END);

		// Producer name
		d->fields->addField_string(C_("PlayStationDisc", "Producer Name"),
			cp1252_to_utf8(cdvdgen.producer_name, sizeof(cdvdgen.producer_name)),
			RomFields::STRF_TRIM_END);

		// Copyright holder
		d->fields->addField_string(C_("PlayStationDisc", "Copyright Holder"),
			cp1252_to_utf8(cdvdgen.copyright_holder, sizeof(cdvdgen.copyright_holder)),
			RomFields::STRF_TRIM_END);

		// Creation date
		// NOTE: Marking as UTC because there's no timezone information.
		d->fields->addField_dateTime(C_("PlayStationDisc", "Creation Date"),
			d->ascii_yyyymmdd_to_unix_time(cdvdgen.creation_date),
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_IS_UTC);

		// TODO: Show the master disc ID?

		// Disc drive information
		// TODO: Hide if empty?
		d->fields->addField_string(C_("PlayStationDisc", "Drive Vendor"),
			cp1252_to_utf8(cdvdgen.drive.vendor, sizeof(cdvdgen.drive.vendor)),
			RomFields::STRF_TRIM_END);
		d->fields->addField_string(C_("PlayStationDisc", "Drive Model"),
			cp1252_to_utf8(cdvdgen.drive.model, sizeof(cdvdgen.drive.model)),
			RomFields::STRF_TRIM_END);
		d->fields->addField_string(C_("PlayStationDisc", "Drive Firmware"),
			cp1252_to_utf8(cdvdgen.drive.revision, sizeof(cdvdgen.drive.revision)),
			RomFields::STRF_TRIM_END);
		d->fields->addField_string(C_("PlayStationDisc", "Drive Notes"),
			cp1252_to_utf8(cdvdgen.drive.notes, sizeof(cdvdgen.drive.notes)),
			RomFields::STRF_TRIM_END);
	}

	// ISO object for ISO-9660 PVD
	ISO *const isoData = new ISO(d->file);
	if (isoData->isOpen()) {
		// Add the fields.
		const RomFields *const isoFields = isoData->fields();
		if (isoFields) {
			d->fields->addFields_romFields(isoFields,
				RomFields::TabOffset_AddTabs);
		}
		isoData->unref();
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

}
