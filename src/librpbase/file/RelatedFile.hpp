/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RelatedFile.hpp: Open a related file.                                   *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
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
