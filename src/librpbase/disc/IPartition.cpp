/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IPartition.cpp: Partition reader interface.                             *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "IPartition.hpp"

// C includes (C++ namespace)
#include <cassert>

// Other rom-properties libraries
using LibRpFile::IRpFilePtr;

namespace LibRpBase {

/** IFst wrapper functions **/
// NOTE: May return nullptr if not implemented for a given class.

/**
 * Open a directory.
 * @param path	[in] Directory path.
 * @return IFst::Dir*, or nullptr on error.
 */
IFst::Dir *IPartition::opendir(const char *path)
{
	RP_UNUSED(path);
	assert(!"IFst wrapper functions are not implemented for this class!");
	return nullptr;
}

/**
 * Read a directory entry.
 * @param dirp IFst::Dir pointer.
 * @return IFst::DirEnt, or nullptr if end of directory or on error.
 * (TODO: Add lastError()?)
 */
IFst::DirEnt *IPartition::readdir(IFst::Dir *dirp)
{
	RP_UNUSED(dirp);
	assert(!"IFst wrapper functions are not implemented for this class!");
	return nullptr;
}

/**
 * Close an opened directory.
 * @param dirp IFst::Dir pointer.
 * @return 0 on success; negative POSIX error code on error.
 */
int IPartition::closedir(IFst::Dir *dirp)
{
	RP_UNUSED(dirp);
	assert(!"IFst wrapper functions are not implemented for this class!");
	return -ENOTSUP;
}

/**
 * Open a file. (read-only)
 * @param filename Filename.
 * @return IRpFile*, or nullptr on error.
 */
LibRpFile::IRpFilePtr IPartition::open(const char *filename)
{
	RP_UNUSED(filename);
	assert(!"IFst wrapper functions are not implemented for this class!");
	return {};
}

};
