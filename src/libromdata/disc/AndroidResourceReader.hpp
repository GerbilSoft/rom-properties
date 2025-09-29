/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AndroidResourceReader.hpp: Android resource reader.                     *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: This class does *not* derive from IDiscReader.

#pragma once

#include "../Handheld/n3ds_structs.h"

// librpbase
#include "librpbase/RomFields.hpp"

namespace LibRomData {

class AndroidResourceReaderPrivate;
class AndroidResourceReader
{
public:
	/**
	 * Construct an AndroidResourceReader with the specified IRpFile.
	 *
	 * NOTE: The specified memory buffer *must* remain valid while this
	 * AndroidResourceReader is open.
	 *
	 * @param pArsc		[in] resources.arsc
	 * @param arscLen	[in] Size of pArsc
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	AndroidResourceReader(const uint8_t *pArsc, size_t arscLen);
public:
	~AndroidResourceReader();

protected:
	friend class AndroidResourceReaderPrivate;
	AndroidResourceReaderPrivate *const d_ptr;

public:
	/**
	 * Is the resource data valid?
	 * @return True if valid; false if not.
	 */
	bool isValid(void) const;

	/**
	 * Parse a resource ID from AndroidManifest.xml, as loaded by the AndroidManifestXML class.
	 * @param str Resource ID (in format: "@0x12345678")
	 * @return Resource ID, or 0 if not valid
	 */
	static uint32_t parseResourceID(const char *str);

	/**
	 * Parse an unsigned integer value from AndroidManifest.xml, as loaded by the AndroidManifestXML class.
	 * @param str Unsigned integer value (as a decimal or hexadecimal string)
	 * @return Unsigned integer value, or 0 if not valid
	 */
	static uint32_t parseUnsignedInteger(const char *str);

	/**
	 * Get a string from Android resource data.
	 * @param id	[in] Resource ID
	 * @return String, or nullptr if not found.
	 */
	const char *getStringFromResource(uint32_t id) const;

	/**
	 * Add string field data to the specified RomFields object.
	 *
	 * If the string is in the format "@0x12345678", it will be loaded from
	 * resource.arsc, with RFT_STRING_MULTI.
	 *
	 * @param fields RomFields
	 * @param name Field name
	 * @param str String
	 * @param flags Formatting flags
	 * @return Field index, or -1 on error.
	 */
	int addField_string_i18n(LibRpBase::RomFields *fields, const char *name, const char *str, unsigned int flags = 0) const;

	/**
	 * Add string field data to the specified RomFields object.
	 *
	 * If the string is in the format "@0x12345678", it will be loaded from
	 * resource.arsc, with RFT_STRING_MULTI.
	 *
	 * @param fields RomFields
	 * @param name Field name
	 * @param str String
	 * @param flags Formatting flags
	 * @return Field index, or -1 on error.
	 */
	inline int addField_string_i18n(LibRpBase::RomFields *fields, const char *name, const std::string &str, unsigned int flags = 0) const
	{
		return addField_string_i18n(fields, name, str.c_str(), flags);
	}

	/**
	 * Find an icon filename with the highest density.
	 * @param resource_id Resource ID
	 * @return Icon filename, or empty string if not found.
	 */
	std::string findIconHighestDensity(uint32_t resource_id) const;
};

typedef std::shared_ptr<AndroidResourceReader> AndroidResourceReaderPtr;

}
