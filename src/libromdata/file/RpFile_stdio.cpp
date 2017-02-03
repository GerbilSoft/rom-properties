/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpFile_stdio.cpp: Standard file object. (stdio implementation)          *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
// Needed for using "\\?\" to bypass MAX_PATH.
using std::wstring;
#include <cctype>
// _chsize()
#include <io.h>

#else /* !_WIN32 */

// Other: fopen() requires an 8-bit mode string.
typedef char mode_str_t;
#define _MODE(str) (str)
// ftruncate()
#include <unistd.h>
#endif

namespace LibRomData {

// Deleter for std::unique_ptr<FILE> m_file.
struct myFile_deleter {
	void operator()(FILE *p) const {
		if (p != nullptr) {
			fclose(p);
		}
	}
};

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
	, m_filename(filename)
	, m_mode(mode)
{
	init();
}

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename.
 * @param mode File mode.
 */
RpFile::RpFile(const rp_string &filename, FileMode mode)
	: super()
	, m_file(nullptr)
	, m_filename(filename)
	, m_mode(mode)
{
	init();
}

/**
 * Common initialization function for RpFile's constructors.
 * Filename must be set in m_filename.
 */
void RpFile::init(void)
{
	const mode_str_t *mode_str = mode_to_str(m_mode);
	if (!mode_str) {
		m_lastError = EINVAL;
		return;
	}

	// TODO: On Windows, prepend "\\\\?\\" for super-long filenames?

#if defined(_WIN32)
	// Windows: Use RP2W() to convert the filename to wchar_t.

	// If this is an absolute path, make sure it starts with
	// "\\?\" in order to support filenames longer than MAX_PATH.
	wstring filenameW;
	if (m_filename.size() > 3 &&
	    iswascii(m_filename[0]) && iswalpha(m_filename[0]) &&
	    m_filename[1] == _RP_CHR(':') && m_filename[2] == _RP_CHR('\\'))
	{
		// Absolute path. Prepend "\\?\" to the path.
		filenameW = L"\\\\?\\";
		filenameW += RP2W_s(m_filename);
	} else {
		// Not an absolute path, or "\\?\" is already
		// prepended. Use it as-is.
		filenameW = RP2W_s(m_filename);
	}

	m_file.reset(_wfopen(filenameW.c_str(), mode_str), myFile_deleter());
#else /* !_WIN32 */
	// Linux: Use UTF-8 filenames.
#if defined(RP_UTF8)
	m_file.reset(fopen(m_filename.c_str(), mode_str), myFile_deleter());
#elif defined(RP_UTF16)
	string u8_filename = rp_string_to_utf8(m_filename);
	m_file.reset(fopen(u8_filename.c_str(), mode_str), myFile_deleter());
#endif /* RP_UTF8, RP_UTF16 */
#endif /* _WIN32 */

	if (!m_file) {
		// An error occurred while opening the file.
		m_lastError = errno;
	}
}

RpFile::~RpFile()
{
	m_file.reset();
}

/**
 * Copy constructor.
 * @param other Other instance.
 */
RpFile::RpFile(const RpFile &other)
	: super()
	, m_file(other.m_file)
	, m_filename(other.m_filename)
	, m_mode(other.m_mode)
{
	m_lastError = other.m_lastError;
}

/**
 * Assignment operator.
 * @param other Other instance.
 * @return This instance.
 */
RpFile &RpFile::operator=(const RpFile &other)
{
	m_file = other.m_file;
	m_filename = other.m_filename;
	m_mode = other.m_mode;
	m_lastError = other.m_lastError;
	return *this;
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpFile::isOpen(void) const
{
	return (m_file.get() != nullptr);
}

/**
 * dup() the file handle.
 *
 * Needed because IRpFile* objects are typically
 * pointers, not actual instances of the object.
 *
 * NOTE: The dup()'d IRpFile* does NOT have a separate
 * file pointer. This is due to how dup() works.
 *
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
	m_file.reset();
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

	size_t ret = fread(ptr, 1, size, m_file.get());
	if (ferror(m_file.get())) {
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

	size_t ret = fwrite(ptr, 1, size, m_file.get());
	if (ferror(m_file.get())) {
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

	int ret = fseeko(m_file.get(), pos, SEEK_SET);
	if (ret != 0) {
		m_lastError = errno;
	}
	::fflush(m_file.get());	// needed for some things like gzip
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

	return ftello(m_file.get());
}

/**
 * Truncate the file.
 * @param size New size. (default is 0)
 * @return 0 on success; -1 on error.
 */
int RpFile::truncate(int64_t size)
{
	if (!m_file || !(m_mode & FM_WRITE)) {
		// Either the file isn't open,
		// or it's read-only.
		m_lastError = EBADF;
		return -1;
	} else if (size < 0) {
		m_lastError = EINVAL;
		return -1;
	}

	// Get the current position.
	int64_t pos = ftello(m_file.get());
	if (pos < 0) {
		m_lastError = errno;
		return -1;
	}

	// Truncate the file.
	fflush(m_file.get());
#ifdef _WIN32
	int ret = _chsize_s(fileno(m_file.get()), size);
#else
	int ret = ftruncate(fileno(m_file.get()), size);
#endif
	if (ret != 0) {
		m_lastError = errno;
		return -1;
	}

	// If the previous position was past the new
	// file size, reset the pointer.
	if (pos > size) {
		ret = fseeko(m_file.get(), size, SEEK_SET);
		if (ret != 0) {
			m_lastError = errno;
			return -1;
		}
	}

	// File truncated.
	return 0;
}

/** File properties. **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
int64_t RpFile::size(void)
{
	if (!m_file) {
		m_lastError = EBADF;
		return -1;
	}

	// TODO: Error checking?

	// Save the current position.
	int64_t cur_pos = ftello(m_file.get());

	// Seek to the end of the file and record its position.
	fseeko(m_file.get(), 0, SEEK_END);
	int64_t end_pos = ftello(m_file.get());

	// Go back to the previous position.
	fseeko(m_file.get(), cur_pos, SEEK_SET);

	// Return the file size.
	return end_pos;
}

/**
 * Get the filename.
 * @return Filename. (May be empty if the filename is not available.)
 */
rp_string RpFile::filename(void) const
{
	return m_filename;
}

}
