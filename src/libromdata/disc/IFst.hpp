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
#include "common.h"

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
		RP_DISABLE_COPY(IFst)

	public:
		// TODO: Base class?

		/**
		 * Is the FST open?
		 * @return True if open; false if not.
		 */
		virtual bool isOpen(void) const = 0;

		/**
		 * Have any errors been detected in the FST?
		 * @return True if yes; false if no.
		 */
		virtual bool hasErrors(void) const = 0;

	public:
		/** opendir() interface. **/

		struct DirEnt {
			int64_t offset;		// Starting address.
			int64_t size;		// File size.
			uint8_t type;		// File type. (See d_type.h)
			const rp_char *name;	// Filename.

			// TODO: Additional placeholders?
			int idx;		// File index.
		};

		struct Dir {
			IFst *parent;		// IFst that owns this Dir.
			int dir_idx;		// Directory index in the FST.
			DirEnt entry;		// Current DirEnt.
		};

		/**
		 * Open a directory.
		 * @param path	[in] Directory path.
		 * @return Dir*, or nullptr on error.
		 */
		virtual Dir *opendir(const rp_char *path) = 0;

		/**
		 * Open a directory.
		 * @param path	[in] Directory path.
		 * @return Dir*, or nullptr on error.
		 */
		inline Dir *opendir(const LibRomData::rp_string &path)
		{
			return opendir(path.c_str());
		}

		/**
		 * Read a directory entry.
		 * @param dirp Dir pointer.
		 * @return DirEnt*, or nullptr if end of directory or on error.
		 * (TODO: Add lastError()?)
		 */
		virtual DirEnt *readdir(Dir *dirp) = 0;

		/**
		 * Close an opened directory.
		 * @param dirp Dir pointer.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int closedir(Dir *dirp) = 0;

		/**
		 * Get the directory entry for the specified file.
		 * @param filename	[in] Filename.
		 * @param dirent	[out] Pointer to DirEnt buffer.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int find_file(const rp_char *filename, DirEnt *dirent) = 0;
};

/**
 * Both gcc and MSVC fail to compile unless we provide
 * an empty implementation, even though the function is
 * declared as pure-virtual.
 */
inline IFst::~IFst() { }

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_IFst_HPP__ */
