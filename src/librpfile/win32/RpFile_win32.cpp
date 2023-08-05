/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpFile_win32.cpp: Standard file object. (Win32 implementation)          *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "../RpFile.hpp"
#include "../RpFile_p.hpp"

// libwin32common
#include "libwin32common/w32err.hpp"

// librptext
#include "librptext/conversion.hpp"
#include "librptext/wchar.hpp"

// C includes
#include <fcntl.h>

// C++ STL classes
using std::string;
using std::wstring;

#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRpFile {

#ifdef _MSC_VER
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#endif /* _MSC_VER */

/** RpFilePrivate **/

RpFilePrivate::RpFilePrivate(RpFile *q, const wchar_t *filenameW, RpFile::FileMode mode)
	: q_ptr(q), file(INVALID_HANDLE_VALUE), filename(nullptr)
	, mode(mode), gzfd(nullptr), gzsz(-1), devInfo(nullptr)
{
	assert(filenameW != nullptr);
	this->filenameW = wcsdup(filenameW);
}

RpFilePrivate::~RpFilePrivate()
{
	if (gzfd) {
		gzclose_r(gzfd);
	}
	if (file && file != INVALID_HANDLE_VALUE) {
		CloseHandle(file);
	}
	free(filename);
	free(filenameW);
	delete devInfo;
}

/**
 * Convert an RpFile::FileMode to Win32 CreateFile() parameters.
 * @param mode				[in] FileMode
 * @param pdwDesiredAccess		[out] dwDesiredAccess
 * @param pdwShareMode			[out] dwShareMode
 * @param pdwCreationDisposition	[out] dwCreationDisposition
 * @return 0 on success; non-zero on error.
 */
