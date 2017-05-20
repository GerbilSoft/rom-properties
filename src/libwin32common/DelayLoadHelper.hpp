/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * DelayLoadHelper.hpp: DelayLoad helper functions and macros.             *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_DELAYLOADHELPER_HPP__
#define __ROMPROPERTIES_LIBWIN32COMMON_DELAYLOADHELPER_HPP__

#if !defined(_WIN32) || !defined(_MSC_VER)
#error DelayLoadHelper.hpp is MSVC and Win32 only at the moment.
#endif

// MSVC: Exception handling for /DELAYLOAD.
// TODO: Check for /DELAYLOAD usage.
#include "libwin32common/RpWin32_sdk.h"
#include <delayimp.h>
#include <excpt.h>
#include <errno.h>

// This isn't defined if compiling with MSVC 2012+
// when using the Windows 7 SDK.
#ifndef FACILITY_VISUALCPP
#define FACILITY_VISUALCPP 109
#endif

/**
 * DELAYLOAD_FILTER_FUNCTION_IMPL(): Implementation of the DelayLoad filter function.
 * Don't use this directly; it's used by DELAYLOAD_TEST_FUNCTION_IMPL*().
 * @param fn Function name.
 */
#define DELAYLOAD_FILTER_FUNCTION_IMPL(fn) \
static LONG WINAPI DelayLoad_filter_##fn(DWORD exceptioncode) { \
	if (exceptioncode == VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND)) { \
		return EXCEPTION_EXECUTE_HANDLER; \
	} \
	return EXCEPTION_CONTINUE_SEARCH; \
}

/**
 * DELAYLOAD_TEST_FUNCTION_IMPL0(): Implementation of the DelayLoad test function.
 * Creates a test function named DelayLoad_test_##fn, which should
 * be called prior to using the DLL.
 * @param fn Function name. Must take 0 arguments.
 */
#define DELAYLOAD_TEST_FUNCTION_IMPL0(fn) \
DELAYLOAD_FILTER_FUNCTION_IMPL(fn) \
static int DelayLoad_test_##fn(void) { \
	static int success = 0; \
	if (!success) { \
		__try { \
			(void)fn(); \
		} __except (DelayLoad_filter_##fn(GetExceptionCode())) { \
			return -ENOTSUP; \
		} \
		success = 1; \
	} \
	return 0; \
}

/**
 * DELAYLOAD_TEST_FUNCTION_IMPL2(): Implementation of the DelayLoad test function.
 * Creates a test function named DelayLoad_test_##fn, which should
 * be called prior to using the DLL.
 * @param fn Function name. Must take 2 arguments.
 */
#define DELAYLOAD_TEST_FUNCTION_IMPL2(fn, arg0, arg1) \
DELAYLOAD_FILTER_FUNCTION_IMPL(fn) \
static int DelayLoad_test_##fn(void) { \
	static int success = 0; \
	if (!success) { \
		__try { \
			(void)fn(arg0, arg1); \
		} __except (DelayLoad_filter_##fn(GetExceptionCode())) { \
			return -ENOTSUP; \
		} \
		success = 1; \
	} \
	return 0; \
}

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_DELAYLOADHELPER_HPP__ */
