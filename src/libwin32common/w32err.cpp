/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * w32err.cpp: Error code mapping. (Windows to POSIX)                      *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "w32err.hpp"

// C includes
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

// C++ includes
#include <algorithm>

// Some errors aren't present in various SDKs.
#ifndef ERROR_IMAGE_SUBSYSTEM_NOT_PRESENT	// not present in MinGW-w64 4.0.6
#  define ERROR_IMAGE_SUBSYSTEM_NOT_PRESENT 308
#endif
#ifndef ERROR_DISK_RESOURCES_EXHAUSTED		// not present in Windows 7 SDK
#  define ERROR_DISK_RESOURCES_EXHAUSTED 314
#endif

typedef struct _errmap {
        uint16_t w32;	// Win32 error code.
        uint16_t posix;	// POSIX error code.
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
	// MinGW-w64 4.0.6 doesn't have ETXTBSY.
	{ERROR_SHARING_VIOLATION,	ETXTBSY   },  // 32 (TODO)
#endif
	{ERROR_LOCK_VIOLATION,		EACCES    },  // 33
	{ERROR_HANDLE_DISK_FULL,	ENOSPC    },  // 39
	{ERROR_NOT_SUPPORTED,		ENOTSUP   },  // 50
#ifdef ENOTUNIQ
	// MSVC 2015 doesn't have ENOTUNIQ.
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
	{ERROR_INVALID_LEVEL,		EINVAL    },  // 124
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
	{ERROR_IMAGE_SUBSYSTEM_NOT_PRESENT, ENOEXEC}, // 308
	{ERROR_DISK_RESOURCES_EXHAUSTED, ENOSPC   },  // 314
	{ERROR_INVALID_ADDRESS,		EFAULT    },  // 487
	{ERROR_NO_UNICODE_TRANSLATION,	EILSEQ    },  // 1113
	{ERROR_IO_DEVICE,		EIO       },  // 1117
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
 * Convert a Win32 error number to a POSIX error code.
 * @param w32err Win32 error number.
 * @return Positive POSIX error code. (If no equivalent is found, default is EINVAL.)
 */
int w32err_to_posix(DWORD w32err)
{
	if (w32err > UINT16_MAX) {
		// Error code table is limited to uint16_t.
		return EINVAL;
	}

	// Check the error code table.
	static const errmap *const p_w32_to_posix_end =
		&w32_to_posix[_countof(w32_to_posix)];
	auto pErr = std::lower_bound(w32_to_posix, p_w32_to_posix_end, w32err,
		[](const errmap &err, DWORD w32err) noexcept -> bool {
			return (err.w32 < w32err);
		});
	if (pErr != p_w32_to_posix_end && pErr->w32 == w32err) {
		// Found an error code.
		return pErr->posix;
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
