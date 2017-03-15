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
#include <cassert>
#include <cctype>

// C++ includes.
#include <memory>
#include <string>
using std::string;
using std::unique_ptr;
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
		RpFilePrivate(RpFile *q, const rp_char *filename, RpFile::FileMode mode);
		RpFilePrivate(RpFile *q, const rp_string &filename, RpFile::FileMode mode);

	private:
		RP_DISABLE_COPY(RpFilePrivate)
		RpFile *const q_ptr;

	public:
		// NOTE: file is a HANDLE, but shared_ptr doesn't
		// work correctly with pointer types as the
		// template parameter.
		std::shared_ptr<void> file;
		rp_string filename;
		RpFile::FileMode mode;

		// Block device parameters.
		// NOTE: These fields are all 0 if the
		// file is a regular file.
		int64_t device_size;		// Device size.
		unsigned int sector_size;	// Sector size. (bytes per sector)

	public:
		static inline int mode_to_win32(RpFile::FileMode mode,
			DWORD *pdwDesiredAccess,
			DWORD *pdwCreationDisposition);

		/**
		 * Read using block reads.
		 * Required for block devices.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		size_t readUsingBlocks(void *ptr, size_t size);
};

/** RpFilePrivate **/

RpFilePrivate::RpFilePrivate(RpFile *q, const rp_char *filename, RpFile::FileMode mode)
	: q_ptr(q)
	, filename(filename)
	, mode(mode)
	, device_size(0)
	, sector_size(0)
{ }

RpFilePrivate::RpFilePrivate(RpFile *q, const rp_string &filename, RpFile::FileMode mode)
	: q_ptr(q)
	, filename(filename)
	, mode(mode)
	, device_size(0)
	, sector_size(0)
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
 * Read using block reads.
 * Required for block devices.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpFilePrivate::readUsingBlocks(void *ptr, size_t size)
{
	assert(device_size > 0);
	assert(sector_size >= 512);
	if (device_size <= 0 || sector_size < 512) {
		// Not a block device...
		return 0;
	}

	uint8_t *ptr8 = static_cast<uint8_t*>(ptr);
	size_t ret = 0;

	RP_Q(RpFile);
	int64_t pos = q->tell();

	// Are we already at the end of the block device?
	if (pos >= device_size) {
		// End of the block device.
		return 0;
	}

	// Make sure d->pos + size <= d->device_size.
	// If it isn't, we'll do a short read.
	if (pos + (int64_t)size >= device_size) {
		size = (size_t)(device_size - pos);
	}

	// Seek to the beginning of the first block.
	// TODO: Make sure sector_size is a power of 2.
	int seek_ret = q->seek(pos & ~((int64_t)sector_size - 1));
	if (seek_ret != 0) {
		// Seek error.
		return 0;
	}

	// Sector buffer.
	unique_ptr<uint8_t[]> sector_buffer;

	// Check if we're not starting on a block boundary.
	const uint32_t blockStartOffset = pos % sector_size;
	if (blockStartOffset != 0) {
		// Not a block boundary.
		// Read the end of the first block.
		if (!sector_buffer) {
			sector_buffer.reset(new uint8_t[sector_size]);
		}

		// Read the first block.
		DWORD bytesRead;
		BOOL bRet = ReadFile(file.get(), sector_buffer.get(), sector_size, &bytesRead, nullptr);
		if (bRet == 0 || bytesRead != sector_size) {
			// Read error.
			q->m_lastError = w32err_to_posix(GetLastError());
			return (bytesRead > 0 ? bytesRead : 0);
		}

		// Copy the data from the sector buffer.
		uint32_t read_sz = sector_size - blockStartOffset;
		if (size < (size_t)read_sz) {
			read_sz = (uint32_t)size;
		}
		memcpy(ptr8, &sector_buffer[blockStartOffset], read_sz);

		// Starting block read.
		size -= read_sz;
		ptr8 += read_sz;
		ret += read_sz;
	} else {
		// Seek to the beginning of the first block.
		seek_ret = q->seek(pos);
		if (seek_ret != 0) {
			// Seek error.
			return 0;
		}
	}

	// Must be on a sector boundary now.
	assert(q->tell() % sector_size == 0);

	// Read entire blocks.
	for (; size >= sector_size;
	    size -= sector_size, ptr8 += sector_size,
	    ret += sector_size)
	{
		// Read the next block.
		// FIXME: Read all of the contiguous blocks at once.
		DWORD bytesRead;
		BOOL bRet = ReadFile(file.get(), ptr8, sector_size, &bytesRead, nullptr);
		if (bRet == 0 || bytesRead != sector_size) {
			// Read error.
			q->m_lastError = w32err_to_posix(GetLastError());
			return ret + (bytesRead > 0 ? bytesRead : 0);
		}
	}

	// Check if we still have data left. (not a full block)
	if (size > 0) {
		if (!sector_buffer) {
			sector_buffer.reset(new uint8_t[sector_size]);
		}

		// Read the last block.
		pos = q->tell();
		assert(pos % sector_size == 0);
		DWORD bytesRead;
		BOOL bRet = ReadFile(file.get(), sector_buffer.get(), sector_size, &bytesRead, nullptr);
		if (bRet == 0 || bytesRead != sector_size) {
			// Read error.
			q->m_lastError = w32err_to_posix(GetLastError());
			return ret + (bytesRead > 0 ? bytesRead : 0);
		}

		// Copy the data from the sector buffer.
		memcpy(ptr8, sector_buffer.get(), size);

		ret += size;
	}

	// Finished reading the data.
	return ret;
}

