/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SparseDiscReader.hpp: Disc reader base class for disc image formats     *
 * that use sparse and/or compressed blocks, e.g. CISO, WBFS, GCZ.         *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_SPARSEDISCREADER_HPP__
#define __ROMPROPERTIES_LIBRPBASE_SPARSEDISCREADER_HPP__

#include "IDiscReader.hpp"

namespace LibRpBase {

class SparseDiscReaderPrivate;
class SparseDiscReader : public IDiscReader
{
	protected:
		explicit SparseDiscReader(SparseDiscReaderPrivate *d);
	public:
		virtual ~SparseDiscReader();

	private:
		typedef IDiscReader super;
		RP_DISABLE_COPY(SparseDiscReader)
	protected:
		friend class SparseDiscReaderPrivate;
		SparseDiscReaderPrivate *const d_ptr;

	public:
		/** IDiscReader functions. **/

		/**
		 * Is the disc image open?
		 * This usually only returns false if an error occurred.
		 * @return True if the disc image is open; false if it isn't.
		 */
		bool isOpen(void) const final;

		/**
		 * Read data from the disc image.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		size_t read(void *ptr, size_t size) final;

		/**
		 * Set the disc image position.
		 * @param pos disc image position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(int64_t pos) final;

		/**
		 * Seek to the beginning of the disc image.
		 */
		void rewind(void) final;

		/**
		 * Get the disc image position.
		 * @return Disc image position on success; -1 on error.
		 */
		int64_t tell(void) final;

		/**
		 * Get the disc image size.
		 * @return Disc image size, or -1 on error.
		 */
		int64_t size(void) final;

	protected:
		/** Virtual functions for SparseDiscReader subclasses. **/

		/**
		 * Read the specified block.
		 *
		 * This can read either a full block or a partial block.
		 * For a full block, set pos = 0 and size = block_size.
		 *
		 * @param blockIdx	[in] Block index.
		 * @param ptr		[out] Output data buffer.
		 * @param pos		[in] Starting position. (Must be >= 0 and <= the block size!)
		 * @param size		[in] Amount of data to read, in bytes. (Must be <= the block size!)
		 * @return Number of bytes read, or -1 if the block index is invalid.
		 */
		virtual int readBlock(uint32_t blockIdx, void *ptr, int pos, size_t size) = 0;
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_SPARSEDISCREADER_HPP__ */
