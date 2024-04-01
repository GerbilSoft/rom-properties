/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IResourceReader.cpp: Interface for Windows resource readers.            *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "IResourceReader.hpp"

namespace LibRomData {

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

}
