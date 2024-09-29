/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpFile_p.hpp: Standard file object. (PRIVATE CLASS)                     *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.librpfile.h"
#include "RpFile.hpp"

// C includes (C++ namespace)
#include <cassert>

// C++ includes
#include <memory>

// zlib for transparent gzip decompression.
#include <zlib.h>
// gzclose_r() and gzclose_w() were introduced in zlib-1.2.4.
#if (ZLIB_VER_MAJOR > 1) || \
    (ZLIB_VER_MAJOR == 1 && ZLIB_VER_MINOR > 2) || \
    (ZLIB_VER_MAJOR == 1 && ZLIB_VER_MINOR == 2 && ZLIB_VER_REVISION >= 4)
// zlib-1.2.4 or later
#else
#  define gzclose_r(file) gzclose(file)
#  define gzclose_w(file) gzclose(file)
#endif

#ifdef _WIN32
// Windows SDK
#  include <windows.h>
#  include <winioctl.h>
#  include <io.h>
#else /* !_WIN32 */
// C includes. (C++ namespace)
#  include <cstdio>
#endif /* _WIN32 */

// Windows uses INVALID_HANDLE_VALUE for invalid handles.
// Other systems generally use nullptr.
#ifndef _WIN32
#  define INVALID_HANDLE_VALUE nullptr
#endif

#ifdef RP_OS_SCSI_SUPPORTED
// OS-specific headers for DeviceInfo.
#  if defined(__FreeBSD__) || defined(__DragonFly__)
#    define USING_FREEBSD_CAMLIB 1
#    include <camlib.h>
#  endif /* __FreeBSD__ || __DragonFly__ */
#endif /* RP_OS_SCSI_SUPPORTED */

namespace LibRpFile {

/** RpFilePrivate **/

class RpFilePrivate
{
	public:
#ifdef _WIN32
		typedef HANDLE FILE_TYPE;
		RpFilePrivate(RpFile *q, const wchar_t *filenameW, RpFile::FileMode mode);
#else /* !_WIN32 */
		typedef FILE *FILE_TYPE;
		RpFilePrivate(RpFile *q, const char *filename, RpFile::FileMode mode);
#endif /* _WIN32 */

		~RpFilePrivate();

	private:
		RP_DISABLE_COPY(RpFilePrivate)
		RpFile *const q_ptr;

	public:
		FILE_TYPE file;		// File pointer
		char *filename;		// Filename (UTF-8)
#ifdef _WIN32
		wchar_t *filenameW;	// Filename (UTF-16)
					// NOTE: Win32 version uses this as primary.
					// UTF-8 filename is only used for the filename() function.
#endif /* _WIN32 */
		RpFile::FileMode mode;	// File mode

		gzFile gzfd;		// Used for transparent gzip decompression.
		off64_t gzsz;		// Uncompressed file size.

		// Device information struct.
		// Only used if the underlying file
		// is a device node.
		struct DeviceInfo {
			off64_t device_pos;	// Device position
			off64_t device_size;	// Device size
			uint32_t sector_size;	// Sector size (bytes per sector)
			bool isKreonUnlocked;	// Is Kreon mode unlocked?

			// Sector cache
			std::unique_ptr<uint8_t[]> sector_cache;	// Sector cache
			uint32_t lba_cache;				// Last LBA cached

			// OS-specific variables.
#ifdef USING_FREEBSD_CAMLIB
			struct cam_device *cam;
#endif /* USING_FREEBSD_CAMLIB */

			DeviceInfo()
				: device_pos(0)
				, device_size(0)
				, sector_size(0)
				, isKreonUnlocked(false)
				, lba_cache(~0U)
#ifdef USING_FREEBSD_CAMLIB
				, cam(nullptr)
#endif /* USING_FREEBSD_CAMLIB */
			{ }

			~DeviceInfo()
			{
#ifdef USING_FREEBSD_CAMLIB
				if (cam) {
					cam_close_device(cam);
				}
#endif /* USING_FREEBSD_CAMLIB */
			}

			void alloc_sector_cache(void)
			{
				assert(sector_size >= 512);
				assert(sector_size <= 65536);
				if (!sector_cache) {
					if (sector_size >= 512 && sector_size <= 65536) {
						sector_cache.reset(new uint8_t[sector_size]);
					}
				}
			}

			void close(void)
			{
				sector_cache.reset();

#ifdef USING_FREEBSD_CAMLIB
				if (cam) {
					cam_close_device(cam);
					cam = nullptr;
				}
#endif /* USING_FREEBSD_CAMLIB */
			}
		};

		std::unique_ptr<DeviceInfo> devInfo;

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
		ATTR_ACCESS(write_only, 2)
		ATTR_ACCESS(write_only, 3)
		ATTR_ACCESS(write_only, 4)
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

	public:
		/**
		 * Read one sector into the sector cache.
		 * @param lba LBA to read.
		 * @return 0 on success; non-zero on error.
		 */
		int readOneLBA(uint32_t lba);

		/**
		 * Read using block reads.
		 * Required for block devices.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		ATTR_ACCESS_SIZE(write_only, 2, 3)
		size_t readUsingBlocks(void *ptr, size_t size);

	public:
		enum class ScsiDirection {
			None,
			In,
			Out,
		};

		/**
		 * Send a SCSI command to the device.
		 * @param cdb		[in] SCSI command descriptor block
		 * @param cdb_len	[in] Length of cdb
		 * @param data		[in/out] Data buffer, or nullptr for ScsiDirection::None operations
		 * @param data_len	[in] Length of data
		 * @param direction	[in] Data direction
		 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
		 */
		ATTR_ACCESS_SIZE(read_only, 2, 3)
		ATTR_ACCESS_SIZE(read_write, 4, 5)
		int scsi_send_cdb(const void *cdb, uint8_t cdb_len,
			void *data, size_t data_len,
			ScsiDirection direction);

		/**
		 * Get the capacity of the device using SCSI commands.
		 * @param pDeviceSize	[out] Retrieves the device size, in bytes.
		 * @param pSectorSize	[out,opt] If not NULL, retrieves the sector size, in bytes.
		 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
		 */
		ATTR_ACCESS(write_only, 2)
		ATTR_ACCESS(write_only, 3)
		int scsi_read_capacity(off64_t *pDeviceSize, uint32_t *pSectorSize = nullptr);

		/**
		 * Read data from a device using SCSI commands.
		 * @param lbaStart	[in] Starting LBA of the data to read.
		 * @param lbaCount	[in] Number of LBAs to read.
		 * @param pBuf		[out] Output buffer.
		 * @param bufLen	[in] Output buffer length.
		 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
		 */
		ATTR_ACCESS_SIZE(write_only, 4, 5)
		int scsi_read(uint32_t lbaStart, uint16_t lbaCount, uint8_t *pBuf, size_t bufLen);
};

}
