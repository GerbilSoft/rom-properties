/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpFile_win32.cpp: Standard file object. (Win32 implementation)          *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "../RpFile.hpp"
#include "../RpFile_p.hpp"

// libwin32common
#include "libwin32common/w32err.hpp"
#include "libwin32common/w32time.h"
#include "libwin32common/MiniU82T.hpp"
using namespace LibWin32Common;

// librpbyteswap
#include "librpbyteswap/byteswap_rp.h"

// C includes
#include <fcntl.h>

// C++ STL classes
using std::string;
using std::string_view;
using std::wstring;
using std::wstring_view;

// tcharx
#include "tcharx.h"

namespace LibRpFile {

/** RpFilePrivate **/

RpFilePrivate::RpFilePrivate(RpFile *q, wstring_view filenameW, IRpFile::FileMode mode)
	: q_ptr(q)
	, file(nullptr)
	, mode(mode)
{
	assert(!filenameW.empty());
	WSTRING_ASSIGN_WSV(this->filenameW, filenameW);
}

RpFilePrivate::~RpFilePrivate()
{
	if (file) {
		CloseHandle(file);
	}
}

/**
 * Convert an IRpFile::FileMode to Win32 CreateFile() parameters.
 * @param mode				[in] FileMode
 * @param pdwDesiredAccess		[out] dwDesiredAccess
 * @param pdwShareMode			[out] dwShareMode
 * @param pdwCreationDisposition	[out] dwCreationDisposition
 * @return 0 on success; non-zero on error.
 */
inline int RpFilePrivate::mode_to_win32(IRpFile::FileMode mode,
	DWORD *pdwDesiredAccess,
	DWORD *pdwShareMode,
	DWORD *pdwCreationDisposition)
{
	switch (mode & IRpFile::FM_MODE_MASK) {
		case IRpFile::FM_OPEN_READ:
			*pdwDesiredAccess = GENERIC_READ;
			*pdwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
			*pdwCreationDisposition = OPEN_EXISTING;
			break;
		case IRpFile::FM_OPEN_WRITE:
			*pdwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
			*pdwShareMode = FILE_SHARE_READ;
			*pdwCreationDisposition = OPEN_EXISTING;
			break;
		case IRpFile::FM_CREATE | IRpFile::FM_READ /*RpFile::FM_CREATE_READ*/ :
		case IRpFile::FM_CREATE_WRITE:
			*pdwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
			*pdwShareMode = FILE_SHARE_READ;
			*pdwCreationDisposition = CREATE_ALWAYS;
			break;
		default:
			// Invalid mode.
			return -1;
	}

	// Mode converted successfully.
	return 0;
}

template<typename CharType>
static constexpr inline bool isBackslashOrSlash(CharType c)
{
	return (c == static_cast<CharType>('\\') ||
	        c == static_cast<CharType>('/'));
}

/**
 * (Re-)Open the main file.
 * @param mode		[in] Requested file mode
 * @param p_isWritable	[out] Indicates if the opened stream is writable
 * @param p_err		[out] 0 on success; positive POSIX error code on error
 * @return File handle, or NULL on error.
 */
HANDLE RpFilePrivate::reOpenFile(IRpFile::FileMode mode, bool *p_isWritable, int *p_err)
{
	RP_Q(RpFile);	// for q->m_fileType

	assert(p_isWritable != nullptr);
	assert(p_err != nullptr);

	// Initialize the output variables.
	*p_isWritable = false;
	*p_err = 0;

	// Make sure we have a filename and that the file can be opened.
	assert(!filenameW.empty());
	assert(canOpenFile);
	if (unlikely(filenameW.empty()) || !canOpenFile) {
		// No filename, or can't open the file...
		*p_err = EBADF;
		return nullptr;
	}

	// Determine the file mode.
	DWORD dwDesiredAccess = 0;
	DWORD dwShareMode = 0;
	DWORD dwCreationDisposition = 0;
	if (mode_to_win32(mode, &dwDesiredAccess, &dwShareMode, &dwCreationDisposition) != 0) {
		// Invalid mode.
		*p_err = EINVAL;
		return nullptr;
	}

	// Adjusted filename for Windows.
	tstring tfilename;
	uint8_t fileType = DT_UNKNOWN;

	// Check if the path starts with a drive letter.
	if (IsDriveLetterW(filenameW[0]) &&
	    filenameW[1] == L':' && isBackslashOrSlash(filenameW[2]))
	{
		// Is it only a drive letter?
		if (filenameW[3] == L'\0') {
			// This is a drive letter.
			// Only CD-ROM (and similar) drives are supported.
			// TODO: Verify if opening by drive letter works,
			// or if we have to resolve the physical device name.
			// NOTE: filename is UTF-8, but we can use it as if
			// it's ANSI for a drive letter.
#ifdef UNICODE
			const UINT driveType = GetDriveType(filenameW.c_str());
#else /* !UNICODE */
			const char filenameA[4] = {(char)filenameW[0], ':', '\\', '\0'};
			const UINT driveType = GetDriveType(filenameA);
#endif /* UNICODE */
			switch (driveType) {
				case DRIVE_CDROM:
					// CD-ROM works.
					break;
				case DRIVE_UNKNOWN:
				case DRIVE_NO_ROOT_DIR:
					// No drive.
					*p_err = ENODEV;
					return nullptr;
				default:
					// Not a CD-ROM drive.
					*p_err = ENOTSUP;
					return nullptr;
			}

			// Create a raw device filename.
			// Reference: https://support.microsoft.com/en-us/help/138434/how-win32-based-applications-read-cd-rom-sectors-in-windows-nt
			tfilename = _T("\\\\.\\X:");
			tfilename[4] = filenameW[0];
			fileType = DT_BLK;	// this is a device
		} else {
			// Absolute path: Prepend "\\?\" in order to support filenames longer than MAX_PATH.
			tfilename = _T("\\\\?\\");
			tfilename += filenameW;
		}
	} else {
		// Not an absolute path, or "\\?\" is already
		// prepended. Use it as-is.
		tfilename = filenameW;
	}

	// Save the file type if it isn't already set.
	if (q->m_fileType == DT_UNKNOWN) {
		// q->m_fileType isn't set. First reOpenFile() call!
		if (fileType != DT_BLK) {
			// Make sure this isn't a directory.
			// TODO: Other checks?
			const DWORD dwAttr = GetFileAttributes(tfilename.c_str());
			if (dwAttr == INVALID_FILE_ATTRIBUTES) {
				// File cannot be opened.
				// This is okay if creating a new file, but not if we're
				// opening an existing file.
				if (!(mode & RpFile::FM_CREATE)) {
					int err = w32err_to_posix(GetLastError());
					if (err == 0) {
						err = EIO;
					}
					q->m_fileType = DT_UNKNOWN;
					*p_err = err;
					return nullptr;
				}
			} else if (dwAttr & FILE_ATTRIBUTE_DIRECTORY) {
				// File is a directory.
				q->m_fileType = DT_DIR;
				*p_err = EISDIR;
				return nullptr;
			} else {
				// Regular file.
				fileType = DT_REG;
			}
		}

		q->m_fileType = fileType;
	} else {
		// File type is already set. Load it.
		fileType = q->m_fileType;
	}

	if (fileType == DT_BLK) {
		if (mode & RpFile::FM_WRITE) {
			// Writing to block devices is not allowed.
			*p_err = EINVAL;
			return nullptr;
		}

		// NOTE: We need WRITE permission for
		// DeviceIoControl() to function properly.
		dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
		dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	}

	// Open the file.
	if (file) {
		CloseHandle(file);
	}
	file = CreateFile(
		tfilename.c_str(),	// lpFileName
		dwDesiredAccess,	// dwDesiredAccess
		dwShareMode,		// dwShareMode
		nullptr,		// lpSecurityAttributes
		dwCreationDisposition,	// dwCreationDisposition
		FILE_ATTRIBUTE_NORMAL,	// dwFlagsAndAttributes
		nullptr);		// hTemplateFile
	if (fileType == DT_BLK) {
		if (!file || file == INVALID_HANDLE_VALUE) {
			// Try again without WRITE permission.
			file = CreateFile(
				tfilename.c_str(),	// lpFileName
				GENERIC_READ,		// dwDesiredAccess
				FILE_SHARE_READ,	// dwShareMode
				nullptr,		// lpSecurityAttributes
				dwCreationDisposition,	// dwCreationDisposition
				FILE_ATTRIBUTE_NORMAL,	// dwFlagsAndAttributes
				nullptr);		// hTemplateFile
		}
	}
	if (!file || file == INVALID_HANDLE_VALUE) {
		// Error opening the file.
		*p_err = w32err_to_posix(GetLastError());
		return nullptr;
	}

	// Return the file handle.
	return file;
}

/** RpFile **/

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename (UTF-16)
 * @param mode File mode
 */
RpFile::RpFile(wstring_view filenameW, FileMode mode)
	: d_ptr(new RpFilePrivate(this, filenameW, mode))
{
	RP_D(RpFile);

	// If the filename is "X:", change it to "X:\\".
	if (IsDriveLetterW(d->filenameW[0]) &&
	    d->filenameW[1] == L':' && d->filenameW[2] == L'\0')
	{
		// Drive letter. Append '\\'.
		const wchar_t drvfilename[4] = {d->filenameW[0], L':', L'\\', L'\0'};
		d->filenameW.assign(drvfilename);
		d->filename.clear();
	}

	// Open the file.
	// NOTE: m_fileType is set by reOpenFile() if it's DT_UNKNOWN.
	m_fileType = DT_UNKNOWN;
	d->canOpenFile = true;
	d->file = d->reOpenFile(d->mode, &m_isWritable, &m_lastError);
	if (!d->file || d->file == INVALID_HANDLE_VALUE) {
		// An error occurred while opening the file.
		return;
	}

	if (isDevice()) {
		// This is a device. Allocate devInfo.
		// NOTE: This is kept around until RpFile is deleted,
		// even if the device can't be opeend for some reason.
		d->devInfo.reset(new RpFilePrivate::DeviceInfo());

		// Get the disk space.
		int ret = rereadDeviceSizeOS();
		if (ret != 0) {
			// An error occurred.
			DWORD dwErr = GetLastError();
			if (dwErr == ERROR_NOT_READY) {
				// No media is present.
				// We'll still keep the file open for e.g. SCSI INQUIRY commands.
			} else {
				// Some other error.
				m_lastError = ret;
				if (m_lastError == 0) {
					m_lastError = EIO;
				}
				CloseHandle(d->file);
				d->file = nullptr;
				return;
			}
		}
	}
}

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename (UTF-8)
 * @param mode File mode
 */
RpFile::RpFile(string_view filename, FileMode mode)
	: RpFile(U82W_str(filename), mode)
{}

RpFile::~RpFile()
{
	delete d_ptr;
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpFile::isOpen(void) const
{
	RP_D(const RpFile);
	return (d->file != nullptr);
}

/**
 * Close the file.
 */
void RpFile::close(void)
{
	RP_D(RpFile);

	// NOTE: devInfo is not deleted here,
	// since the properties may still be used.
	// We *will* close any handles and free the
	// sector cache, though.
	if (d->devInfo) {
		d->devInfo->close();
	}

	if (d->file) {
		CloseHandle(d->file);
		d->file = nullptr;
	}
}

/**
 * Are we at EOF?
 * @return True if EOF; false if not.
 */
bool RpFile::eof(void)
{
	RP_D(RpFile);
	if (!d->file) {
		m_lastError = EBADF;
		return true;
	}

	// Check if the current position is >= the file size.

	// Get the current file position.
	LARGE_INTEGER liPos;
	if (d->devInfo) {
		// SetFilePointerEx() *requires* sector alignment when
		// accessing device files. Hence, we'll have to maintain
		// our own device position.
		liPos.QuadPart = d->devInfo->device_pos;
	} else {
		LARGE_INTEGER liSeekPos;
		liSeekPos.QuadPart = 0;
		BOOL bRet = SetFilePointerEx(d->file, liSeekPos, &liPos, FILE_CURRENT);
		if (!bRet) {
			m_lastError = w32err_to_posix(GetLastError());
			return true;
		}
	}

	// Get the file size.
	LARGE_INTEGER liFileSize;
	if (!GetFileSizeEx(d->file, &liFileSize)) {
		// Could not get the file size.
		m_lastError = w32err_to_posix(GetLastError());
		return true;
	}

	return (liPos.QuadPart >= liFileSize.QuadPart);
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
	if (!d->file) {
		m_lastError = EBADF;
		return 0;
	} else if (size == 0) {
		// Nothing to read.
		return 0;
	}

	if (d->devInfo) {
		// Block device. Need to read in multiples of the block size.
		return d->readUsingBlocks(ptr, size);
	}

	DWORD bytesRead;
	BOOL bRet = ReadFile(d->file, ptr, static_cast<DWORD>(size), &bytesRead, nullptr);
	if (!bRet) {
		// An error occurred.
		m_lastError = w32err_to_posix(GetLastError());
		bytesRead = 0;
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
	if (!d->file || !(d->mode & FM_WRITE)) {
		// Either the file isn't open,
		// or it's read-only.
		m_lastError = EBADF;
		return 0;
	}

	if (d->devInfo) {
		// Writing to block devices is not allowed.
		m_lastError = EBADF;
		return 0;
	}

	DWORD bytesWritten;
	BOOL bRet = WriteFile(d->file, ptr, static_cast<DWORD>(size), &bytesWritten, nullptr);
	if (!bRet) {
		// An error occurred.
		m_lastError = w32err_to_posix(GetLastError());
		return 0;
	}

	return bytesWritten;
}

/**
 * Set the file position.
 * @param pos		[in] File position
 * @param whence	[in] Where to seek from
 * @return 0 on success; -1 on error.
 */
int RpFile::seek(off64_t pos, SeekWhence whence)
{
	RP_D(RpFile);
	if (!d->file) {
		m_lastError = EBADF;
		return -1;
	}

	if (d->devInfo) {
		// SetFilePointerEx() *requires* sector alignment when
		// accessing device files. Hence, we'll have to maintain
		// our own device position.
		pos = adjust_file_pos_for_whence(pos, whence, d->devInfo->device_pos, d->devInfo->device_size);
		d->devInfo->device_pos = constrain_file_pos(pos, d->devInfo->device_size);
		return 0;
	}

	int ret;

	LARGE_INTEGER liSeekPos;
	liSeekPos.QuadPart = pos;
	BOOL bRet = SetFilePointerEx(d->file, liSeekPos, nullptr, static_cast<DWORD>(whence));
	if (bRet) {
		ret = 0;
	} else {
		ret = -1;
		m_lastError = w32err_to_posix(GetLastError());
	}

	return ret;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
off64_t RpFile::tell(void)
{
	RP_D(RpFile);
	if (!d->file) {
		m_lastError = EBADF;
		return -1;
	}

	if (d->devInfo) {
		// SetFilePointerEx() *requires* sector alignment when
		// accessing device files. Hence, we'll have to maintain
		// our own device position.
		return d->devInfo->device_pos;
	}

	LARGE_INTEGER liSeekPos, liSeekRet;
	liSeekPos.QuadPart = 0;
	BOOL bRet = SetFilePointerEx(d->file, liSeekPos, &liSeekRet, FILE_CURRENT);
	if (!bRet) {
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
int RpFile::truncate(off64_t size)
{
	RP_D(RpFile);
	if (!d->file || !(d->mode & FM_WRITE)) {
		// Either the file isn't open,
		// or it's read-only.
		m_lastError = EBADF;
		return -1;
	} else if (size < 0) {
		m_lastError = EINVAL;
		return -1;
	}

	if (d->devInfo) {
		// Operation not supported.
		m_lastError = ENOTSUP;
		return -1;
	}

	// Set the requested end of file, and
	// get the current file position.
	LARGE_INTEGER liSeekPos, liSeekRet;
	liSeekPos.QuadPart = size;
	BOOL bRet = SetFilePointerEx(d->file, liSeekPos, &liSeekRet, FILE_CURRENT);
	if (!bRet) {
		m_lastError = w32err_to_posix(GetLastError());
		return -1;
	}

	// Truncate the file.
	bRet = SetEndOfFile(d->file);
	if (!bRet) {
		m_lastError = w32err_to_posix(GetLastError());
		return -1;
	}

	// Restore the original position if it was
	// less than the new size.
	if (liSeekRet.QuadPart < size) {
		bRet = SetFilePointerEx(d->file, liSeekRet, nullptr, FILE_BEGIN);
		if (!bRet) {
			m_lastError = w32err_to_posix(GetLastError());
			return -1;
		}
	}

	// File truncated.
	return 0;
}

/**
 * Flush buffers.
 * This operation only makes sense on writable files.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpFile::flush(void)
{
	if (isWritable()) {
		RP_D(RpFile);
		BOOL bRet = FlushFileBuffers(d->file);
		if (!bRet) {
			m_lastError = w32err_to_posix(GetLastError());
			return -m_lastError;
		}
		return 0;
	}

	// Ignore flush operations if the file isn't writable.
	return 0;
}

/** File properties **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
off64_t RpFile::size(void)
{
	RP_D(RpFile);
	if (!d->file) {
		m_lastError = EBADF;
		return -1;
	}

	// TODO: Error checking?

	if (d->devInfo) {
		// Block device. Use the cached device size.
		return d->devInfo->device_size;
	}

	// Regular file.
	LARGE_INTEGER liFileSize;
	if (!GetFileSizeEx(d->file, &liFileSize)) {
		// Could not get the file size.
		m_lastError = w32err_to_posix(GetLastError());
		return -1;
	}

	// Return the file/device size.
	return liFileSize.QuadPart;
}

/**
 * Get the filename.
 * @return Filename (UTF-8) (May be nullptr if the filename is not available.)
 */
const char *RpFile::filename(void) const
{
	RP_D(const RpFile);
	if (unlikely(!d->filename.empty())) {
		return d->filename.c_str();
	} else if (likely(!d->filenameW.empty())) {
		const_cast<RpFilePrivate*>(d)->filename = W2U8_str(d->filenameW);
		return d->filename.c_str();
	}

	return nullptr;
}

/**
 * Get the filename.
 * Get the filename. (Windows only: returns UTF-16.)
 * @return Filename (UTF-16) (May be nullptr if the filename is not available.)
 */
const wchar_t *RpFile::filenameW(void) const
{
	RP_D(const RpFile);
	return (likely(!d->filenameW.empty())) ? d->filenameW.c_str() : nullptr;
}

/**
 * Get the file modification time.
 * @return File modification time, or -1 if not available.
 */
rp_time_t RpFile::mtime(void)
{
	FILETIME lastWriteTime;
	int ret = mtime(&lastWriteTime);
	if (ret != 0) {
		// mtime(FILETIME*) failed...
		return -1;
	}

	// Convert FILETIME to Unix time.
	return FileTimeToUnixTime(lastWriteTime);
}

/**
 * Get the file modification time. (FILETIME format)
 * @param mtime Output buffer for file modification time. (If not available, -1 will be written.)
 * @return 0 on success; negative POSIX error code on error.
 */
RP_LIBROMDATA_PUBLIC
int RpFile::mtime(FILETIME *mtime)
{
	RP_D(RpFile);
	if (!d->file) {
		m_lastError = EBADF;
		return -EBADF;
	} else if (!mtime) {
		m_lastError = EINVAL;
		return -EINVAL;
	}

	FILETIME lastWriteTime;
	BOOL bRet = GetFileTime(d->file, nullptr, nullptr, &lastWriteTime);
	if (!bRet) {
		m_lastError = w32err_to_posix(GetLastError());
		if (m_lastError == 0) {
			m_lastError = EIO;
		}
		return -m_lastError;
	}

	*mtime = lastWriteTime;
	return 0;
}

/** Extra functions **/

/**
 * Make the file writable.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpFile::makeWritable(void)
{
	int ret = makeWritable_basic_checks();
	if (ret > 0) {
		// Already writable.
		return 0;
	} else if (ret != 0) {
		// Cannot make writable.
		return ret;
	}

	// Attempt to open the file a second time without closing the existing file pointer.
	// Should work on most systems...
	RP_D(RpFile);
	bool isWritable = false;
	int err = 0;
	HANDLE f_new = d->reOpenFile(IRpFile::FM_OPEN_WRITE, &isWritable, &err);
	if (!f_new) {
		// Failed to open the file as writable...
		m_lastError = err;
		return -err;
	}

	if (!isWritable) {
		// Newly-opened file isn't writable. There's no point in using it.
		CloseHandle(f_new);
		m_lastError = EROFS;	// TODO: Better error code?
		return -EROFS;
	}

	// Copy the seek pointer from the old FILE*.
	// NOTE: If the old HANDLE is nullptr, we'll assume pos == 0.
	LARGE_INTEGER liCurPos;
	liCurPos.QuadPart = 0;
	if (d->file && d->file != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER liSeekPos;
		liSeekPos.QuadPart = 0;
		BOOL bRet = SetFilePointerEx(d->file, liSeekPos, &liCurPos, FILE_CURRENT);
		if (!bRet) {
			// SetFilePointerEx() to get the current file position failed.
			m_lastError = w32err_to_posix(GetLastError());
			CloseHandle(f_new);
			return -m_lastError;;
		}
	}

	BOOL bRet = SetFilePointerEx(f_new, liCurPos, nullptr, FILE_BEGIN);
	if (!bRet) {
		// SetFilePointerEx() to seek in the new file failed.
		m_lastError = w32err_to_posix(GetLastError());
		CloseHandle(f_new);
		return -m_lastError;
	}

	// Close the old file pointer and save the new one.
	if (d->file && d->file != INVALID_HANDLE_VALUE) {
		CloseHandle(d->file);
	}
	d->file = f_new;
	d->mode = static_cast<FileMode>(d->mode | FM_WRITE);
	m_isWritable = isWritable;
	return 0;
}

} // namespace LibRpFile
