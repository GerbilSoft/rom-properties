/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * IRpFile.cpp: File wrapper interface.                                    *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "IRpFile.hpp"

// librpthreads
#include "librpthreads/Atomics.h"

namespace LibRpFile {

IRpFile::IRpFile()
	: m_lastError(0)
	, m_isWritable(false)
	, m_isCompressed(false)
{
	static_assert(sizeof(off64_t) == 8, "off64_t is not 64-bit!");
}

/**
 * Get a single character (byte) from the file
 * @return Character from file, or EOF on end of file or error.
 */
int IRpFile::getc(void)
{
	uint8_t buf;
	size_t sz = this->read(&buf, 1);
	return (sz == 1 ? buf : EOF);
}

/**
 * Un-get a single character (byte) from the file.
 *
 * Note that this implementation doesn't actually
 * use a character buffer; it merely decrements the
 * seek pointer by 1.
 *
 * @param c Character. (ignored!)
 * @return 0 on success; non-zero on error.
 */
int IRpFile::ungetc(int c)
{
	RP_UNUSED(c);	// TODO: Don't ignore this?

	// TODO: seek() overload that supports SEEK_CUR?
	off64_t pos = tell();
	if (pos <= 0) {
		// Cannot ungetc().
		return -1;
	}

	return this->seek(pos-1);
}

}
