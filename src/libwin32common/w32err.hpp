/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * w32err.hpp: Error code mapping. (Windows to POSIX)                      *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_W32ERR_HPP__
#define __ROMPROPERTIES_LIBWIN32COMMON_W32ERR_HPP__

#include "RpWin32_sdk.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert a Win32 error code to a POSIX error code.
 * @param w32err Win32 error code.
 * @return Positive POSIX error code. (If no equivalent is found, default is EINVAL.)
 */
RP_LIBROMDATA_PUBLIC
int w32err_to_posix(DWORD w32err);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_W32ERR_HPP__ */