/** RpFile **/

/**
 * Open a file.
 * NOTE: Files are always opened in binary mode.
 * @param filename Filename.
 * @param mode File mode.
 */
RpFile::RpFile(const rp_char *filename, FileMode mode)
	: super()
	, d_ptr(new RpFilePrivate(this, filename, mode))
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
	, d_ptr(new RpFilePrivate(this, filename, mode))
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

	// Check if the path starts with a drive letter.
	bool isBlockDevice = false;
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
			isBlockDevice = true;
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

	if (isBlockDevice && (d->mode & FM_WRITE)) {
		// Writing to block devices is not allowed.
		m_lastError = EINVAL;
		return;
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

	if (isBlockDevice) {
		// Get the disk space.
		// NOTE: IOCTL_DISK_GET_DRIVE_GEOMETRY_EX seems to report 512-byte sectors
		// for certain emulated CD-ROM device, e.g. the Verizon LG G2.
		// GetDiskFreeSpace() reports the correct value (2048).
		DWORD dwSectorsPerCluster, dwBytesPerSector;
		DWORD dwNumberOfFreeClusters, dwTotalNumberOfClusters;
		DWORD w32err = 0;
		BOOL bRet = GetDiskFreeSpace(RP2W_s(d->filename),
			&dwSectorsPerCluster, &dwBytesPerSector,
			&dwNumberOfFreeClusters, &dwTotalNumberOfClusters);
		if (bRet && dwBytesPerSector >= 512 && dwTotalNumberOfClusters > 0) {
			// TODO: Make sure the sector size is a power of 2
			// and isn't a ridiculous value.

			// Save the device size and sector size.
			// NOTE: GetDiskFreeSpaceEx() eliminates the need for multiplications,
			// but it doesn't provide dwBytesPerSector.
			d->device_size = (dwBytesPerSector * dwSectorsPerCluster) *
					 (int64_t)dwTotalNumberOfClusters;
			d->sector_size = dwBytesPerSector;
		} else {
			// GetDiskFreeSpace() failed.
			DWORD w32err = GetLastError();
			if (w32err == ERROR_INVALID_PARAMETER) {
				// The disk may use some file system that
				// Windows doesn't recognize.
				// Try IOCTL_DISK_GET_DRIVE_GEOMETRY_EX instead.
				DISK_GEOMETRY_EX dg;
				DWORD dwBytesReturned;  // TODO: Check this?
				if (DeviceIoControl(d->file.get(), IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
				    NULL, 0, &dg, sizeof(dg), &dwBytesReturned, NULL) != 0)
				{
					// Device geometry retrieved.
					w32err = 0;
					d->device_size = dg.DiskSize.QuadPart;
					d->sector_size = dg.Geometry.BytesPerSector;
				} else {
					// IOCTL failed.
					w32err = GetLastError();
					if (w32err == 0) {
						w32err = ERROR_INVALID_PARAMETER;
					}
				}
			}
		}

		if (w32err != 0) {
			// An error occurred...
			m_lastError = w32err_to_posix(GetLastError());
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			d->file.reset(INVALID_HANDLE_VALUE, myFile_deleter());
		}
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
	, d_ptr(new RpFilePrivate(this, other.d_ptr->filename, other.d_ptr->mode))
{
	RP_D(RpFile);
	d->file = other.d_ptr->file;
	d->device_size = other.d_ptr->device_size;
	d->sector_size = other.d_ptr->sector_size;
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
	d->device_size = other.d_ptr->device_size;
	d->sector_size = other.d_ptr->sector_size;
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
	} else if (size == 0) {
		// Nothing to read.
		return 0;
	}

	if (d->sector_size != 0) {
		// Block device. Need to read in multiples of the block size.
		return d->readUsingBlocks(ptr, size);
	}

	DWORD bytesRead;
	BOOL bRet = ReadFile(d->file.get(), ptr, (DWORD)size, &bytesRead, nullptr);
	if (!bRet) {
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

	if (d->sector_size != 0) {
		// Writing to block devices is not allowed.
		m_lastError = EBADF;
		return 0;
	}

	DWORD bytesWritten;
	BOOL bRet = WriteFile(d->file.get(), ptr, (DWORD)size, &bytesWritten, nullptr);
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
	if (!bRet) {
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
	if (!bRet) {
		m_lastError = w32err_to_posix(GetLastError());
		return -1;
	}

	// Truncate the file.
	bRet = SetEndOfFile(d->file.get());
	if (!bRet) {
		m_lastError = w32err_to_posix(GetLastError());
		return -1;
	}

	// Restore the original position if it was
	// less than the new size.
	if (liSeekRet.QuadPart < size) {
		bRet = SetFilePointerEx(d->file.get(), liSeekRet, nullptr, FILE_BEGIN);
		if (!bRet) {
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

	if (d->device_size != 0) {
		// Block device. Use the cached device size.
		return d->device_size;
	}

	// Regular file.
	LARGE_INTEGER liFileSize;
	if (!GetFileSizeEx(d->file.get(), &liFileSize)) {
		// Could not get the file size.
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
