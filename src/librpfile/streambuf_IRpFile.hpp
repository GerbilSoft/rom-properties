/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * streambuf_IRpFile.hpp: std::streambuf wrapper around IRpFile.           *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Other rom-properties classes
#include "IRpFile.hpp"

// C++ STL classes
#include <array>
#include <iostream>

namespace LibRpFile {

class RP_LIBROMDATA_PUBLIC streambuf_IRpFile : public std::streambuf
{
public:
	/**
	 * Create an std::streambuf wrapper around an IRpFile.
	 * @param file IRpFilePtr
	 * @param mode std::ios_base::openmode
	 */
	explicit streambuf_IRpFile(const LibRpFile::IRpFilePtr &file, std::ios_base::openmode mode);

	~streambuf_IRpFile() override;

protected:
	/**
	 * Read buffer has underflowed.
	 * @return Next available character
	 */
	int_type underflow() override;

	/**
	 * Write buffer has overflowed.
	 * @param ch Character to write
	 * @return Next available character
	 */
	int_type overflow(int_type ch) override;

	/**
	 * Seek to the specified offset in the file.
	 * @param off Offset
	 * @param way Seek from the beginning, current position, or end of the file.
	 * @param which Which pointer to adjust: in, out, or both
	 */
	pos_type seekoff(off_type off,
		std::ios_base::seekdir way,
		std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;

	/**
	 * Seek to the specified position in the file.
	 * @param sp Position (from the beginning of the file)
	 * @param which Which pointer to adjust: in, out, or both
	 */
	pos_type seekpos(pos_type sp, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;

	/**
	 * Synchronize the buffers.
	 * @return 0 on success; non-zero on error.
	 */
	int sync(void) override;

private:
	/**
	 * Flush the output buffer.
	 * @return 0 on success; -1 on error.
	 */
	int flush_output_buffer(void);

private:
	// Size of each buffer (4 KB)
	static constexpr size_t BUFFER_SIZE = 4096U;

	// File pointer and mode
	LibRpFile::IRpFilePtr m_file;
	std::ios_base::openmode m_mode;

	// Allocated buffer
	char *m_buffer;

	// Pointers to Input and Output sections of the allocated buffer
	char *m_in_buffer;
	char *m_out_buffer;
};

} // namespace LibRpFile
