/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpFile_Win32.cpp: Standard file object. (Win32 implementation)          *
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

#include "../RpFile.hpp"
#include "TextFuncs.hpp"
#include "RpWin32.hpp"

// C includes. (C++ namespace)
#include <cctype>

// C++ includes.
#include <string>
using std::string;
using std::wstring;

// Windows SDK
#include <windows.h>
#include <winioctl.h>

namespace LibRomData {

/**
 * Deleter for std::unique_ptr<void> d->file,
 * where the pointer is actually a HANDLE.
 */
struct myFile_deleter {
	void operator()(HANDLE p) const {
		if (p != nullptr && p != INVALID_HANDLE_VALUE) {
			CloseHandle(p);
		}
	}
};

/** RpFilePrivate **/

class RpFilePrivate
{
	public:
		RpFilePrivate(const rp_char *filename, RpFile::FileMode mode);
		RpFilePrivate(const rp_string &filename, RpFile::FileMode mode);

	private:
		RP_DISABLE_COPY(RpFilePrivate)

	public:
		// NOTE: file is a HANDLE, but shared_ptr doesn't
		// work correctly with pointer types as the
		// template parameter.
		std::shared_ptr<void> file;
		rp_string filename;
		RpFile::FileMode mode;

		// Set if the file is a block device, e.g. "D:\".
		bool isBlockDevice;

