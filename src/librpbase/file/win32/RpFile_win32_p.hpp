/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_win32_p.hpp: Standard file object. (Win32 implementation)        *
 * (PRIVATE CLASS)                                                         *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "../RpFile.hpp"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"

// C++ includes.
#include <string>
using std::string;

// zlib for transparent gzip decompression.
#include <zlib.h>
// gzclose_r() and gzclose_w() were introduced in zlib-1.2.4.
#if (ZLIB_VER_MAJOR > 1) || \
    (ZLIB_VER_MAJOR == 1 && ZLIB_VER_MINOR > 2) || \
    (ZLIB_VER_MAJOR == 1 && ZLIB_VER_MINOR == 2 && ZLIB_VER_REVISION >= 4)
// zlib-1.2.4 or later
#else
# define gzclose_r(file) gzclose(file)
# define gzclose_w(file) gzclose(file)
#endif

// Windows SDK
#include <windows.h>
#include <winioctl.h>
#include <io.h>

// TODO: Combine with RpFile_stdio_p.hpp,
// since these are basically the same except
// for INVALID_HANDLE_VALUE and HANDLE vs. FILE*.

namespace LibRpBase {

/** RpFilePrivate **/

class RpFilePrivate
{
	public:
		RpFilePrivate(RpFile *q, const char *filename, RpFile::FileMode mode)
			: q_ptr(q), file(INVALID_HANDLE_VALUE), filename(filename)
			, mode(mode), gzfd(nullptr), gzsz(-1), devInfo(nullptr) { }
		RpFilePrivate(RpFile *q, const string &filename, RpFile::FileMode mode)
			: q_ptr(q), file(INVALID_HANDLE_VALUE), filename(filename)
			, mode(mode), gzfd(nullptr), gzsz(-1), devInfo(nullptr) { }
		~RpFilePrivate();

	private:
		RP_DISABLE_COPY(RpFilePrivate)
		RpFile *const q_ptr;

	public:
		HANDLE file;		// File handle.
		string filename;	// Filename.
		RpFile::FileMode mode;	// File mode.

		gzFile gzfd;		// Used for transparent gzip decompression.
		int64_t gzsz;		// Uncompressed file size.

		// Device information struct.
		// Only used if the underlying file
		// is a device node.
		struct DeviceInfo {
			// SetFilePointerEx() *requires* sector alignment when
			// accessing device files. Hence, we'll have to maintain
			// our own device position.
			int64_t device_pos;		// Device position.
			int64_t device_size;		// Device size.
			unsigned int sector_size;	// Sector size. (bytes per sector)
			bool isKreonUnlocked;		// Is Kreon mode unlocked?

			DeviceInfo()
				: device_pos(0)
				, device_size(0)
				, sector_size(0)
				, isKreonUnlocked(0)
			{ }
		};

		DeviceInfo *devInfo;

	public:
		/**
		 * Convert an RpFile::FileMode to Win32 CreateFile() parameters.
		 * @param mode				[in] FileMode
		 * @param pdwDesiredAccess		[out] dwDesiredAccess
		 * @param pdwShareMode			[out] dwShareMode
		 * @param pdwCreationDisposition	[out] dwCreationDisposition
		 * @return 0 on success; non-zero on error.
		 */
		static inline int mode_to_win32(RpFile::FileMode mode,
			DWORD *pdwDesiredAccess,
			DWORD *pdwShareMode,
			DWORD *pdwCreationDisposition);

		/**
		 * (Re-)Open the main file.
		 *
		 * INTERNAL FUNCTION. This does NOT affect gzfd.
		 * NOTE: This function sets q->m_lastError.
		 *
		 * Uses parameters stored in this->filename and this->mode.
		 * @return 0 on success; non-zero on error.
		 */
		int reOpenFile(void);

		/**
		 * Read using block reads.
		 * Required for block devices.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		size_t readUsingBlocks(void *ptr, size_t size);
};

}
