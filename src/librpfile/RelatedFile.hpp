/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RelatedFile.hpp: Open a related file.                                   *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_RELATEDFILE_HPP__
#define __ROMPROPERTIES_LIBRPBASE_RELATEDFILE_HPP__

namespace LibRpBase {
	class IRpFile;
}

namespace LibRpBase { namespace FileSystem {

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
LibRpBase::IRpFile *openRelatedFile(const char *filename, const char *basename, const char *ext);

} }

#endif /* __ROMPROPERTIES_LIBRPBASE_RELATEDFILE_HPP__ */