inline int RpFilePrivate::mode_to_win32(RpFile::FileMode mode,
	DWORD *pdwDesiredAccess,
	DWORD *pdwShareMode,
	DWORD *pdwCreationDisposition)
{
	switch (mode & RpFile::FM_MODE_MASK) {
		case RpFile::FM_OPEN_READ:
			*pdwDesiredAccess = GENERIC_READ;
			*pdwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
			*pdwCreationDisposition = OPEN_EXISTING;
			break;
		case RpFile::FM_OPEN_WRITE:
			*pdwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
			*pdwShareMode = FILE_SHARE_READ;
			*pdwCreationDisposition = OPEN_EXISTING;
			break;
		case RpFile::FM_CREATE|RpFile::FM_READ /*RpFile::FM_CREATE_READ*/ :
		case RpFile::FM_CREATE_WRITE:
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

/**
 * (Re-)Open the main file.
 *
 * INTERNAL FUNCTION. This does NOT affect gzfd.
 * NOTE: This function sets q->m_lastError.
 *
 * Uses parameters stored in this->filename and this->mode.
 * @return 0 on success; non-zero on error.
 */
int RpFilePrivate::reOpenFile(void)
{
	RP_Q(RpFile);
	if (!filenameW || filenameW[0] == L'\0') {
		// No filename...
		q->m_lastError = EINVAL;
		return -EINVAL;
	}

	// Determine the file mode.
	DWORD dwDesiredAccess = 0;
	DWORD dwShareMode = 0;
	DWORD dwCreationDisposition = 0;
	if (mode_to_win32(mode, &dwDesiredAccess, &dwShareMode, &dwCreationDisposition) != 0) {
		// Invalid mode.
		q->m_lastError = EINVAL;
		return -EINVAL;
	}

	// If the filename is "X:", change it to "X:\\".
	if (ISASCII(filenameW[0]) && ISALPHA(filenameW[0]) &&
	    filenameW[1] == L':' && filenameW[2] == L'\0')
	{
		// Drive letter. Append '\\'.
		wchar_t *drvfilename = static_cast<wchar_t*>(malloc(4*sizeof(wchar_t)));
		drvfilename[0] = filename[0];
		drvfilename[1] = _T(':');
		drvfilename[2] = _T('\\');
		drvfilename[3] = _T('\0');

		std::swap(drvfilename, this->filenameW);
		free(drvfilename);
		if (this->filename) {
			free(this->filename);
			this->filename = nullptr;
		}
	}

	// Adjusted filename for Windows.
	tstring tfilename;

	// Check if the path starts with a drive letter.
	// NEXT COMMIT TODO: Add ISDRIVELETTER() because ISASCII/ISALPHA only
	// handle 8-bit chars, and iswascii/iswalpha are affected by locale.
	if (ISASCII(filenameW[0]) && ISALPHA(filenameW[0]) &&
	    filenameW[1] == L':' && filenameW[2] == L'\\')
	{
		// Is it only a drive letter?
		if (filenameW[3] == L'\0') {
			// This is a drive letter.
			// Only CD-ROM (and similar) drives are supported.
			// TODO: Verify if opening by drive letter works,
			// or if we have to resolve the physical device name.
			// NOTE: filename is UTF-8, but we can use it as if
			// it's ANSI for a drive letter.
			const UINT driveType = GetDriveType(filenameW);
			switch (driveType) {
				case DRIVE_CDROM:
					// CD-ROM works.
					break;
				case DRIVE_UNKNOWN:
				case DRIVE_NO_ROOT_DIR:
					// No drive.
					q->m_lastError = ENODEV;
					return -ENODEV;
				default:
					// Not a CD-ROM drive.
					q->m_lastError = ENOTSUP;
					return -ENOTSUP;
			}

			// Create a raw device filename.
			// Reference: https://support.microsoft.com/en-us/help/138434/how-win32-based-applications-read-cd-rom-sectors-in-windows-nt
			tfilename = _T("\\\\.\\X:");
			tfilename[4] = filenameW[0];
			q->m_fileType = DT_BLK;	// this is a device
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

	if (q->m_fileType == DT_BLK) {
		// This is a device. Allocate devInfo.
		// NOTE: This is kept around until RpFile is deleted,
		// even if the device can't be opeend for some reason.
		devInfo = new DeviceInfo();

		if (mode & RpFile::FM_WRITE) {
			// Writing to block devices is not allowed.
			q->m_lastError = EINVAL;
			return -EINVAL;
		}
		// NOTE: We need WRITE permission for
		// DeviceIoControl() to function properly.
		dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
		dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	} else {
		// Not a device. Make sure this isn't a directory.
		// TODO: Other checks?
		const DWORD dwAttr = GetFileAttributes(tfilename.c_str());
		if (dwAttr == INVALID_FILE_ATTRIBUTES) {
			// File cannot be opened.
			// This is okay if creating a new file, but not if we're
			// opening an existing file.
			if (!(mode & RpFile::FM_CREATE)) {
				q->m_fileType = DT_UNKNOWN;
				q->m_lastError = EIO;
				return -EIO;
			}
		} else if (dwAttr & FILE_ATTRIBUTE_DIRECTORY) {
			// File is a directory.
			q->m_fileType = DT_DIR;
			q->m_lastError = EISDIR;
			return -EISDIR;
		}
	}

	// Open the file.
	if (file && file != INVALID_HANDLE_VALUE) {
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
	if (q->m_fileType == DT_BLK) {
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
		q->m_lastError = w32err_to_posix(GetLastError());
		return -q->m_lastError;
	}

	if (q->m_fileType == DT_BLK) {
		// Get the disk space.
		int ret = q->rereadDeviceSizeOS();
		if (ret != 0) {
			// An error occurred...
			q->m_lastError = w32err_to_posix(GetLastError());
			if (q->m_lastError == 0) {
				q->m_lastError = EIO;
			}
			CloseHandle(file);
			file = INVALID_HANDLE_VALUE;
			return -q->m_lastError;
		}
	}

	// Return 0 if it's *not* nullptr or INVALID_HANDLE_VALUE.
	return (!file || file == INVALID_HANDLE_VALUE);
}

/** RpFile **/

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename (UTF-8)
 * @param mode File mode
 */
RpFile::RpFile(const char *filename, FileMode mode)
	: super()
	, d_ptr(new RpFilePrivate(this, U82W_c(filename), mode))
{
	init();
}

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename (UTF-8)
 * @param mode File mode
 */
RpFile::RpFile(const string &filename, FileMode mode)
	: super()
	, d_ptr(new RpFilePrivate(this, U82W_s(filename), mode))
{
	init();
}

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename (UTF-16)
 * @param mode File mode
 */
RpFile::RpFile(const wchar_t *filenameW, FileMode mode)
	: super()
	, d_ptr(new RpFilePrivate(this, filenameW, mode))
{
	init();
}

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename (UTF-16)
 * @param mode File mode
 */
RpFile::RpFile(const wstring &filenameW, FileMode mode)
	: super()
	, d_ptr(new RpFilePrivate(this, filenameW.c_str(), mode))
{
	init();
}

/**
 * Common initialization function for RpFile's constructors.
 * Filename must be set in d->filename.
 */
void RpFile::init(void)
{
	RP_D(RpFile);

#ifndef NDEBUG
	// Cannot use decompression with writing.
	if (d->mode & RpFile::FM_GZIP_DECOMPRESS) {
		assert((d->mode & FM_MODE_MASK) != RpFile::FM_WRITE);
	}
#endif /* NDEBUG */

	// Open the file.
	if (d->reOpenFile() != 0) {
		// An error occurred while opening the file.
		return;
	}

	m_isWritable = !!(d->mode & RpFile::FM_WRITE);

	// Check if this is a gzipped file.
	// If it is, use transparent decompression.
	// Reference: https://www.forensicswiki.org/wiki/Gzip
	const bool tryGzip = (!d->devInfo && d->mode == FM_OPEN_READ_GZ);
	if (tryGzip) { do {
#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
		// Delay load verification.
		// TODO: Only if linked with /DELAYLOAD?
		if (DelayLoad_test_get_crc_table() != 0) {
			// Delay load failed.
			// Don't do any gzip checking.
			return;
		}
#else /* !defined(_MSC_VER) || !defined(ZLIB_IS_DLL) */
		// zlib isn't in a DLL, but we need to ensure that the
		// CRC table is initialized anyway.
		get_crc_table();
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

		DWORD bytesRead;
		uint16_t gzmagic;
		BOOL bRet = ReadFile(d->file, &gzmagic, sizeof(gzmagic), &bytesRead, nullptr);
		if (!bRet || bytesRead != sizeof(gzmagic) || gzmagic != be16_to_cpu(0x1F8B))
			break;

		// This is a gzipped file.
		// Get the uncompressed size at the end of the file.
		LARGE_INTEGER liFileSize;
		bRet = GetFileSizeEx(d->file, &liFileSize);
		if (!bRet || liFileSize.QuadPart <= 10+8)
			break;

		LARGE_INTEGER liSeekPos;
		liSeekPos.QuadPart = liFileSize.QuadPart - 4;
		if (!SetFilePointerEx(d->file, liSeekPos, nullptr, FILE_BEGIN))
			break;

		uint32_t uncomp_sz;
		bRet = ReadFile(d->file, &uncomp_sz, sizeof(uncomp_sz), &bytesRead, nullptr);
		uncomp_sz = le32_to_cpu(uncomp_sz);
		if (!bRet || bytesRead != sizeof(uncomp_sz) /*|| uncomp_sz < liFileSize.QuadPart-(10+8)*/)
			break;

		// NOTE: Uncompressed size might be smaller than the real filesize
		// in cases where gzip doesn't help much.
		// TODO: Add better verification heuristics?
		d->gzsz = (off64_t)uncomp_sz;

		liSeekPos.QuadPart = 0;
		SetFilePointerEx(d->file, liSeekPos, nullptr, FILE_BEGIN);
		// NOTE: Not sure if this is needed on Windows.
		FlushFileBuffers(d->file);

		// Open the file with gzdopen().
		HANDLE hGzDup;
		bRet = DuplicateHandle(
			GetCurrentProcess(),	// hSourceProcessHandle
			d->file,		// hSourceHandle
			GetCurrentProcess(),	// hTargetProcessHandle
			&hGzDup,		// lpTargetHandle
			0,			// dwDesiredAccess
			FALSE,			// bInheritHandle
			DUPLICATE_SAME_ACCESS);	// dwOptions
		if (!bRet)
			break;

		// NOTE: close() on gzfd_dup() will close the
		// underlying Windows handle.
		int gzfd_dup = _open_osfhandle((intptr_t)hGzDup, _O_RDONLY);
		if (gzfd_dup >= 0) {
			d->gzfd = gzdopen(gzfd_dup, "r");
			if (d->gzfd) {
				m_isCompressed = true;
			} else {
				// gzdopen() failed.
				// Close the dup()'d handle to prevent a leak.
				_close(gzfd_dup);
			}
		} else {
			// Unable to open an fd.
			CloseHandle(hGzDup);
		}
	} while (0); }

	if (tryGzip && !d->gzfd) {
		// Not a gzipped file.
		// Rewind and flush the file.
		LARGE_INTEGER liSeekPos;
		liSeekPos.QuadPart = 0;
		SetFilePointerEx(d->file, liSeekPos, nullptr, FILE_BEGIN);
		// NOTE: Not sure if this is needed on Windows.
		FlushFileBuffers(d->file);
	}
}

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
	return (d->file != nullptr && d->file != INVALID_HANDLE_VALUE);
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

	if (d->gzfd) {
		gzclose_r(d->gzfd);
		d->gzfd = nullptr;
	}
	if (d->file && d->file != INVALID_HANDLE_VALUE) {
		CloseHandle(d->file);
		d->file = INVALID_HANDLE_VALUE;
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
	RP_D(RpFile);
	if (!d->file || d->file == INVALID_HANDLE_VALUE) {
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
	if (d->gzfd) {
		int iret = gzread(d->gzfd, ptr, (unsigned int)size);
		if (iret >= 0) {
			bytesRead = (DWORD)iret;
		} else {
			// An error occurred.
			bytesRead = 0;
			m_lastError = errno;
		}
	} else {
		BOOL bRet = ReadFile(d->file, ptr, static_cast<DWORD>(size), &bytesRead, nullptr);
		if (!bRet) {
			// An error occurred.
			m_lastError = w32err_to_posix(GetLastError());
			bytesRead = 0;
		}
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
	if (!d->file || d->file == INVALID_HANDLE_VALUE || !(d->mode & FM_WRITE)) {
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
 * @param pos File position.
 * @return 0 on success; -1 on error.
 */
int RpFile::seek(off64_t pos)
{
	RP_D(RpFile);
	if (!d->file || d->file == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return -1;
	}

	if (d->devInfo) {
		// SetFilePointerEx() *requires* sector alignment when
		// accessing device files. Hence, we'll have to maintain
		// our own device position.
		if (pos < 0) {
			d->devInfo->device_pos = 0;
		} else if (pos <= d->devInfo->device_size) {
			d->devInfo->device_pos = pos;
		} else {
			d->devInfo->device_pos = d->devInfo->device_size;
		}
		return 0;
	}

	int ret;
	if (d->gzfd) {
		// FIXME: Might not work with >2GB files...
		z_off_t zret = gzseek(d->gzfd, (long)pos, SEEK_SET);
		if (zret >= 0) {
			ret = 0;
		} else {
			ret = -1;
			m_lastError = -errno;
			if (m_lastError == 0) {
				m_lastError = -EIO;
			}
		}
	} else {
		LARGE_INTEGER liSeekPos;
		liSeekPos.QuadPart = pos;
		BOOL bRet = SetFilePointerEx(d->file, liSeekPos, nullptr, FILE_BEGIN);
		if (bRet) {
			ret = 0;
		} else {
			ret = -1;
			m_lastError = w32err_to_posix(GetLastError());
		}
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
	if (!d->file || d->file == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return -1;
	}

	if (d->devInfo) {
		// SetFilePointerEx() *requires* sector alignment when
		// accessing device files. Hence, we'll have to maintain
		// our own device position.
		return d->devInfo->device_pos;
	}

	if (d->gzfd) {
		return (off64_t)gztell(d->gzfd);
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
	if (!d->file || d->file == INVALID_HANDLE_VALUE || !(d->mode & FM_WRITE)) {
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
	if (!d->file || d->file == INVALID_HANDLE_VALUE) {
		m_lastError = EBADF;
		return -1;
	}

	// TODO: Error checking?

	if (d->devInfo) {
		// Block device. Use the cached device size.
		return d->devInfo->device_size;
	} else if (d->gzfd) {
		// gzipped files have the uncompressed size stored
		// at the end of the stream.
		return d->gzsz;
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
	if (d->filename && d->filename[0] != '\0') {
		return d->filename;
	} else if (d->filenameW && d->filenameW[0] != L'\0') {
		const_cast<RpFilePrivate*>(d)->filename = strdup(W2U8(d->filenameW).c_str());
		return d->filename;
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
	return (d->filenameW != nullptr && d->filenameW[0] != L'\0') ? d->filenameW : nullptr;
}

/** Extra functions **/

/**
 * Make the file writable.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpFile::makeWritable(void)
{
	if (isCompressed()) {
		// File is compressed. Cannot make it writable.
		return -ENOTSUP;
	} else if (isWritable()) {
		// File is already writable.
		return 0;
	}

	RP_D(RpFile);
	const off64_t prev_pos = this->tell();
	// Set file mode to FM_WRITE and reopen it.
	d->mode = (RpFile::FileMode)(d->mode | FM_WRITE);
	int ret = d->reOpenFile();
	if (!ret) {
		// File is now writable.
		m_isWritable = true;
	} else {
		// Failed to open the file as writable.
		// Try reopening as read-only.
		d->mode = (RpFile::FileMode)(d->mode & ~FM_WRITE);
		if (d->reOpenFile() != 0) {
			this->seek(prev_pos);
		}
		return -ENOTSUP;
	}

	// Restore the seek position.
	this->seek(prev_pos);
	return 0;
}

}
