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

// Total reference count for all files.
volatile int IRpFile::ms_refCntTotal = 0;

IRpFile::IRpFile()
	: m_lastError(0)
	, m_refCnt(1)
{
	static_assert(sizeof(off64_t) == 8, "off64_t is not 64-bit!");

	// Increment the total reference count.
	ATOMIC_INC_FETCH(&ms_refCntTotal);
}

/**
 * Take a reference to this IRpFile* object.
 * @return this
 */
IRpFile *IRpFile::ref(void)
{
	ATOMIC_INC_FETCH(&m_refCnt);
	ATOMIC_INC_FETCH(&ms_refCntTotal);
	return this;
}

/**
 * Unreference this IRpFile* object.
 * If ref_cnt reaches 0, the IRpFile* object is deleted.
 */
void IRpFile::unref(void)
{
	assert(m_refCnt > 0);
	assert(ms_refCntTotal > 0);
	if (ATOMIC_DEC_FETCH(&m_refCnt) <= 0) {
		// All references removed.
		delete this;
	}
	ATOMIC_DEC_FETCH(&ms_refCntTotal);
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

/**
 * Seek to the specified address, then read data.
 * @param pos	[in] Requested seek address.
 * @param ptr	[out] Output data buffer.
 * @param size	[in] Amount of data to read, in bytes.
 * @return Number of bytes read on success; 0 on seek or read error.
 */
size_t IRpFile::seekAndRead(off64_t pos, void *ptr, size_t size)
{
	int ret = this->seek(pos);
	if (ret != 0) {
		// Seek error.
		return 0;
	}
	return this->read(ptr, size);
}

}
