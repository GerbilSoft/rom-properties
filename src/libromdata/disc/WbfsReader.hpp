/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WbfsReader.hpp: WBFS disc image reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/disc/SparseDiscReader.hpp"

namespace LibRomData {

class WbfsReaderPrivate;
class WbfsReader : public LibRpBase::SparseDiscReader
{
	public:
		/**
		 * Construct a WbfsReader with the specified file.
		 * The file is ref()'d, so the original file can be
		 * unref()'d by the caller afterwards.
		 * @param file File to read from.
		 */
		explicit WbfsReader(const LibRpFile::IRpFilePtr &file);

	private:
		typedef SparseDiscReader super;
		RP_DISABLE_COPY(WbfsReader)
	private:
		friend class WbfsReaderPrivate;

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
};

}
