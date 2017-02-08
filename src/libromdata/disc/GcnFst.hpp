/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GcnFst.hpp: GameCube/Wii FST parser.                                    *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_GCNFST_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_GCNFST_HPP__

#include "IFst.hpp"
#include "gcn_structs.h"

namespace LibRomData {

class GcnFstPrivate;
class GcnFst : public IFst
{
	public:
		/**
		 * Parse a GameCube FST.
		 * @param fstData FST data.
		 * @param len Length of fstData, in bytes.
		 * @param offsetShift File offset shift. (0 = GCN, 2 = Wii)
		 */
		GcnFst(const uint8_t *fstData, uint32_t len, uint8_t offsetShift);
		virtual ~GcnFst();

	private:
		typedef IFst super;
		RP_DISABLE_COPY(GcnFst)

	private:
		friend class GcnFstPrivate;
		GcnFstPrivate *const d;

	public:
		/**
		 * Is the FST open?
		 * @return True if open; false if not.
		 */
		virtual bool isOpen(void) const override final;

		/**
		 * Have any errors been detected in the FST?
		 * @return True if yes; false if no.
		 */
		virtual bool hasErrors(void) const override final;

	public:
		/** opendir() interface. **/

		/**
		 * Open a directory.
		 * @param path	[in] Directory path.
		 * @return Dir*, or nullptr on error.
		 */
		virtual Dir *opendir(const rp_char *path) override final;

		/**
		 * Read a directory entry.
		 * @param dirp Dir pointer.
		 * @return DirEnt*, or nullptr if end of directory or on error.
		 * (TODO: Add lastError()?)
		 */
		virtual DirEnt *readdir(Dir *dirp) override final;

		/**
		 * Close an opened directory.
		 * @param dirp Dir pointer.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int closedir(Dir *dirp) override final;

		/**
		 * Get the directory entry for the specified file.
		 * @param filename	[in] Filename.
		 * @param dirent	[out] Pointer to DirEnt buffer.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int find_file(const rp_char *filename, DirEnt *dirent) override final;

	public:
		/**
		 * Get the total size of all files.
		 *
		 * This is a shortcut function that reads the FST
		 * directly instead of using opendir().
		 *
		 * @return Size of all files, in bytes. (-1 on error)
		 */
		int64_t totalUsedSize(void) const;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_GCNFST_HPP__ */
