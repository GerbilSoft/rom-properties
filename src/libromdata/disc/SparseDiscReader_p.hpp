/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SparseDiscReader_p.hpp: Disc reader base class for disc image formats   *
 * that use sparse and/or compressed blocks, e.g. CISO, WBFS, GCZ.         *
 * (PRIVATE CLASS)                                                         *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_SPARSEDISCREADER_P_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_SPARSEDISCREADER_P_HPP__

#include <stdint.h>
#include "../common.h"

namespace LibRomData {

class IRpFile;
class SparseDiscReader;

class SparseDiscReaderPrivate
{
	protected:
		SparseDiscReaderPrivate(SparseDiscReader *q, IRpFile *file);
	public:
		virtual ~SparseDiscReaderPrivate();

	private:
		RP_DISABLE_COPY(SparseDiscReaderPrivate)
	protected:
		friend class SparseDiscReader;
		SparseDiscReader *const q_ptr;

	public:
		IRpFile *file;		// Disc image file.
		int64_t disc_size;	// Virtual disc image size.
		int64_t pos;		// Read position.
		unsigned int block_size;	// Block size.
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_SPARSEDISCREADER_P_HPP__ */
