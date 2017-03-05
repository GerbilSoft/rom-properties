/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpWin32.cpp: Windows-specific functions.                                *
 *                                                                         *
 * Copyright (c) 2009-2016 by David Korth.                                 *
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

#include "RpWin32.hpp"
#include "common.h"

namespace LibRomData {

typedef struct _errmap {
        DWORD w32;	// Win32 error code.
        int posix;	// POSIX error code.
} errmap;

static const errmap w32_to_posix[] = {
	{ERROR_SUCCESS,			0         },  // 0
        {ERROR_INVALID_FUNCTION,	EINVAL    },  // 1
        {ERROR_FILE_NOT_FOUND,		ENOENT    },  // 2
        {ERROR_PATH_NOT_FOUND,		ENOENT    },  // 3
        {ERROR_TOO_MANY_OPEN_FILES,	EMFILE    },  // 4
        {ERROR_ACCESS_DENIED,		EACCES    },  // 5
        {ERROR_INVALID_HANDLE,		EBADF     },  // 6
        {ERROR_ARENA_TRASHED,		ENOMEM    },  // 7
        {ERROR_NOT_ENOUGH_MEMORY,	ENOMEM    },  // 8
        {ERROR_INVALID_BLOCK,		ENOMEM    },  // 9
        {ERROR_BAD_ENVIRONMENT,		E2BIG     },  // 10
        {ERROR_BAD_FORMAT,		ENOEXEC   },  // 11
        {ERROR_INVALID_ACCESS,		EINVAL    },  // 12
        {ERROR_INVALID_DATA,		EINVAL    },  // 13
	{ERROR_OUTOFMEMORY,		ENOMEM    },  // 14
        {ERROR_INVALID_DRIVE,		ENOENT    },  // 15
        {ERROR_CURRENT_DIRECTORY,	EACCES    },  // 16
        {ERROR_NOT_SAME_DEVICE,		EXDEV     },  // 17
        {ERROR_NO_MORE_FILES,		ENOENT    },  // 18
	{ERROR_WRITE_PROTECT,		EROFS     },  // 19
	{ERROR_BAD_UNIT,		ENODEV    },  // 20
	{ERROR_WRITE_FAULT,		EIO       },  // 29
	{ERROR_READ_FAULT,		EIO       },  // 30
	{ERROR_GEN_FAILURE,		EIO       },  // 31
#ifdef ETXTBSY
	// ETXTBSY is not defined in MinGW-w64 4.0.6.
	{ERROR_SHARING_VIOLATION,	ETXTBSY   },  // 32 (TODO)
#endif
        {ERROR_LOCK_VIOLATION,		EACCES    },  // BAD33
	{ERROR_HANDLE_DISK_FULL,	ENOSPC    },  // 39
	{ERROR_NOT_SUPPORTED,		ENOTSUP   },  // 50
#ifdef ENOTUNIQ /* MSVC 2015 doesn't have ENOTUNIQ. */
	{ERROR_DUP_NAME,		ENOTUNIQ  },  // 52
#endif
        {ERROR_BAD_NETPATH,		ENOENT    },  // 53
	{ERROR_DEV_NOT_EXIST,		ENODEV    },  // 55
        {ERROR_NETWORK_ACCESS_DENIED,	EACCES    },  // 65
        {ERROR_BAD_NET_NAME,		ENOENT    },  // 67
        {ERROR_FILE_EXISTS,		EEXIST    },  // 80
        {ERROR_CANNOT_MAKE,		EACCES    },  // 82
        {ERROR_FAIL_I24,		EACCES    },  // 83
        {ERROR_INVALID_PARAMETER,	EINVAL    },  // 87
        {ERROR_NO_PROC_SLOTS,		EAGAIN    },  // 89
        {ERROR_DRIVE_LOCKED,		EACCES    },  // 108
        {ERROR_BROKEN_PIPE,		EPIPE     },  // 109
	{ERROR_OPEN_FAILED,		EIO       },  // 110
	{ERROR_BUFFER_OVERFLOW,		ENAMETOOLONG}, // 111
        {ERROR_DISK_FULL,		ENOSPC    },  // 112
        {ERROR_INVALID_TARGET_HANDLE,	EBADF     },  // 114
	{ERROR_CALL_NOT_IMPLEMENTED,	ENOSYS    },  // 120
        {ERROR_INVALID_HANDLE,		EINVAL    },  // 124
        {ERROR_WAIT_NO_CHILDREN,	ECHILD    },  // 128
        {ERROR_CHILD_NOT_COMPLETE,	ECHILD    },  // 129
        {ERROR_DIRECT_ACCESS_HANDLE,	EBADF     },  // 130
        {ERROR_NEGATIVE_SEEK,		EINVAL    },  // 131
        {ERROR_SEEK_ON_DEVICE,		EACCES    },  // 132
        {ERROR_DIR_NOT_EMPTY,		ENOTEMPTY },  // 145
        {ERROR_NOT_LOCKED,		EACCES    },  // 158
        {ERROR_BAD_PATHNAME,		ENOENT    },  // 161
        {ERROR_MAX_THRDS_REACHED,	EAGAIN    },  // 164
        {ERROR_LOCK_FAILED,		EACCES    },  // 167
        {ERROR_ALREADY_EXISTS,		EEXIST    },  // 183
        {ERROR_FILENAME_EXCED_RANGE,	ENOENT    },  // 206
        {ERROR_NESTING_NOT_ALLOWED,	EAGAIN    },  // 215
	{ERROR_EXE_MACHINE_TYPE_MISMATCH, ENOEXEC },  // 216
#ifdef ERROR_IMAGE_SUBSYSTEM_NOT_PRESENT
	// ERROR_IMAGE_SUBSYSTEM_NOT_PRESENT is not defined in MinGW-w64 4.0.6.
	{ERROR_IMAGE_SUBSYSTEM_NOT_PRESENT, ENOEXEC}, // 308
#endif
#ifdef ERROR_DISK_RESOURCES_EXHAUSTED
	// ERROR_DISK_RESOURCES_EXHAUSTED is not defined in the Windows 7 SDK.
	{ERROR_DISK_RESOURCES_EXHAUSTED, ENOSPC   },  // 314
#endif
	{ERROR_INVALID_ADDRESS,		EFAULT    },  // 487
        {ERROR_NOT_ENOUGH_QUOTA,	ENOMEM    }   // 1816
};

/* The following two constants must be the minimum and maximum
   values in the (contiguous) range of Exec Failure errors. */
#define MIN_EXEC_ERROR ERROR_INVALID_STARTING_CODESEG
#define MAX_EXEC_ERROR ERROR_INFLOOP_IN_RELOC_CHAIN

/* These are the low and high value in the range of errors that are
   access violations */
#define MIN_EACCES_RANGE ERROR_WRITE_PROTECT
#define MAX_EACCES_RANGE ERROR_SHARING_BUFFER_EXCEEDED

/**
 * bsearch() comparison function for the error code table.
 * @param a
 * @param b
 * @return
 */
static int errmap_compar(const void *a, const void *b)
{
	DWORD err1 = static_cast<const errmap*>(a)->w32;
	DWORD err2 = static_cast<const errmap*>(b)->w32;
	if (err1 < err2) return -1;
	if (err1 > err2) return 1;
	return 0;
}

/**
 * Convert a Win32 error number to a POSIX error code.
 * @param w32err Win32 error number.
 * @return Positive POSIX error code. (If no equivalent is found, default is EINVAL.)
 */
int w32err_to_posix(DWORD w32err)
{
	// Check the error code table.
	const errmap key = {w32err, 0};
	const errmap *entry = static_cast<const errmap*>(bsearch(&key,
			w32_to_posix, ARRAY_SIZE(w32_to_posix),
			sizeof(errmap), errmap_compar));
	if (entry) {
		// Found an error code.
		return entry->posix;
	}

	/* The error code wasn't in the table.  We check for a range of */
	/* EACCES errors or exec failure errors (ENOEXEC).  Otherwise   */
	/* EINVAL is returned.                                          */
	if (w32err >= MIN_EACCES_RANGE && w32err <= MAX_EACCES_RANGE)
		return EACCES;
	else if (w32err >= MIN_EXEC_ERROR && w32err <= MAX_EXEC_ERROR)
		return ENOEXEC;

	// Error code not found.
	// Return the default EINVAL.
	return EINVAL;
}

}