	public:
		static inline int mode_to_win32(RpFile::FileMode mode,
			DWORD *pdwDesiredAccess,
			DWORD *pdwCreationDisposition);
};

RpFilePrivate::RpFilePrivate(const rp_char *filename, RpFile::FileMode mode)
	: filename(filename)
	, mode(mode)
	, isBlockDevice(false)
{ }

RpFilePrivate::RpFilePrivate(const rp_string &filename, RpFile::FileMode mode)
	: filename(filename)
	, mode(mode)
	, isBlockDevice(false)
{ }

inline int RpFilePrivate::mode_to_win32(RpFile::FileMode mode,
	DWORD *pdwDesiredAccess,
	DWORD *pdwCreationDisposition)
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
	: super()
	, d_ptr(new RpFilePrivate(filename, mode))
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
	, d_ptr(new RpFilePrivate(filename, mode))
{
	init();
}

/**
 * Common initialization function for RpFile's constructors.
 * Filename must be set in d->filename.
 */
void RpFile::init(void)
{
	// Determine the file mode.
	RP_D(RpFile);
	DWORD dwDesiredAccess, dwCreationDisposition;
	if (d->mode_to_win32(d->mode, &dwDesiredAccess, &dwCreationDisposition) != 0) {
		// Invalid mode.
		m_lastError = EINVAL;
		return;
	}

	// Unicode filename.
	wstring filenameW;

	// Check if this is a drive letter.
	if (d->filename.size() >= 3 &&
	    iswascii(d->filename[0]) && iswalpha(d->filename[0]) &&
	    d->filename[1] == _RP_CHR(':') && d->filename[2] == _RP_CHR('\\'))
	{
		// Is it only a drive letter?
		if (d->filename.size() == 3) {
			// This is a drive letter.
			// Only CD-ROM (and similar) drives are supported.
			// TODO: Verify if opening by drive letter works,
			// or if we have to resolve the physical device name.
			if (GetDriveType(RP2W_s(d->filename)) != DRIVE_CDROM) {
				// Not a CD-ROM drive.
				m_lastError = ENOTSUP;
				return;
			}

			// Create a raw device filename.
			// Reference: https://support.microsoft.com/en-us/help/138434/how-win32-based-applications-read-cd-rom-sectors-in-windows-nt
			filenameW = L"\\\\.\\X:";
			filenameW[4] = d->filename[0];
			d->isBlockDevice = true;
		} else {
			// Absolute path.
			// Prepend "\\?\" in order to support filenames longer than MAX_PATH.
			filenameW = L"\\\\?\\";
			filenameW += RP2W_s(d->filename);
		}
	} else {
		// Not an absolute path, or "\\?\" is already
		// prepended. Use it as-is.
		filenameW = RP2W_s(d->filename);
	}

	// Open the file.
	d->file.reset(CreateFile(filenameW.c_str(),
			dwDesiredAccess, FILE_SHARE_READ, nullptr,
			dwCreationDisposition, FILE_ATTRIBUTE_NORMAL,
			nullptr), myFile_deleter());
	if (!d->file || d->file.get() == INVALID_HANDLE_VALUE) {
		// Error opening the file.
		m_lastError = w32err_to_posix(GetLastError());
		return;
	}
}

RpFile::~RpFile()
{
	delete d_ptr;
}

/**
 * Copy constructor.
 * @param other Other instance.
 */
RpFile::RpFile(const RpFile &other)
	: super()
	, d_ptr(new RpFilePrivate(other.d_ptr->filename, other.d_ptr->mode))
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
	RP_D(RpFile);
	d->file = other.d_ptr->file;
	d->filename = other.d_ptr->filename;
	d->mode = other.d_ptr->mode;
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
	RP_D(const RpFile);
	return (d->file && d->file.get() != INVALID_HANDLE_VALUE);
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
	RP_D(RpFile);
	d->file.reset(INVALID_HANDLE_VALUE, myFile_deleter());
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpFile::read(void *ptr, size_t size)
{
	RP_D(RpFile);
	if (!d->file || d->file.get() == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return 0;
	}

	DWORD bytesRead;
	BOOL bRet = ReadFile(d->file.get(), ptr, (DWORD)size, &bytesRead, nullptr);
	if (bRet == 0) {
		// An error occurred.
		m_lastError = w32err_to_posix(GetLastError());
		return 0;
	}

	return bytesRead;
}

/**
 * Write data to the file.
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t RpFile::write(const void *ptr, size_t size)
{
	RP_D(RpFile);
	if (!d->file || d->file.get() == INVALID_HANDLE_VALUE || !(d->mode & FM_WRITE)) {
		// Either the file isn't open,
		// or it's read-only.
		m_lastError = EBADF;
		return 0;
	}

	DWORD bytesWritten;
	BOOL bRet = WriteFile(d->file.get(), ptr, (DWORD)size, &bytesWritten, nullptr);
	if (bRet == 0) {
		// An error occurred.
		m_lastError = w32err_to_posix(GetLastError());
		return 0;
	}

	return bytesWritten;
}

/**
 * Set the file position.
 * @param pos File position.
 * @return 0 on success; -1 on error.
 */
int RpFile::seek(int64_t pos)
{
	RP_D(RpFile);
	if (!d->file || d->file.get() == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return -1;
	}

	LARGE_INTEGER liSeekPos;
	liSeekPos.QuadPart = pos;
	BOOL bRet = SetFilePointerEx(d->file.get(), liSeekPos, nullptr, FILE_BEGIN);
	if (bRet == 0) {
		m_lastError = w32err_to_posix(GetLastError());
		return -1;
	}

	return 0;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
int64_t RpFile::tell(void)
{
	RP_D(RpFile);
	if (!d->file || d->file.get() == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return -1;
	}

	LARGE_INTEGER liSeekPos, liSeekRet;
	liSeekPos.QuadPart = 0;
	BOOL bRet = SetFilePointerEx(d->file.get(), liSeekPos, &liSeekRet, FILE_CURRENT);
	if (bRet == 0) {
		m_lastError = w32err_to_posix(GetLastError());
		return -1;
	}

	return liSeekRet.QuadPart;
}

/**
 * Truncate the file.
 * @param size New size. (default is 0)
 * @return 0 on success; -1 on error.
 */
int RpFile::truncate(int64_t size)
{
	RP_D(RpFile);
	if (!d->file || d->file.get() == INVALID_HANDLE_VALUE || !(d->mode & FM_WRITE)) {
		// Either the file isn't open,
		// or it's read-only.
		m_lastError = EBADF;
		return -1;
	} else if (size < 0) {
		m_lastError = EINVAL;
		return -1;
	}

	// Set the requested end of file, and
	// get the current file position.
	LARGE_INTEGER liSeekPos, liSeekRet;
	liSeekPos.QuadPart = size;
	BOOL bRet = SetFilePointerEx(d->file.get(), liSeekPos, &liSeekRet, FILE_CURRENT);
	if (bRet == 0) {
		m_lastError = w32err_to_posix(GetLastError());
		return -1;
	}

	// Truncate the file.
	bRet = SetEndOfFile(d->file.get());
	if (bRet == 0) {
		m_lastError = w32err_to_posix(GetLastError());
		return -1;
	}

	// Restore the original position if it was
	// less than the new size.
	if (liSeekRet.QuadPart < size) {
		bRet = SetFilePointerEx(d->file.get(), liSeekRet, nullptr, FILE_BEGIN);
		if (bRet == 0) {
			m_lastError = w32err_to_posix(GetLastError());
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
	RP_D(RpFile);
	if (!d->file || d->file.get() == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return -1;
	}

	LARGE_INTEGER liFileSize;
	BOOL bRet = FALSE;
	if (d->isBlockDevice) {
		// Block device. Use an IOCTL to get the disk geometry.
		// NOTE: IOCTL_DISK_GET_DRIVE_GEOMETRY_EX works for CD-ROM,
		// so we don't need to use the CD-ROM specific version,
		// IOCTL_CDROM_GET_DRIVE_GEOMETRY.
		// TODO: Works on 7; check XP and Wine.
		DISK_GEOMETRY_EX dg;
		DWORD dwBytesReturned;	// TODO: Check this?
		bRet = DeviceIoControl(d->file.get(), IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
			NULL, 0, &dg, sizeof(dg), &dwBytesReturned, NULL);
		if (bRet) {
			liFileSize = dg.DiskSize;
		}
	} else {
		// Regular file.
		bRet = GetFileSizeEx(d->file.get(), &liFileSize);
	}

	if (!bRet) {
		// Could not get the file/device size.
		m_lastError = w32err_to_posix(GetLastError());
		return -1;
	}

	// Return the file/device size.
	return liFileSize.QuadPart;
}

/**
 * Get the filename.
 * @return Filename. (May be empty if the filename is not available.)
 */
rp_string RpFile::filename(void) const
{
	RP_D(const RpFile);
	return d->filename;
}

}
