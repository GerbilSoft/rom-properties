/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_p.hpp: Standard file object. (PRIVATE CLASS)                     *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_FILE_RPFILE_P_HPP__
#define __ROMPROPERTIES_LIBRPBASE_FILE_RPFILE_P_HPP__

#include "RpFile.hpp"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

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

#ifdef _WIN32
// Windows SDK
# include <windows.h>
# include <winioctl.h>
# include <io.h>
#else /* !_WIN32 */
// C includes. (C++ namespace)
# include <cstdio>
#endif /* _WIN32 */

// Windows uses INVALID_HANDLE_VALUE for invalid handles.
// Other systems generally use nullptr.
#ifndef _WIN32
# define INVALID_HANDLE_VALUE nullptr
#endif

namespace LibRpBase {

/** RpFilePrivate **/

class RpFilePrivate
{
	public:
		// NOTE: Using #define instead of static const
		// to work around C2864 on MSVC.
#ifdef _WIN32
		typedef HANDLE FILE_TYPE;
# define FILE_INIT INVALID_HANDLE_VALUE
#else /* !_WIN32 */
		typedef FILE *FILE_TYPE;
# define FILE_INIT nullptr
#endif /* _WIN32 */

		RpFilePrivate(RpFile *q, const char *filename, RpFile::FileMode mode)
			: q_ptr(q), file(FILE_INIT), filename(filename)
			, mode(mode), gzfd(nullptr), gzsz(-1), devInfo(nullptr) { }
		RpFilePrivate(RpFile *q, const string &filename, RpFile::FileMode mode)
			: q_ptr(q), file(FILE_INIT), filename(filename)
			, mode(mode), gzfd(nullptr), gzsz(-1), devInfo(nullptr) { }
		~RpFilePrivate();

	private:
		RP_DISABLE_COPY(RpFilePrivate)
		RpFile *const q_ptr;

	public:
		FILE_TYPE file;		// File pointer.
		string filename;	// Filename.
		RpFile::FileMode mode;	// File mode.

		gzFile gzfd;		// Used for transparent gzip decompression.
		int64_t gzsz;		// Uncompressed file size.

		// Device information struct.
		// Only used if the underlying file
		// is a device node.
		struct DeviceInfo {
			int64_t device_pos;	// Device position.
			int64_t device_size;	// Device size.
			uint32_t sector_size;	// Sector size. (bytes per sector)
			bool isKreonUnlocked;	// Is Kreon mode unlocked?

			// Sector cache.
			uint8_t *sector_cache;	// Sector cache.
			uint32_t lba_cache;	// Last LBA cached.

			DeviceInfo()
				: device_pos(0)
				, device_size(0)
				, sector_size(0)
				, isKreonUnlocked(0)
				, sector_cache(nullptr)
				, lba_cache(~0U)
			{ }

			~DeviceInfo() { delete[] sector_cache; }

			void alloc_sector_cache(void)
			{
				assert(sector_size != 0);
				assert(sector_size <= 65536);
				if (!sector_cache) {
					if (sector_size > 0 && sector_size <= 65536) {
						sector_cache = new uint8_t[sector_size];
					}
				}
			}
		};

		DeviceInfo *devInfo;

	public:
#ifdef _WIN32
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
#else /* !_WIN32 */
		/**
		 * Convert an RpFile::FileMode to an fopen() mode string.
		 * @param mode	[in] FileMode
		 * @return fopen() mode string.
		 */
		static inline const char *mode_to_str(RpFile::FileMode mode);
#endif /* _WIN32 */

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

#endif /* __ROMPROPERTIES_LIBRPBASE_FILE_RPFILE_STDIO_P_HPP__ */
