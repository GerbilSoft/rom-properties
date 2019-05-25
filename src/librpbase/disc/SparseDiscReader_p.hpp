/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SparseDiscReader_p.hpp: Disc reader base class for disc image formats   *
 * that use sparse and/or compressed blocks, e.g. CISO, WBFS, GCZ.         *
 * (PRIVATE CLASS)                                                         *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_DISC_SPARSEDISCREADER_P_HPP__
#define __ROMPROPERTIES_LIBRPBASE_DISC_SPARSEDISCREADER_P_HPP__

#include <stdint.h>
#include "../common.h"

namespace LibRpBase {

class IRpFile;
class SparseDiscReader;

class SparseDiscReaderPrivate
{
	protected:
		SparseDiscReaderPrivate(SparseDiscReader *q);
	public:
		virtual ~SparseDiscReaderPrivate() { };

	private:
		RP_DISABLE_COPY(SparseDiscReaderPrivate)
	protected:
		friend class SparseDiscReader;
		SparseDiscReader *const q_ptr;

	public:
		int64_t disc_size;	// Virtual disc image size.
		int64_t pos;		// Read position.
		unsigned int block_size;	// Block size.
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_DISC_SPARSEDISCREADER_P_HPP__ */