/** C99/POSIX replacement functions. **/

#ifdef _MSC_VER
// MSVC doesn't have struct timeval or gettimeofday().
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	union {
		uint64_t ns100;	// Time since 1/1/1601 in 100ns units.
		FILETIME ft;
	} _now;
	TIME_ZONE_INFORMATION TimeZoneInformation;
	DWORD tzi;

	if (tz) {
		// Get the timezone information.
		if ((tzi = GetTimeZoneInformation(&TimeZoneInformation)) != TIME_ZONE_ID_INVALID) {
			tz->tz_minuteswest = TimeZoneInformation.Bias;
			if (tzi == TIME_ZONE_ID_DAYLIGHT) {
				tz->tz_dsttime = 1;
			} else {
				tz->tz_dsttime = 0;
			}
		} else {
			tz->tz_minuteswest = 0;
			tz->tz_dsttime = 0;
		}
	}

	if (tv) {
		GetSystemTimeAsFileTime(&_now.ft);	// 100ns units since 1/1/1601
		// Windows XP's accuract seems to be ~125,000ns == 125us == 0.125ms
		_now.ns100 -= FILETIME_1970;
		tv->tv_sec = _now.ns100 / HECTONANOSEC_PER_SEC;			// seconds since 1/1/1970
		tv->tv_usec = (_now.ns100 % HECTONANOSEC_PER_SEC) / 10LL;	// microseconds
	}

	return 0;
}
#endif /* _MSC_VER */
