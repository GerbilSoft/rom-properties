/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IFst.hpp: File System Table interface.                                  *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_IFST_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_IFST_HPP__

#include "config.libromdata.h"

// C includes.
#include <stdint.h>

// Directory type values.
// Based on dirent.h from glibc-2.23.
#include "d_type.h"

namespace LibRomData {

class IFst
{
	protected:
		IFst() { }
	public:
		virtual ~IFst() = 0;

	private:
		IFst(const IFst &other);
		IFst &operator=(const IFst &other);

	public:
		/** opendir() interface. **/

		struct DirEnt {
			int64_t offset;		// Starting address.
			int64_t size;		// File size.
			uint8_t type;		// File type. (See d_type.h)
			const char *name;	// Filename. (TODO: Encoding?)

			// TODO: Additional placeholders?
			int idx;		// File index.
		};

		struct Dir {
			int dir_idx;		// Directory index in the FST.
			DirEnt entry;		// Current FstDirEntry.
		};

		/**
		 * Open a directory.
		 * @param path	[in] Directory path.
		 * @return FstDir*, or nullptr on error.
		 */
		virtual Dir *opendir(const rp_char *path) = 0;

		/**
		 * Open a directory.
		 * @param path	[in] Directory path.
		 * @return FstDir*, or nullptr on error.
		 */
		inline Dir *opendir(const LibRomData::rp_string &path)
		{
			return opendir(path.c_str());
		}

		/**
		 * Read a directory entry.
		 * @param dirp FstDir pointer.
		 * @return FstDirEntry*, or nullptr if end of directory or on error.
		 * (TODO: Add lastError()?)
		 */
		virtual DirEnt *readdir(Dir *dirp) = 0;

		/**
		 * Close an opened directory.
		 * @param dirp FstDir pointer.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int closedir(Dir *dirp) = 0;
};

/**
 * Both gcc and MSVC fail to compile unless we provide
 * an empty implementation, even though the function is
 * declared as pure-virtual.
 */
inline IFst::~IFst() { }

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_IFst_HPP__ */
