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

class streambuf_IRpFile : public std::streambuf
{
public:
	/**
	 * Create a (read-only) std::streambuf wrapper around an IRpFile.
	 * @param file IRpFilePtr
	 */
	explicit streambuf_IRpFile(const LibRpFile::IRpFilePtr &file);

protected:
	/**
	 * Read buffer has underflowed.
	 * @return Next available character
	 */
	int_type underflow() override;

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

private:
	static constexpr size_t BUFFER_SIZE = 4096U;
	LibRpFile::IRpFilePtr m_file;
	std::array<char, BUFFER_SIZE> m_in_buffer;
};

} // namespace LibRpFile
