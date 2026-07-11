/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IRpFile_unzFile_filefuncs.hpp: IRpFile filefuncs for MiniZip-NG.        *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// MiniZip-NG
#include "compat/unzip.h"

// librpfile
#include "librpfile/IRpFile.hpp"

namespace LibRomData { namespace IRpFile_unzFile_filefuncs {

/**
 * Open a ZIP file for reading using an IRpFile.
 * @param file IRpFilePtr
 * @return unzFile, or nullptr on error.
 */
unzFile unzOpen2_64_IRpFile(const LibRpFile::IRpFilePtr &file);

} } // namespace LibRomData::IRpFile_unzFile_filefuncs
