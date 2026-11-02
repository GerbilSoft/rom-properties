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

// C includes (C++ namespace)
#include <cstring>

// C++ STL classes
using std::streamsize;

// streambuf_IRpFile *is* used by libromdata (in WiiUPackage),
// but for some reason, ld doesn't find it when linking the
// GTK UI frontends on Debian 8 (ppc). (gcc-4.9.2)
// Use some linker hax to force linkage.
extern "C" {
	extern unsigned char RP_LibRpFile_streambuf_IRpFile_ForceLinkage;
	unsigned char RP_LibRpFile_streambuf_IRpFile_ForceLinkage;
}

namespace LibRpFile {

/**
 * Create a (read-only) std::streambuf wrapper around an IRpFile.
 * @param file IRpFilePtr
 * @param mode std::ios_base::openmode
 */
streambuf_IRpFile::streambuf_IRpFile(const IRpFilePtr &file, std::ios_base::openmode mode)
	: m_file(file)
	, m_mode(mode)
	, m_buffer(nullptr)
	, m_in_buffer(nullptr)
	, m_out_buffer(nullptr)
{
	assert((mode & (std::ios_base::in | std::ios_base::out)) != 0);
	if (!m_file || ((mode & (std::ios_base::in | std::ios_base::out)) == 0)) {
		// No file, and/or no open mode.
		return;
	}

	// NOTE: Not checking m_file->isWritable() here.

	// Initialize the buffers.
	// NOTE: If both In and Out are enabled, we need 2x the buffer size.
	const bool alloc2x = (mode & (std::ios_base::in | std::ios_base::out)) ==
	                             (std::ios_base::in | std::ios_base::out);
	const size_t buffer_size = (alloc2x) ? (BUFFER_SIZE * 2) : BUFFER_SIZE;
	m_buffer = new char[buffer_size];

	// Initialize the streambuf pointers.
	if (mode & std::ios_base::in) {
		// Input buffer is initially empty.
		m_in_buffer = m_buffer;
		setg(m_in_buffer, m_in_buffer, m_in_buffer);
	}
	if (mode & std::ios_base::out) {
		// Output buffer is also initially empty,
		// but it's writable, so let streambuf know
		// the full size is available.
		m_out_buffer = (alloc2x) ? &m_buffer[BUFFER_SIZE] : m_buffer;
		setp(m_out_buffer, m_out_buffer + BUFFER_SIZE);
	}

	const bool is_trunc = (mode & (std::ios_base::out | std::ios_base::trunc)) ==
	                              (std::ios_base::out | std::ios_base::trunc);
	if (is_trunc) {
		// Truncate the file. (Only if writable!)
		// All IRpFile implementations that support truncate()
		// will adjust the file pointer as necessary.
		// TODO: What if it fails?
		m_file->truncate();
	}

	if (mode & (std::ios_base::app | std::ios_base::ate)) {
		// Seek to the end of the file.
		m_file->seek(0, IRpFile::SeekWhence::End);
	}
}

streambuf_IRpFile::~streambuf_IRpFile()
{
	delete[] m_buffer;
}

/**
 * Read buffer has underflowed.
 * @return Next available character
 */
streambuf_IRpFile::int_type streambuf_IRpFile::underflow(void)
{
	assert((bool)m_file);
	assert(m_mode & std::ios_base::in);
	if (!m_file || !(m_mode & std::ios_base::in)) {
		return traits_type::eof();
	}

	// Do we still have any data left in the "get" buffer?
	if (gptr() < egptr()) {
		return traits_type::to_int_type(*gptr());
	}

	// Load data from the file.
	size_t sz_read = m_file->read(m_in_buffer, BUFFER_SIZE);
	if (sz_read == 0) {
		// Couldn't load anything...
		return traits_type::eof();
	}

	// Update the "get" pointers to tell streambuf how much data is available.
	setg(m_in_buffer, m_in_buffer, m_in_buffer + sz_read);
	return traits_type::to_int_type(*gptr());
}

/**
 * Write buffer has overflowed.
 * @param ch Character to write
 * @return Next available character
 */
streambuf_IRpFile::int_type streambuf_IRpFile::overflow(int_type ch)
{
	assert((bool)m_file);
	assert(m_mode & std::ios_base::out);
	if (!m_file || !(m_mode & std::ios_base::out)) {
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

	// FIXME: Maintain two separate seek pointers, one for "get" and one for "put"?
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
	assert((bool)m_file);
	assert(m_mode & std::ios_base::out);
	if (!m_file || !(m_mode & std::ios_base::out)) {
		return -1;
	}

	if (m_mode & std::ios_base::app) {
		// TODO: Verify if this is correct!
		m_file->seek(0, IRpFile::SeekWhence::End);
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
	setp(m_out_buffer, m_out_buffer + BUFFER_SIZE);
	return 0;
}

/** Bulk read/write functions **/

/**
 * Read a block of data.
 * @param s Output buffer
 * @param n Number of bytes to read
 * @return Number of bytes read
 */
streamsize streambuf_IRpFile::xsgetn(char_type *s, streamsize n)
{
	assert((bool)m_file);
	assert(m_mode & std::ios_base::in);
	assert(s != nullptr);
	assert(n > 0);
	if (!m_file || !(m_mode & std::ios_base::in)) {
		return 0;
	}
	if (!s || n <= 0) {
		// Empty output buffer?
		return 0;
	}

	streamsize sz_total = 0;

	while (n > 0) {
		// Check if any data is still available in streambuf's "get" buffer.
		ptrdiff_t gbuf_remain = egptr() - gptr();

		if (gbuf_remain > 0) {
			// Use up the rest of the "get" buffer.
			streamsize to_copy = std::min(n, static_cast<streamsize>(gbuf_remain));
			memcpy(s, gptr(), n);
			gbump(static_cast<int>(to_copy));

			s += to_copy;
			n -= to_copy;
			sz_total += to_copy;
		}

		if (n > 0) {
			// More data has been requested than is available in the "get" buffer.
			if (n >= static_cast<streamsize>(BUFFER_SIZE)) {
				// Requested more than our internal buffer size.
				// Read directly from the file.
				if (m_mode & std::ios_base::out) {
					// Open for writing, so we need to flush the write buffers first.
					if (flush_output_buffer() != 0) {
						// flush_output_buffer() failed...
						return sz_total;
					}
				}

				size_t sz_read = m_file->read(s, n);
				if (sz_read == 0) {
					// Read failed.
					return sz_total;
				}

				sz_total += sz_read;
				break;
			} else {
				// Less than the buffer size.
				// Use underflow() to read the data into the "get" buffer.
				if (underflow() == traits_type::eof()) {
					// End of file?
					return sz_total;
				}
			}
		}
	}

	return sz_total;
}

} // namespace LibRpFile
