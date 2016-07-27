/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WbfsReader.hpp: WBFS disc image reader.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_WBFSREADER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_WBFSREADER_HPP__

#include "IDiscReader.hpp"
#include "libwbfs.h"

namespace LibRomData {

class WbfsReader : public IDiscReader
{
	public:
		/**
		 * Construct a WbfsReader with the specified file.
		 * The file is dup()'d, so the original file can be
		 * closed afterwards.
		 * @param file File to read from.
		 */
		WbfsReader(FILE *file);

		~WbfsReader();

	protected:
		/**
		 * Read the WBFS header.
		 * @param ppHead Pointer to wbfs_head_t*. (will be allocated on success)
		 * @param p wbfs_t struct.
		 * @return Allocated wbfs_t on success; nullptr on error.
		 */
		wbfs_t *readWbfsHeader(void);

		/**
		 * Free an allocated WBFS header.
		 * This frees all associated structs.
		 * All opened discs *must* be closed.
		 * @param wbfs_t wbfs_t struct.
		 */
		void freeWbfsHeader(wbfs_t *p);

		/**
		 * Open a disc from the WBFS image.
		 * @param p wbfs_t struct.
		 * @param index Disc index.
		 * @return Allocated wbfs_disc_t on success; nullptr on error.
		 */
		wbfs_disc_t *openWbfsDisc(wbfs_t *p, uint32_t index);

		/**
		 * Close a WBFS disc.
		 * This frees all associated structs.
		 * @param disc wbfs_disc_t.
		 */
		void closeWbfsDisc(wbfs_disc_t *disc);

		/**
		 * Get the non-sparse size of an open WBFS disc, in bytes.
		 * This scans the block table to find the first block
		 * from the end of wlba_table[] that has been allocated.
		 * @param disc wbfs_disc_t struct.
		 * @return Non-sparse size, in bytes.
		 */
		int64_t getWbfsDiscSize(const wbfs_disc_t *disc);

	public:
		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		virtual size_t read(void *ptr, size_t size) override;

		/**
		 * Set the file position.
		 * @param pos File position.
		 */
		virtual void seek(int64_t pos) override;

	protected:
		// WBFS structs.
		wbfs_t *m_wbfs;

		// Opened disc information.
		wbfs_disc_t *m_wbfs_disc;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_WBFSREADER_HPP__ */
