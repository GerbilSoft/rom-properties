/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GcnFst.hpp: GameCube/Wii FST parser.                                    *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_GCNFST_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_GCNFST_HPP__

#include "config.libromdata.h"
#include "gcn_structs.h"

// C includes.
#include <stdint.h>

// Directory type values.
// Based on dirent.h from glibc-2.23.
#include "d_type.h"

namespace LibRomData {

class GcnFst
{
	public:
		/**
		 * Parse a GameCube FST.
		 * @param fstData FST data.
		 * @param len Length of fstData, in bytes.
		 * @param offsetShift File offset shift. (0 = GCN, 2 = Wii)
		 */
		GcnFst(const uint8_t *fstData, uint32_t len, uint8_t offsetShift);
		~GcnFst();

	private:
		GcnFst(const GcnFst &other);
		GcnFst &operator=(const GcnFst &other);

	public:
		/**
		 * Get the number of FST entries.
		 * @return Number of FST entries, or -1 on error.
		 */
		int count(void) const;

		/** opendir() interface. **/
		// TOOD: IFst?

		/**
		 * Get an FST entry.
		 * NOTE: FST entries have NOT been byteswapped.
		 * Use be32_to_cpu() when reading.
		 * @param idx		[in] FST entry index.
		 * @param ppszName	[out, opt] Entry name. (Do not free this!)
		 * @return FST entry, or nullptr on error.
		 */
		const GCN_FST_Entry *entry(int idx, const char **ppszName = nullptr) const;

		struct FstDirEntry {
			int idx;	// File index.
			uint8_t type;	// File type. (See d_type.h)
			const char *name;	// Filename. (TODO: Encoding?)
			int64_t offset;		// Starting address.
			uint32_t size;		// File size.
		};

		struct FstDir {
			int dir_idx;		// Directory index in the FST.
			FstDirEntry entry;	// Current FstDirEntry.
		};

		/**
		 * Open a directory.
		 * @param path	[in] Directory path. [TODO; always reads "/" right now.]
		 * @return FstDir*, or nullptr on error.
		 */
		FstDir *opendir(const rp_char *path);

		/**
		 * Open a directory.
		 * @param path	[in] Directory path. [TODO; always reads "/" right now.]
		 * @return FstDir*, or nullptr on error.
		 */
		inline FstDir *opendir(const LibRomData::rp_string &path)
		{
			return opendir(path.c_str());
		}

		/**
		 * Read a directory entry.
		 * @param dirp FstDir pointer.
		 * @return FstDirEntry*, or nullptr if end of directory or on error.
		 * (TODO: Add lastError()?)
		 */
		FstDirEntry *readdir(FstDir *dirp);

		/**
		 * Close an opened directory.
		 * @param dirp FstDir pointer.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int closedir(FstDir *dirp);

	private:
		// FST data.
		GCN_FST_Entry *m_fstData;
		uint32_t m_fstData_sz;

		// String table. (Pointer into m_fstData.)
		// String table. (malloc'd)
		const char *m_string_table;
		uint32_t m_string_table_sz;

		// Offset shift.
		uint8_t m_offsetShift;

		// FstDir* reference counter.
		int m_fstDirCount;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_GCNFST_HPP__ */
