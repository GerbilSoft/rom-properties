/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IResourceReader.cpp: Interface for Windows resource readers.            *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "IResourceReader.hpp"

// C includes (C++ namespace)
#include <cassert>

namespace LibRpBase {

/**
 * DWORD alignment function.
 * @param file	[in] File to DWORD align.
 * @return 0 on success; non-zero on error.
 */
int IResourceReader::alignFileDWORD(IRpFile *file)
{
	int ret = 0;
	off64_t pos = file->tell();
	if (pos % 4 != 0) {
		pos = ALIGN_BYTES(4, pos);
		ret = file->seek(pos);
	}
	return ret;
}

/**
 * IPartition open() function.
 * We don't want to use this one.
 * @param filename Filename.
 * @return IRpFile*, or nullptr on error.
 */
LibRpFile::IRpFilePtr IResourceReader::open(const char *filename)
{
	RP_UNUSED(filename);
	assert(!"IPartition::open(const char*) should not be used for IResourceReader!");
	return {};
}

/**
 * Open a resource.
 * @param type	[in] Resource type ID [named]
 * @param id	[in] Resource ID [named] (nullptr for "first entry")
 * @param lang	[in] Language ID (-1 for "first entry")
 * @return IRpFile*, or nullptr on error.
 */
LibRpFile::IRpFilePtr IResourceReader::open(const char *type, const char *id, int lang)
{
	// Convert the names to IDs.
	// NOTE: Need to ensure the type ID is loaded after getting u_type.
	uint32_t u_type = nameToResourceID(type);
	if (u_type == 0) {
		// Type was not found...
		return {};
	}
	this->ensureTypeIDIsLoaded(u_type);

	uint32_t u_id = nameToResourceID(id);
	if (u_id == 0) {
		// ID was not found...
		return {};
	}

	return this->open(u_type, u_id, lang);
}

}
