/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IRpFile_unzFile_filefuncs.hpp: IRpFile filefuncs for MiniZip-NG.        *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// MiniZip-NG
#include "compat/ioapi.h"
#include "compat/unzip.h"

// librpfile
#include "librpfile/IRpFile.hpp"

namespace LibRomData { namespace IRpFile_unzFile_filefuncs {

// NOTE: Only implementing the LFS (64-bit) functions.

/**
 * Fill in filefuncs for IRpFile.
 * When using IRpFile filefuncs, specify a pointer to IRpFilePtr as the "filename".
 * @param pzlib_filefunc_def
 */
void fill_IRpFile_filefunc64(zlib_filefunc64_def *pzlib_filefunc_def);

/**
 * Open a ZIP file for reading using an IRpFile.
 * @param file IRpFilePtr
 * @return unzFile, or nullptr on error.
 */
unzFile unzOpen2_64_IRpFile(const LibRpFile::IRpFilePtr &file);

} }
