/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RelatedFile.hpp: Open a related file.                                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

namespace LibRpFile {
	class IRpFile;
}

namespace LibRpFile { namespace FileSystem {

/**
 * Attempt to open a related file. (read-only)
 *
 * Related files are located in the same directory as the
 * primary file, but may have a different filename and/or
 * file extension.
 *
 * If the primary file is a symlink, the related file may
 * be located in the original file's directory.
 *
 * @param filename	[in] Primary filename.
 * @param basename	[in,opt] New basename. If nullptr, uses the existing basename.
 * @param ext		[in] New extension, including leading dot.
 * @return IRpFile*, or nullptr if not found.
 */
LibRpFile::IRpFile *openRelatedFile(const char *filename, const char *basename, const char *ext);

} }
