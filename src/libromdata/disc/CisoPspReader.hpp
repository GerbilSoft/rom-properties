/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CisoPspReader.hpp: PlayStation Portable CISO disc image reader.         *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_CISOPSPREADER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_CISOPSPREADER_HPP__

#include "librpbase/disc/SparseDiscReader.hpp"

namespace LibRomData {

class CisoPspReaderPrivate;
class CisoPspReader : public LibRpBase::SparseDiscReader
{
	public:
		/**
		 * Construct a CisoPspReader with the specified file.
		 * The file is ref()'d, so the original file can be
		 * unref()'d by the caller afterwards.
		 * @param file File to read from.
		 */
		explicit CisoPspReader(LibRpFile::IRpFile *file);

	private:
		typedef SparseDiscReader super;
		RP_DISABLE_COPY(CisoPspReader)
	private:
		friend class CisoPspReaderPrivate;

	public:
		/** Disc image detection functions. **/

		/**
		 * Is a disc image supported by this class?
		 * @param pHeader Disc image header.
		 * @param szHeader Size of header.
		 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
		 */
		ATTR_ACCESS_SIZE(read_only, 1, 2)
		static int isDiscSupported_static(const uint8_t *pHeader, size_t szHeader);

		/**
		 * Is a disc image supported by this object?
		 * @param pHeader Disc image header.
		 * @param szHeader Size of header.
		 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
		 */
		ATTR_ACCESS_SIZE(read_only, 2, 3)
		int isDiscSupported(const uint8_t *pHeader, size_t szHeader) const final;

	protected:
		/** SparseDiscReader functions. **/

		/**
		 * Get the physical address of the specified logical block index.
		 *
		 * @param blockIdx	[in] Block index.
		 * @return Physical address. (0 == empty block; -1 == invalid block index)
		 */
		off64_t getPhysBlockAddr(uint32_t blockIdx) const final;

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
		ATTR_ACCESS_SIZE(write_only, 3, 5)
		int readBlock(uint32_t blockIdx, void *ptr, int pos, size_t size) final;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_GCZREADER_HPP__ */
