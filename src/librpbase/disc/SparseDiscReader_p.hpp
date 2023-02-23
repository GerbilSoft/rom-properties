/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SparseDiscReader_p.hpp: Disc reader base class for disc image formats   *
 * that use sparse and/or compressed blocks, e.g. CISO, WBFS, GCZ.         *
 * (PRIVATE CLASS)                                                         *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

namespace LibRpBase {

class SparseDiscReader;
class SparseDiscReaderPrivate
{
	protected:
		explicit SparseDiscReaderPrivate(SparseDiscReader *q);
	public:
		virtual ~SparseDiscReaderPrivate() { };

	private:
		RP_DISABLE_COPY(SparseDiscReaderPrivate)
	protected:
		friend class SparseDiscReader;
		SparseDiscReader *const q_ptr;

	public:
		off64_t disc_size;		// Virtual disc image size.
		off64_t pos;			// Read position.
		unsigned int block_size;	// Block size.
};

}
