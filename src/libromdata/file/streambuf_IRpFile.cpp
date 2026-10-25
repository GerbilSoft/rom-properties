#include "streambuf_IRpFile.hpp"

// Other rom-properties classes
using LibRpFile::IRpFile;
using LibRpFile::IRpFilePtr;

namespace LibRomData {

/**
 * Create a (read-only) std::streambuf wrapper around an IRpFile.
 * @param file IRpFilePtr
 */
streambuf_IRpFile::streambuf_IRpFile(const IRpFilePtr &file)
	: m_file(file)
{
	if (!m_file) {
		return;
	}

	// Initialize the read buffer.
	// NOTE: Read-only; not supporting writes.
	m_in_buffer.resize(BUFFER_SIZE);
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

	if (!(which & std::ios_base::in)) {
		// Read-only; this seek is not valid.
		return static_cast<pos_type>(-1);
	}

	if (way == std::ios_base::cur) {
		// Seeking from the current position requires some adjustments because
		// part of the buffer might not have been used yet.
		off -= (egptr() - gptr());
	}
	// Invalidate the get pointer.
	setg(nullptr, nullptr, nullptr);

	// Convert from C++ seek constants to IRpFile::SeekWhence (C-style).
	// NOTE: libstdc++ has the same values as C-style functions.
	// Need to check MSVCRT/UCRT and libc++.
	static_assert(static_cast<int>(std::ios_base::beg) == static_cast<int>(IRpFile::SeekWhence::Set), "std::ios_base::beg != IRpFile::SeekWhence::Set");
	static_assert(static_cast<int>(std::ios_base::cur) == static_cast<int>(IRpFile::SeekWhence::Cur), "std::ios_base::cur != IRpFile::SeekWhence::Cur");
	static_assert(static_cast<int>(std::ios_base::end) == static_cast<int>(IRpFile::SeekWhence::End), "std::ios_base::end != IRpFile::SeekWhence::End");

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

} // namespace LibRomData
