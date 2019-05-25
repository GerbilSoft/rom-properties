/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Cdrom2352Reader.hpp: CD-ROM reader for 2352-byte sector images.         *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_CDROM2352READER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_CDROM2352READER_HPP__

#include "librpbase/disc/SparseDiscReader.hpp"

namespace LibRpBase {
	class IRpFile;
}

namespace LibRomData {

class Cdrom2352ReaderPrivate;
class Cdrom2352Reader : public LibRpBase::SparseDiscReader
{
	public:
		/**
		 * Construct a Cdrom2352Reader with the specified file.
		 * The file is ref()'d, so the original file can be
		 * unref()'d by the caller afterwards.
		 * @param file File to read from.
		 */
		explicit Cdrom2352Reader(LibRpBase::IRpFile *file);

	private:
		typedef SparseDiscReader super;
		RP_DISABLE_COPY(Cdrom2352Reader)
	private:
		friend class Cdrom2352ReaderPrivate;

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
		int64_t getPhysBlockAddr(uint32_t blockIdx) const final;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_CDROM2352READER_HPP__ */
