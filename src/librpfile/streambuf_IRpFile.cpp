/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * streambuf_IRpFile.cpp: std::streambuf wrapper around IRpFile.           *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "streambuf_IRpFile.hpp"

// Other rom-properties classes
using LibRpFile::IRpFile;
using LibRpFile::IRpFilePtr;

namespace LibRpFile {

/**
 * Create a (read-only) std::streambuf wrapper around an IRpFile.
 * @param file IRpFilePtr
 * @param mode std::ios_base::openmode
 */
streambuf_IRpFile::streambuf_IRpFile(const IRpFilePtr &file, std::ios_base::openmode mode)
	: m_file(file)
	, m_mode(mode)
{
	if (!m_file) {
		return;
	}

	// NOTE: Not checking m_file->isWritable() here.

	// Initialize the buffers.
	m_in_buffer.fill(0);
	m_out_buffer.fill(0);	// TODO: Only if output?

	// Initialize the streambuf pointers.
	if (mode & std::ios_base::in) {
		// Input buffer is initially empty.
		setg(m_in_buffer.data(), m_in_buffer.data(), m_in_buffer.data());
	}
	if (mode & std::ios_base::out) {
		// Output buffer is also initially empty,
		// but it's writable, so let streambuf know
		// the full size is available.
		setp(m_out_buffer.data(), m_out_buffer.data() + m_out_buffer.size());
	}
}

/**
 * Read buffer has underflowed.
 * @return Next available character
 */
streambuf_IRpFile::int_type streambuf_IRpFile::underflow(void)
{
	if (!m_file) {
		return traits_type::eof();
	}

	// Do we still have any data left in the "get" buffer?
	if (gptr() < egptr()) {
		return traits_type::to_int_type(*gptr());
	}

	// Load data from the file.
	size_t sz_read = m_file->read(m_in_buffer.data(), m_in_buffer.size());
	if (sz_read == 0) {
		// Couldn't load anything...
		return traits_type::eof();
	}

	// Update the "get" pointers to tell streambuf how much data is available.
	setg(m_in_buffer.data(), m_in_buffer.data(), m_in_buffer.data() + sz_read);
	return traits_type::to_int_type(*gptr());
}

/**
 * Write buffer has overflowed.
 * @param ch Character to write
 * @return Next available character
 */
streambuf_IRpFile::int_type streambuf_IRpFile::overflow(int_type ch)
{
	if (!m_file) {
		return traits_type::eof();
	}

	// Flush the current output buffer to the IRpFile.
	if (flush_output_buffer() != 0) {
		// flush_output_buffer() failed...
		return traits_type::eof();
	}

	// If ch is not EOF, add it to the buffer.
	if (!traits_type::eq_int_type(ch, traits_type::eof())) {
		*pptr() = traits_type::to_char_type(ch);
		pbump(1);
	}

	return traits_type::not_eof(ch);}

/**
 * Seek to the specified offset in the file.
 * @param off Offset
 * @param way Seek from the beginning, current position, or end of the file.
 * @param which Which pointer to adjust: in, out, or both
 */
streambuf_IRpFile::pos_type streambuf_IRpFile::seekoff(off_type off,
	std::ios_base::seekdir way, std::ios_base::openmode which)
{
	if (!m_file) {
		return static_cast<pos_type>(-1);
	}

	// Ensure that std::ios_base seek constants have the same values as IRpFile::SeekWhence (C-style).
	// NOTE: libstdc++ has the same values as C-style functions.
	// Need to check MSVCRT/UCRT and libc++.
	static_assert(static_cast<int>(std::ios_base::beg) == static_cast<int>(IRpFile::SeekWhence::Set), "std::ios_base::beg != IRpFile::SeekWhence::Set");
	static_assert(static_cast<int>(std::ios_base::cur) == static_cast<int>(IRpFile::SeekWhence::Cur), "std::ios_base::cur != IRpFile::SeekWhence::Cur");
	static_assert(static_cast<int>(std::ios_base::end) == static_cast<int>(IRpFile::SeekWhence::End), "std::ios_base::end != IRpFile::SeekWhence::End");

	if (which & std::ios_base::in) {
		// Adjust the "get" pointer for reading.

		if (way == std::ios_base::cur) {
			// Seeking from the current position requires some adjustments because
			// part of the buffer might not have been used yet.
			off -= (egptr() - gptr());
		}

		// Invalidate the "get" pointer.
		setg(nullptr, nullptr, nullptr);
	}

	if (which & std::ios_base::out) {
		// Flush the output buffer before seeking.
		int ret = flush_output_buffer();
		if (ret != 0) {
			// Flushing the output buffer failed?
			return ret;
		}
	}

	int ret = m_file->seek(off, static_cast<IRpFile::SeekWhence>(way));
	if (ret != 0) {
		return static_cast<pos_type>(-1);
	}

	// Return the new file position.
	return m_file->tell();
}

/**
 * Seek to the specified position in the file.
 * @param sp Position (from the beginning of the file)
 * @param which Which pointer to adjust: in, out, or both
 */
streambuf_IRpFile::pos_type streambuf_IRpFile::seekpos(
	streambuf_IRpFile::pos_type sp,
	std::ios_base::openmode which)
{
	return seekoff(static_cast<off_type>(sp), std::ios_base::beg, which);
}

/**
 * Synchronize the buffers.
 * @return 0 on success; non-zero on error.
 */
int streambuf_IRpFile::sync(void)
{
	if (!m_file) {
		return -1;
	}

	if (m_mode & std::ios_base::out) {
		// Flush the output buffer.
		int ret = flush_output_buffer();
		if (ret != 0) {
			return ret;
		}
	}

	// Flush the IRpFile.
	return m_file->flush();
}

/**
 * Flush the output buffer.
 * @return 0 on success; -1 on error.
 */
int streambuf_IRpFile::flush_output_buffer(void)
{
	if (!m_file || !(m_mode & std::ios_base::out)) {
		return -1;
	}

	ptrdiff_t pending = pptr() - pbase();
	if (pending > 0) {
		size_t bytes_written = m_file->write(pbase(), pending);
		if (bytes_written != static_cast<size_t>(pending)) {
			// Short write?
			return -1;
		}
	}

	// Reset the output buffer pointer.
	setp(m_out_buffer.data(), m_out_buffer.data() + m_out_buffer.size());
	return 0;
}

} // namespace LibRpFile
