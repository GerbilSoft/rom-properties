/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * InitOnceExecuteOnceXP.h: InitOnceExecuteOnce() implementation for       *
 * Windows XP. (also works on later systems)                               *
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

// Based on the implementation from Chromium:
// - https://chromium.googlesource.com/chromium/src.git/+/18ad5f3a40ceab583961ca5dc064e01900514c57%5E%21/#F0

#ifndef __LIBROMDATA_THREADS_INITONCEEXECUTEONCEXP_H__
#define __LIBROMDATA_THREADS_INITONCEEXECUTEONCEXP_H__

#ifndef _WIN32
#error InitOnceExecuteOnceXP.h should only be included in Windows builds.
#endif /* _WIN32 */

#include "../RpWin32_sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

// We're only using our own InitOnceExecuteOnce() implementation
// if targetting a version of Windows earlier than Vista.
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0600

// Chromium's implementation uses int32_t.
// Windows Vista's implementation uses PVOID.
// Let's use LONG to save memory if we're
// not compiling a Vista-only build.
typedef LONG INIT_ONCE_XP, *PINIT_ONCE_XP;
#ifdef INIT_ONCE
#undef INIT_ONCE
#endif
#ifdef PINIT_ONCE
#undef PINIT_ONCE
#endif
#define INIT_ONCE INIT_ONCE_XP
#define PINIT_ONCE PINIT_ONCE_XP

#define INIT_ONCE_STATIC_INIT_XP 0L
#ifdef INIT_ONCE_STATIC_INIT
#undef INIT_ONCE_STATIC_INIT
#endif
#define INIT_ONCE_STATIC_INIT INIT_ONCE_STATIC_INIT_XP

typedef BOOL (WINAPI *PINIT_ONCE_XP_FN)(_Inout_ PINIT_ONCE_XP once, _In_ PVOID param, _Out_opt_ LPVOID *context);
#define PINIT_ONCE PINIT_ONCE_XP
#define PINIT_ONCE_FN PINIT_ONCE_XP_FN

/**
 * InitOnceExecuteOnce() implementation for Windows XP.
 * Based on the implementation from Chromium.
 * @param once
 * @param func
 * @param param
 * @param context
 * @return
 */
BOOL WINAPI InitOnceExecuteOnceXP(_Inout_ PINIT_ONCE_XP once, _In_ PINIT_ONCE_XP_FN func, _Inout_opt_ PVOID param, _Out_opt_ LPVOID *context);
#define InitOnceExecuteOnce(once, func, param, context) InitOnceExecuteOnceXP(once, func, param, context)

#endif /* !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0600 */

#ifdef __cplusplus
}
#endif

#endif /* __LIBROMDATA_THREADS_INITONCEEXECUTEONCEXP_H__ */
