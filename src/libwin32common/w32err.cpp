/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * w32err.cpp: Error code mapping. (Windows to POSIX)                      *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "w32err.hpp"

#include "RpWin32_sdk.h"
#include <winternl.h>	// for NT_SUCCESS() and related
#include <ntstatus.h>

// C includes (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstdint>

// C++ STL classes
#include <algorithm>
#include <limits>

// Some errors aren't present in various SDKs.
#ifndef ERROR_IMAGE_SUBSYSTEM_NOT_PRESENT	// not present in MinGW-w64 4.0.6
#  define ERROR_IMAGE_SUBSYSTEM_NOT_PRESENT 308
#endif
#ifndef ERROR_DISK_RESOURCES_EXHAUSTED		// not present in Windows 7 SDK
#  define ERROR_DISK_RESOURCES_EXHAUSTED 314
#endif

typedef struct _w32err_map_t {
        uint16_t w32;	// Win32 error code
        uint16_t posix;	// POSIX error code
} w32err_map_t;

static const w32err_map_t w32err_map[] = {
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
	// Error code table is limited to uint16_t.
	assert(w32err <= std::numeric_limits<uint16_t>::max());
	if (w32err > ERROR_NOT_ENOUGH_QUOTA) {
		return EINVAL;
	}

	// Check the error code table.
	static const w32err_map_t *const p_w32err_map_end = &w32err_map[_countof(w32err_map)];
	auto iter = std::lower_bound(w32err_map, p_w32err_map_end, w32err,
		[](w32err_map_t err, DWORD w32err) noexcept -> bool {
			return (err.w32 < w32err);
		});
	if (iter != p_w32err_map_end && iter->w32 == w32err) {
		// Found an error code.
		return iter->posix;
	}

	// The error code wasn't in the table.  We check for a range of
	// EACCES errors or exec failure errors (ENOEXEC).  Otherwise
	// EINVAL is returned.
	if (w32err >= MIN_EACCES_RANGE && w32err <= MAX_EACCES_RANGE) {
		return EACCES;
	} else if (w32err >= MIN_EXEC_ERROR && w32err <= MAX_EXEC_ERROR) {
		return ENOEXEC;
	}

	// Error code not found.
	// Return the default EINVAL.
	return EINVAL;
}

typedef struct _NTSTATUS_map_t {
        uint16_t lo_status;	// NTSTATUS (low word)
        uint16_t posix;		// POSIX error code
} NTSTATUS_map_t;

