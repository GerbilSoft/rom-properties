/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpFile_Win32.cpp: Standard file object. (Win32 implementation)          *
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

// Define this symbol to get XP themes. See:
// http://msdn.microsoft.com/library/en-us/dnwxp/html/xptheming.asp
// for more info. Note that as of May 2006, the page says the symbols should
// be called "SIDEBYSIDE_COMMONCONTROLS" but the headers in my SDKs in VC 6 & 7
// don't reference that symbol. If ISOLATION_AWARE_ENABLED doesn't work for you,
// try changing it to SIDEBYSIDE_COMMONCONTROLS
#define ISOLATION_AWARE_ENABLED 1

// Windows API
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

namespace LibRomData {

static inline int mode_to_win32(RpFile::FileMode mode, DWORD *pdwDesiredAccess, DWORD *pdwCreationDisposition)
{
	switch (mode) {
		case RpFile::FM_OPEN_READ:
			*pdwDesiredAccess = GENERIC_READ;
			*pdwCreationDisposition = OPEN_EXISTING;
			break;
		case RpFile::FM_OPEN_WRITE:
			*pdwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
			*pdwCreationDisposition = OPEN_EXISTING;
			break;
		case RpFile::FM_CREATE|RpFile::FM_READ /*RpFile::FM_CREATE_READ*/ :
		case RpFile::FM_CREATE_WRITE:
			*pdwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
			*pdwCreationDisposition = CREATE_ALWAYS;
			break;
		default:
			// Invalid mode.
			return -1;
	}

	// Mode converted successfully.
	return 0;
}

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename.
 * @param mode File mode.
 */
RpFile::RpFile(const rp_char *filename, FileMode mode)
	: IRpFile()
	, m_file(INVALID_HANDLE_VALUE)
	, m_mode(mode)
	, m_lastError(0)
{
	// TODO: Combine the two constructors.
#ifndef RP_UTF16
	#error RpFile_Win32.cpp only supports UTF-16. FIXME?
#endif

	// Determine the file mode.
	DWORD dwDesiredAccess, dwCreationDisposition;
	if (mode_to_win32(mode, &dwDesiredAccess, &dwCreationDisposition) != 0) {
		// Invalid mode.
		m_lastError = EINVAL;
		return;
	}

	// Open the file.
	m_file = CreateFile(reinterpret_cast<LPCWSTR>(filename),
			dwDesiredAccess, FILE_SHARE_READ, nullptr,
			dwCreationDisposition, FILE_ATTRIBUTE_NORMAL,
			nullptr);
	if (m_file == INVALID_HANDLE_VALUE) {
		// Error opening the file.
		// TODO: More extensive conversion of GetLastError() to POSIX?
		DWORD dwError = GetLastError();
		m_lastError = (dwError == ERROR_FILE_NOT_FOUND ? ENOENT : EIO);
		return;
	}
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
	// TODO: Combine the two constructors.
#ifndef RP_UTF16
	#error RpFile_Win32.cpp only supports UTF-16. FIXME?
#endif

	// Determine the file mode.
	DWORD dwDesiredAccess, dwCreationDisposition;
	if (mode_to_win32(mode, &dwDesiredAccess, &dwCreationDisposition) != 0) {
		// Invalid mode.
		m_lastError = EINVAL;
		return;
	}

	// Open the file.
	m_file = CreateFile(reinterpret_cast<LPCWSTR>(filename.c_str()),
			dwDesiredAccess, FILE_SHARE_READ, nullptr,
			dwCreationDisposition, FILE_ATTRIBUTE_NORMAL,
			nullptr);
	if (m_file == INVALID_HANDLE_VALUE) {
		// Error opening the file.
		// TODO: More extensive conversion of GetLastError() to POSIX?
		DWORD dwError = GetLastError();
		m_lastError = (dwError == ERROR_FILE_NOT_FOUND ? ENOENT : EIO);
		return;
	}
}

RpFile::~RpFile()
{
	if (m_file != INVALID_HANDLE_VALUE) {
		CloseHandle(m_file);
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

	if (other.m_file == INVALID_HANDLE_VALUE) {
		// No file to dup().
		return;
	}

	HANDLE hProcess = GetCurrentProcess();
	BOOL bRet = DuplicateHandle(hProcess, other.m_file,
			hProcess, &m_file,
			0, FALSE, DUPLICATE_SAME_ACCESS);
	if (bRet == 0) {
		// An error occurred.
		// TODO: Convert GetLastError() to POSIX?
		m_file = INVALID_HANDLE_VALUE;
		m_lastError = ENOMEM;
		return;
	}

	// Make sure we're at the beginning of the file.
	LARGE_INTEGER liSeekPos;
	liSeekPos.QuadPart = 0;
	SetFilePointerEx(m_file, liSeekPos, nullptr, FILE_BEGIN);
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
	if (m_file != INVALID_HANDLE_VALUE) {
		CloseHandle(m_file);
		m_file = INVALID_HANDLE_VALUE;
	}

	m_mode = other.m_mode;
	m_lastError = other.m_lastError;

	if (other.m_file == INVALID_HANDLE_VALUE) {
		// No file to dup().
		return *this;
	}

	HANDLE hProcess = GetCurrentProcess();
	BOOL bRet = DuplicateHandle(hProcess, other.m_file,
			hProcess, &m_file,
			0, FALSE, DUPLICATE_SAME_ACCESS);
	if (bRet == 0) {
		// An error occurred.
		// TODO: Convert GetLastError() to POSIX?
		m_file = INVALID_HANDLE_VALUE;
		m_lastError = ENOMEM;
		return *this;
	}

	// Make sure we're at the beginning of the file.
	LARGE_INTEGER liSeekPos;
	liSeekPos.QuadPart = 0;
	SetFilePointerEx(m_file, liSeekPos, nullptr, FILE_BEGIN);
	return *this;
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpFile::isOpen(void) const
{
	return (m_file != INVALID_HANDLE_VALUE);
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
	if (m_file != INVALID_HANDLE_VALUE) {
		CloseHandle(m_file);
		m_file = INVALID_HANDLE_VALUE;
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
	if (m_file == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return 0;
	}

	DWORD bytesRead;
	BOOL bRet = ReadFile(m_file, ptr, (DWORD)size, &bytesRead, nullptr);
	if (bRet == 0) {
		// An error occurred.
		// TODO: Convert GetLastError() to POSIX?
		m_lastError = EIO;
		return 0;
	}

	return bytesRead;
}

/**
 * Set the file position.
 * @param pos File position.
 * @return 0 on success; -1 on error.
 */
int RpFile::seek(int64_t pos)
{
	if (m_file == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return -1;
	}

	LARGE_INTEGER liSeekPos;
	liSeekPos.QuadPart = pos;
	BOOL bRet = SetFilePointerEx(m_file, liSeekPos, nullptr, FILE_BEGIN);
	if (bRet == 0) {
		// TODO: Convert GetLastError() to POSIX?
		m_lastError = EIO;
	}
	return (bRet == 0 ? -1 : 0);
}

/**
 * Seek to the beginning of the file.
 */
void RpFile::rewind(void)
{
	if (m_file == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return;
	}

	LARGE_INTEGER liSeekPos;
	liSeekPos.QuadPart = 0;
	SetFilePointerEx(m_file, liSeekPos, nullptr, FILE_BEGIN);
}

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
int64_t RpFile::fileSize(void)
{
	if (m_file == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return -1;
	}

	LARGE_INTEGER liFileSize;
	BOOL bRet = GetFileSizeEx(m_file, &liFileSize);
	return (bRet != 0 ? liFileSize.QuadPart : -1);
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
