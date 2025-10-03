/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * IRpFile.cpp: File wrapper interface.                                    *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "IRpFile.hpp"

// C++ STL classes
using std::unique_ptr;

namespace LibRpFile {

IRpFile::IRpFile()
	: m_lastError(0)
	, m_isWritable(false)
	, m_isCompressed(false)
	, m_fileType(DT_REG)
{
	static_assert(sizeof(off64_t) == 8, "off64_t is not 64-bit!");
}

/**
 * Copy data from this IRpFile to another IRpFile.
 * Read/write positions must be set before calling this function.
 * @param pDestFile	[in] Destination IRpFile.
 * @param size		[in] Number of bytes to copy.
 * @param pcbRead	[out,opt] Number of bytes read.
 * @param pcbWritten	[out,opt] Number of bytes written.
 * @return 0 on success; negative POSIX error code on error.
 */
int IRpFile::copyTo(IRpFile *pDestFile, off64_t size,
	off64_t *pcbRead, off64_t *pcbWritten)
{
	if (!pDestFile->isWritable()) {
		// Destination is not writable.
		return -EPERM;
	}

	int ret = 0;
	off64_t cbReadTotal = 0;
	off64_t cbWrittenTotal = 0;

	// Read buffer
	static constexpr size_t COPYTO_BUFFER_SIZE = 64U * 1024U;
	unique_ptr<uint8_t[]> buf(new uint8_t[COPYTO_BUFFER_SIZE]);

	// Copy the data.
	for (; size > 0; size -= COPYTO_BUFFER_SIZE) {
		const size_t cbRead = this->read(buf.get(), COPYTO_BUFFER_SIZE);
		cbReadTotal += cbRead;
		if (cbRead != COPYTO_BUFFER_SIZE &&
		    (size < static_cast<off64_t>(COPYTO_BUFFER_SIZE) && cbRead != static_cast<size_t>(size)))
		{
			// Short read. We'll continue with a final write.
			ret = -this->m_lastError;
			if (ret == 0) {
				ret = -EIO;
			}
			size = 0;
			if (cbRead == 0)
				break;
		}

		const size_t cbWritten = pDestFile->write(buf.get(), cbRead);
		cbWrittenTotal += cbWritten;
		if (cbWritten != cbRead) {
			// Short write.
			ret = -pDestFile->m_lastError;
			if (ret == 0) {
				ret = -EIO;
			}
			break;
		}
	}

	if (pcbRead) {
		*pcbRead = cbReadTotal;
	}
	if (pcbWritten) {
		*pcbWritten = cbWrittenTotal;
	}
	return ret;
}

}
