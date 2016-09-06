/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpFile_stdio.cpp: Standard file object. (stdio implementation)          *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "RpFile.hpp"
#include "TextFuncs.hpp"

// C++ includes.
#include <string>
using std::string;
using std::u16string;

#ifdef _WIN32
// Windows: _wfopen() requires a Unicode mode string.
typedef wchar_t mode_str_t;
#define _MODE(str) (L ##str)
#include "RpWin32.hpp"
#else
// Other: fopen() requires an 8-bit mode string.
typedef char mode_str_t;
#define _MODE(str) (str)
#endif

 // dup()
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace LibRomData {

static inline const mode_str_t *mode_to_str(RpFile::FileMode mode)
{
	switch (mode) {
		case RpFile::FM_OPEN_READ:
			return _MODE("rb");
		case RpFile::FM_OPEN_WRITE:
			return _MODE("rb+");
		case RpFile::FM_CREATE|RpFile::FM_READ /*RpFile::FM_CREATE_READ*/ :
		case RpFile::FM_CREATE_WRITE:
			return _MODE("wb+");
		default:
			// Invalid mode.
			return nullptr;
	}
}

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename.
 * @param mode File mode.
 */
RpFile::RpFile(const rp_char *filename, FileMode mode)
	: super()
	, m_file(nullptr)
	, m_mode(mode)
	, m_lastError(0)
{
	init(filename);
}

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename.
 * @param mode File mode.
 */
RpFile::RpFile(const rp_string &filename, FileMode mode)
	: IRpFile()
	, m_file(nullptr)
	, m_mode(mode)
	, m_lastError(0)
{
	init(filename.c_str());
}

/**
 * Common initialization function for RpFile's constructors.
 * @param filename Filename.
 */
void RpFile::init(const rp_char *filename)
{
	const mode_str_t *mode_str = mode_to_str(m_mode);
	if (!mode_str) {
		m_lastError = EINVAL;
		return;
	}

	// TODO: On Windows, prepend "\\\\?\\" for super-long filenames?

#if defined(_WIN32)
	// Windows: Use RP2W() to convert the filename to wchar_t.
	m_file = _wfopen(RP2W_s(filename), mode_str);
#else /* !_WIN32 */
	// Linux: Use UTF-8 filenames.
#if defined(RP_UTF8)
	m_file = fopen(filename, mode_str);
#elif defined(RP_UTF16)
	string u8_filename = rp_string_to_utf8(filename, rp_strlen(filename));
	m_file = fopen(u8_filename.c_str(), mode_str);
#endif /* RP_UTF8, RP_UTF16 */
#endif /* _WIN32 */

	if (!m_file) {
		// An error occurred while opening the file.
		m_lastError = errno;
	}
}

RpFile::~RpFile()
{
	if (m_file) {
		fclose(m_file);
	}
}

/**
 * Copy constructor.
 * @param other Other instance.
 */
RpFile::RpFile(const RpFile &other)
	: IRpFile()
	, m_file(nullptr)
	, m_mode(other.m_mode)
	, m_lastError(0)
{
	// TODO: Combine with assignment constructor?

	if (other.m_file == nullptr) {
		// No file to dup().
		return;
	}

	// Get the mode string.
	const mode_str_t *mode_str = mode_to_str(m_mode);
	if (!mode_str) {
		m_lastError = EINVAL;
		return;
	}

	// dup() the file.
	int fd_old = fileno(other.m_file);
	if (fd_old < 0) {
		m_lastError = errno;
		return;
	}
	int fd_new = ::dup(fd_old);
	if (fd_new < 0) {
		m_lastError = errno;
		return;
	}

#ifdef _WIN32
	m_file = _wfdopen(fd_new, mode_str);
#else
	m_file = fdopen(fd_new, mode_str);
#endif
	if (!m_file) {
		m_lastError = errno;
		::close(fd_new);
		return;
	}

	// Make sure we're at the beginning of the file.
	::rewind(m_file);
	::fflush(m_file);
}

/**
 * Assignment operator.
 * @param other Other instance.
 * @return This instance.
 */
RpFile &RpFile::operator=(const RpFile &other)
{
	// TODO: Combine with copy constructor?

	// If we have a file open, close it first.
	if (m_file) {
		fclose(m_file);
		m_file = nullptr;
	}

	m_mode = other.m_mode;
	m_lastError = other.m_lastError;

	if (other.m_file == nullptr) {
		// No file to dup().
		return *this;
	}

	// Get the mode string.
	const mode_str_t *mode_str = mode_to_str(m_mode);
	if (!mode_str) {
		m_lastError = EINVAL;
		return *this;
	}

	// dup() the file.
	int fd_old = fileno(other.m_file);
	if (fd_old < 0) {
		m_lastError = errno;
		return *this;
	}
	int fd_new = ::dup(fd_old);
	if (fd_new < 0) {
		m_lastError = errno;
		return *this;
	}

#ifdef _WIN32
	m_file = _wfdopen(fd_new, mode_str);
#else
	m_file = fdopen(fd_new, mode_str);
#endif
	if (!m_file) {
		m_lastError = errno;
		::close(fd_new);
		return *this;
	}

	// Make sure we're at the beginning of the file.
	::rewind(m_file);
	::fflush(m_file);
	return *this;
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpFile::isOpen(void) const
{
	return (m_file != nullptr);
}

/**
 * dup() the file handle.
 * Needed because IRpFile* objects are typically
 * pointers, not actual instances of the object.
 * @return dup()'d file, or nullptr on error.
 */
IRpFile *RpFile::dup(void)
{
	return new RpFile(*this);
}

/**
 * Close the file.
 */
void RpFile::close(void)
{
	if (m_file) {
		fclose(m_file);
		m_file = nullptr;
	}
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpFile::read(void *ptr, size_t size)
{
	if (!m_file) {
		m_lastError = EBADF;
		return 0;
	}

	size_t ret = fread(ptr, 1, size, m_file);
	if (ferror(m_file)) {
		// An error occurred.
		m_lastError = errno;
	}
	return ret;
}

/**
 * Write data to the file.
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t RpFile::write(const void *ptr, size_t size)
{
	if (!m_file || !(m_mode & FM_WRITE)) {
		// Either the file isn't open,
		// or it's read-only.
		m_lastError = EBADF;
		return 0;
	}

	size_t ret = fwrite(ptr, 1, size, m_file);
	if (ferror(m_file)) {
		// An error occurred.
		m_lastError = errno;
	}
	return ret;
}

/**
 * Set the file position.
 * @param pos File position.
 * @return 0 on success; -1 on error.
 */
int RpFile::seek(int64_t pos)
{
	if (!m_file) {
		m_lastError = EBADF;
		return -1;
	}

	int ret = fseek(m_file, pos, SEEK_SET);
	if (ret != 0) {
		m_lastError = errno;
	}
	return ret;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
int64_t RpFile::tell(void)
{
	if (!m_file) {
		m_lastError = EBADF;
		return -1;
	}

	return ftell(m_file);
}

/**
 * Seek to the beginning of the file.
 */
void RpFile::rewind(void)
{
	if (!m_file) {
		m_lastError = EBADF;
		return;
	}

	::rewind(m_file);
	::fflush(m_file);	// needed for some things like gzip
}

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
int64_t RpFile::fileSize(void)
{
	if (!m_file) {
		m_lastError = EBADF;
		return -1;
	}

	// TODO: Error checking?

	// Save the current position.
	int64_t cur_pos = ftell(m_file);

	// Seek to the end of the file and record its position.
	fseek(m_file, 0, SEEK_END);
	int64_t end_pos = ftell(m_file);

	// Go back to the previous position.
	fseek(m_file, cur_pos, SEEK_SET);

	// Return the file size.
	return end_pos;
}

/**
 * Get the last error.
 * @return Last POSIX error, or 0 if no error.
 */
int RpFile::lastError(void) const
{
	// TODO: Move up to IRpFile?
	return m_lastError;
}

}
