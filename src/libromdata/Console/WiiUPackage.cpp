/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUPackage.cpp: Wii U NUS Package reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WiiUPackage.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::string;

namespace LibRomData {

class WiiUPackagePrivate final : public RomDataPrivate
{
public:
	WiiUPackagePrivate(const char *path);
	~WiiUPackagePrivate();

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(WiiUPackagePrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// Directory path (strdup()'d)
	char *path;
};

ROMDATA_IMPL(WiiUPackage)

/** WiiUPackagePrivate **/

/* RomDataInfo */
const char *const WiiUPackagePrivate::exts[] = {
	// No file extensions; NUS packages are directories.
	nullptr
};
const char *const WiiUPackagePrivate::mimeTypes[] = {
	// NUS packages are directories.
	"inode/directory",

	nullptr
};
const RomDataInfo WiiUPackagePrivate::romDataInfo = {
	"WiiUPackage", exts, mimeTypes
};

WiiUPackagePrivate::WiiUPackagePrivate(const char *path)
	: super({}, &romDataInfo)
{
	if (path && path[0] != '\0') {
		this->path = strdup(path);
	} else {
		this->path = nullptr;
	}
}

WiiUPackagePrivate::~WiiUPackagePrivate()
{
	free(path);
}

/** WiiUPackage **/

/**
 * Read a Wii U NUS package.
 *
 * NOTE: Wii U NUS packages are directories. This constructor
 * only accepts IRpFilePtr, so it isn't usable.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
WiiUPackage::WiiUPackage(const IRpFilePtr &file)
	: super(new WiiUPackagePrivate(nullptr))
{
	// Not supported!
	RP_UNUSED(file);
	return;
}

/**
 * Read a Wii U NUS package.
 *
 * NOTE: Wii U NUS packages are directories. This constructor
 * takes a local directory path.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param path Local directory path (UTF-8)
 */
WiiUPackage::WiiUPackage(const char *path)
	: super(new WiiUPackagePrivate(path))
{
	RP_D(WiiUPackage);
	d->fileType = FileType::ApplicationPackage;

	if (!d->path) {
		// No path specified...
		free(d->path);
		d->path = nullptr;
		return;
	}

	// Check if this path is supported.
	d->isValid = (isDirSupported_static(path) >= 0);

	if (!d->isValid) {
		free(d->path);
		d->path = nullptr;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiUPackage::isRomSupported_static(const DetectInfo *info)
{
	// Files are not supported.
	RP_UNUSED(info);
	return -1;
}

/**
 * Is a directory supported by this class?
 * @param path Directory to check.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiUPackage::isDirSupported_static(const char *path)
{
	assert(path != nullptr);
	assert(path[0] != '\0');
	if (!path || path[0] == '\0') {
		// No path specified.
		return -1;
	}

	string s_path(path);
	s_path += DIR_SEP_CHR;

	/// Check for the ticket, TMD, and certificate chain files.

	s_path += "title.tik";
	if (FileSystem::access(s_path.c_str(), R_OK) != 0) {
		// No ticket.
		return -1;
	}

	s_path.resize(s_path.size()-4);
	s_path += ".tmd";
	if (FileSystem::access(s_path.c_str(), R_OK) != 0) {
		// No TMD.
		return -1;
	}

	s_path.resize(s_path.size()-4);
	s_path += ".cert";
	if (FileSystem::access(s_path.c_str(), R_OK) != 0) {
		// No certificate chain.
		return -1;
	}

	// This appears to be a Wii U NUS package.
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *WiiUPackage::systemName(unsigned int type) const
{
	RP_D(const WiiUPackage);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// WiiUPackage has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"WiiUPackage::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Nintendo Wii U", "Wii U", "Wii U", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int WiiUPackage::loadFieldData(void)
{
	RP_D(WiiUPackage);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->path) {
		// No directory...
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// TODO

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

}
