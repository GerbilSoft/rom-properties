/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AndroidCommon.hpp: Android common functions.                            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

namespace LibRpBase {
	class RomFields;
	class RomMetaData;
}

namespace pugi {
	class xml_document;
}

namespace LibRomData {

class AndroidResourceReader;

namespace AndroidCommon {

/**
 * Load field data.
 * @param fields	[out] RomFields
 * @param manifest_xml	[in] AndroidManifest.xml
 * @param arscReader	[in,opt] AndroidResourceReader, if available.
 * @return Number of fields added read on success; negative POSIX error code on error.
 */
int loadFieldData(LibRpBase::RomFields &fields, const pugi::xml_document &manifest_xml, const AndroidResourceReader *arscReader = nullptr);

/**
 * Load metadata properties.
 * @param metaData	[out] RomMetaData
 * @param manifest_xml	[in] AndroidManifest.xml
 * @param arscReader	[in,opt] AndroidResourceReader, if available.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int loadMetaData(LibRpBase::RomMetaData &metaData, const pugi::xml_document &manifest_xml, const AndroidResourceReader *arscReader = nullptr);

/**
 * Does the Android manifest have "dangerous" permissions?
 *
 * @param manifest_xml	[in] AndroidManifest.xml
 * @return True if the Android manifest has "dangerous" permissions; false if not.
 */
bool hasDangerousPermissions(const pugi::xml_document &manifest_xml);

} }