static const NTSTATUS_map_t NTSTATUS_C000xxxx_map[] = {
	{LOWORD(STATUS_UNSUCCESSFUL),		EIO},		// 0xC0000001
	{LOWORD(STATUS_NOT_IMPLEMENTED),	ENOTSUP},	// 0xC0000002
	{LOWORD(STATUS_ACCESS_VIOLATION),	EFAULT},	// 0xC0000005
	{LOWORD(STATUS_IN_PAGE_ERROR),		EFAULT},	// 0xC0000006
	{LOWORD(STATUS_INVALID_HANDLE),		EBADF},		// 0xC0000008
	{LOWORD(STATUS_NO_SUCH_DEVICE),		ENODEV},	// 0xC000000E
	{LOWORD(STATUS_NO_SUCH_FILE),		ENOENT},	// 0xC000000F
	{LOWORD(STATUS_INVALID_DEVICE_REQUEST),	ENOTSUP},	// 0xC0000010
#ifdef EBADFD
	// MinGW-w64 14.0.0 and MSVC 2026 don't have EBADFD.
	{LOWORD(STATUS_END_OF_FILE),		EBADFD},	// 0xC0000011 (NOTE: POSIX doesn't have an EOF error code...)
#endif
#ifdef ENOMEDIUM
	// MinGW-w64 14.0.0 and MSVC 2026 don't have ENOMEDIUM.
	{LOWORD(STATUS_NO_MEDIA_IN_DEVICE),	ENOMEDIUM},	// 0xC0000013
#endif
#ifdef EMEDIUMTYPE
	// MinGW-w64 14.0.0 and MSVC 2026 don't have EMEDIUMTYPE.
	{LOWORD(STATUS_UNRECOGNIZED_MEDIA),	EMEDIUMTYPE},	// 0xC0000014
#endif
	{LOWORD(STATUS_NO_MEMORY),		ENOMEM},	// 0xC0000017
	{LOWORD(STATUS_ACCESS_DENIED),		EACCES},	// 0xC0000022
	//{LOWORD(STATUS_BUFFER_TOO_SMALL),	???},		// 0xC0000023 (TODO)
	{LOWORD(STATUS_OBJECT_NAME_NOT_FOUND),	ENOENT},	// 0xC0000034
	{LOWORD(STATUS_OBJECT_PATH_NOT_FOUND),	ENOENT},	// 0x0000003A
	{LOWORD(STATUS_FILE_IS_A_DIRECTORY),	EISDIR},	// 0xC00000BA
	{LOWORD(STATUS_NOT_SUPPORTED),		ENOTSUP},	// 0xC00000BB
	{LOWORD(STATUS_NETWORK_BUSY),		EBUSY},		// 0xC00000BF
	{LOWORD(STATUS_DEVICE_DOES_NOT_EXIST),	ENODEV},	// 0xC00000C0
	{LOWORD(STATUS_DIRECTORY_NOT_EMPTY),	ENOTEMPTY},	// 0xC0000101
	{LOWORD(STATUS_NOT_A_DIRECTORY),	ENOTDIR},	// 0xC0000103
	{LOWORD(STATUS_NAME_TOO_LONG),		ENAMETOOLONG},	// 0xC0000106
	{LOWORD(STATUS_CANCELLED),		ECANCELED},	// 0xC0000120
	{LOWORD(STATUS_FILE_CLOSED),		EBADF},		// 0xC0000128
	{LOWORD(STATUS_IO_DEVICE_ERROR),	EIO},		// 0xC0000185
	{LOWORD(STATUS_NOT_FOUND),		ENOENT},	// 0xC0000225

	// TODO: Reparse point errors?
	//{LOWORD(STATUS_NOT_A_REPARSE_POINT),	???},		// 0xC0000275
	//{LOWORD(STATUS_REPARSE_POINT_NOT_RESOLVED),	???},	// 0xC0000280
	//{LOWORD(STATUS_DIRECTORY_IS_A_REPARSE_POINT),	???},	// 0xC0000281

	{LOWORD(STATUS_SYSTEM_DEVICE_NOT_FOUND), ENODEV},	// 0xC0000452
	{LOWORD(STATUS_FILE_TOO_LARGE),		EFBIG},		// 0xC0000904
};

/**
 * Convert an NTSTATUS error code to a POSIX error code.
 * @param status NTSTATUS
 * @return Positive POSIX error code. (If no equivalent is found, default is EINVAL.)
 */
int NTSTATUS_to_posix(NTSTATUS status)
{
	if (NT_SUCCESS(status)) {
		// Not an error.
		return 0;
	}

	// NOTE: Only checking ERROR (0xC0000000) values right now.
	// Need to check WARNING (0x80000000) values later.
	if (HIWORD(status) != 0xC000U) {
		// Not an ERROR (0xC0000000) value.
		return EINVAL;
	}

	// Check the error code table.
	static const NTSTATUS_map_t *const p_NTSTATUS_C000xxxx_map_end = &NTSTATUS_C000xxxx_map[_countof(NTSTATUS_C000xxxx_map)];
	auto iter = std::lower_bound(NTSTATUS_C000xxxx_map, p_NTSTATUS_C000xxxx_map_end, LOWORD(status),
		[](NTSTATUS_map_t err, uint16_t lo_status) noexcept -> bool {
			return (err.lo_status < lo_status);
		});
	if (iter != p_NTSTATUS_C000xxxx_map_end && iter->lo_status == LOWORD(status)) {
		// Found an error code.
		return iter->posix;
	}

	// Error code not found.
	// Return the default EINVAL.
	return EINVAL;
}
