/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GdiReader.hpp: GD-ROM reader for Dreamcast GDI images.                  *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_GDIREADER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_GDIREADER_HPP__

#include "librpbase/disc/SparseDiscReader.hpp"

namespace LibRomData {

class IsoPartition;
class ISO;

class GdiReaderPrivate;
class GdiReader : public LibRpBase::SparseDiscReader
{
	public:
		/**
		 * Construct a GdiReader with the specified file.
		 * The file is ref()'d, so the original file can be
		 * unref()'d by the caller afterwards.
		 * @param file File to read from.
		 */
		explicit GdiReader(LibRpFile::IRpFile *file);

	private:
		typedef SparseDiscReader super;
		RP_DISABLE_COPY(GdiReader)
	private:
		friend class GdiReaderPrivate;

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
		 * NOTE: Not implemented in this subclass.
		 *
		 * @param blockIdx	[in] Block index.
		 * @return Physical block address. (-1 due to not being implemented)
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
		int readBlock(uint32_t blockIdx, void *ptr, int pos, size_t size) final;

	public:
		/** GDI-specific functions. **/

		/**
		 * Get the track count.
		 * @return Track count.
		 */
		int trackCount(void) const;

		/**
		 * Get the starting LBA of the specified track number.
		 * @param trackNumber Track number. (1-based)
		 * @return Starting LBA, or -1 if the track number is invalid.
		 */
		int startingLBA(int trackNumber) const;

		/**
		 * Open a track using IsoPartition.
		 * @param trackNumber Track number. (1-based)
		 * @return IsoPartition, or nullptr on error.
		 */
		IsoPartition *openIsoPartition(int trackNumber);

		/**
		 * Create an ISO RomData object for a given track number.
		 * @param trackNumber Track number. (1-based)
		 * @return ISO object, or nullptr on error.
		 */
		ISO *openIsoRomData(int trackNumber);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_GDIREADER_HPP__ */
