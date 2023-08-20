/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Cdrom2352Reader.hpp: CD-ROM reader for 2352-byte sector images.         *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/disc/SparseDiscReader.hpp"

namespace LibRomData {

class Cdrom2352ReaderPrivate;
class Cdrom2352Reader : public LibRpBase::SparseDiscReader
{
	public:
		/**
		 * Construct a Cdrom2352Reader with the specified file.
		 * The file is ref()'d, so the original file can be
		 * unref()'d by the caller afterwards.
		 *
		 * Defaults to 2352-byte sectors.
		 *
		 * @param file File to read from.
		 */
		explicit Cdrom2352Reader(const LibRpFile::IRpFilePtr &file);

		/**
		 * Construct a Cdrom2352Reader with the specified file.
		 * The file is ref()'d, so the original file can be
		 * unref()'d by the caller afterwards.
		 *
		 * @param file File to read from.
		 * @param physBlockSize Sector size. (2352, 2446)
		 */
		explicit Cdrom2352Reader(const LibRpFile::IRpFilePtr &file, unsigned int physBlockSize);

	private:
		typedef SparseDiscReader super;
		RP_DISABLE_COPY(Cdrom2352Reader)
	private:
		friend class Cdrom2352ReaderPrivate;

	private:
		/**
		 * Common initialization function.
		 */
		void init(void);

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
		 * @param pos		[in] Starting position. (Must be >= 0 and <= the block size!)
		 * @param ptr		[out] Output data buffer.
		 * @param size		[in] Amount of data to read, in bytes. (Must be <= the block size!)
		 * @return Number of bytes read, or -1 if the block index is invalid.
		 */
		ATTR_ACCESS_SIZE(write_only, 4, 5)
		int readBlock(uint32_t blockIdx, int pos, void *ptr, size_t size) final;
};

}
