/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomDataTestObject.cpp: RomData test object for unit tests.              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RomDataTestObject.hpp"
#include "RomData_p.hpp"

// RomDataTestObject isn't used by libromdata directly,
// so use some linker hax to force linkage.
extern "C" {
	extern unsigned char RP_LibRpBase_RomDataTestObject_ForceLinkage;
	unsigned char RP_LibRpBase_RomDataTestObject_ForceLinkage;
}

// Other rom-properties libraries
using namespace LibRpFile;
//using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRpBase {

class RomDataTestObjectPrivate final : public RomDataPrivate
{
public:
	explicit RomDataTestObjectPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(RomDataTestObjectPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 0+1> exts;
	static const array<const char*, 0+1> mimeTypes;
	static const RomDataInfo romDataInfo;
};

ROMDATA_IMPL(RomDataTestObject)

/** RomDataTestObject **/

/* RomDataInfo */
const array<const char*, 0+1> RomDataTestObjectPrivate::exts = {{
	nullptr
}};
const array<const char*, 0+1> RomDataTestObjectPrivate::mimeTypes = {{
	nullptr
}};
const RomDataInfo RomDataTestObjectPrivate::romDataInfo = {
	"RomDataTestObject", exts.data(), mimeTypes.data()
};

RomDataTestObjectPrivate::RomDataTestObjectPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{ }

/** RomDataTestObject **/

/**
 * Dummy RomData subclass for unit tests.
 *
 * @param file Open ROM image.
 */
RomDataTestObject::RomDataTestObject(const IRpFilePtr &file)
	: super(new RomDataTestObjectPrivate(file))
{
	RP_D(RomDataTestObject);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	d->isValid = true;
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int RomDataTestObject::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	if (!info || !info->header.pData) {
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Dummy implementation. Always return 0.
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *RomDataTestObject::systemName(unsigned int type) const
{
	RP_D(const RomDataTestObject);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GBA has the same name worldwide, so we can
	// ignore the region selection.
	// TODO: Abbreviation might be different... (Japan uses AGB?)
	static_assert(SYSNAME_TYPE_MASK == 3,
		"RomDataTestObject::systemName() array index optimization needs to be updated.");

	static const array<const char*, 4> sysNames = {{
		"RomData Dummy Implementation for Unit Tests", "RomDataTestObject", "Dummy", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int RomDataTestObject::loadFieldData(void)
{
	// Not implemented.
	// Unit test must add fields manually by getting the RomFields object.
	RP_D(RomDataTestObject);
	return static_cast<int>(d->fields.count());
}

/** RomDataTestObject unit test functions **/

/**
 * Get a writable RomFields object for unit test purposes.
 * @return Writable RomFields object
 */
RomFields *RomDataTestObject::getWritableFields(void)
{
	RP_D(RomDataTestObject);
	return &d->fields;
}

} // namespace LibRpBase
