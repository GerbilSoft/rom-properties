/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CisoGcnReader.hpp: GameCube/Wii CISO disc image reader.                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_CISOGCNREADER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_CISOGCNREADER_HPP__

#include "SparseDiscReader.hpp"

namespace LibRomData {

class CisoGcnReaderPrivate;
class CisoGcnReader : public SparseDiscReader
{
	public:
		/**
		 * Construct a CisoGcnReader with the specified file.
		 * The file is dup()'d, so the original file can be
		 * closed afterwards.
		 * @param file File to read from.
		 */
		explicit CisoGcnReader(IRpFile *file);

	private:
		typedef SparseDiscReader super;
		RP_DISABLE_COPY(CisoGcnReader)
	private:
		friend class CisoGcnReaderPrivate;

	public:
		/** Disc image detection functions. **/

		/**
		 * Is a disc image supported by this class?
		 * @param pHeader Disc image header.
		 * @param szHeader Size of header.
		 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
		 */
		static int isDiscSupported_static(const uint8_t *pHeader, size_t szHeader);

		/**
		 * Is a disc image supported by this object?
		 * @param pHeader Disc image header.
		 * @param szHeader Size of header.
		 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
		 */
		virtual int isDiscSupported(const uint8_t *pHeader, size_t szHeader) const override final;

	protected:
		/** SparseDiscReader functions. **/

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
		virtual int readBlock(uint32_t blockIdx, void *ptr, int pos, size_t size) override final;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_CISOGCNREADER_HPP__ */
