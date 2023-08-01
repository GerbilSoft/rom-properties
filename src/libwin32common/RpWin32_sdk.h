/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * RpWin32_sdk.h: Windows SDK defines and includes.                        *
 *                                                                         *
 * Copyright (c) 2009-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "libwin32common/config.libwin32common.h"

#if !defined(UNICODE) || !defined(_UNICODE)
#  error rom-properties for Windows must be compiled with -DUNICODE and -D_UNICODE enabled
#endif

// Show a warning if one of the macros isn't defined in CMake.
#ifndef WINVER
#  pragma message("WINVER not defined; defaulting to 0x0500.")
#  define WINVER	0x0500
#endif
#  ifndef _WIN32_WINNT
#  pragma message("_WIN32_WINNT not defined; defaulting to 0x0500.")
#  define _WIN32_WINNT	0x0500
#endif
#ifndef _WIN32_IE
#  pragma message("_WIN32_IE not defined; defaulting to 0x0500.")
#  define _WIN32_IE	0x0500
#endif

// Define this symbol to get XP themes. See: [FIXME: 404]
// http://msdn.microsoft.com/library/en-us/dnwxp/html/xptheming.asp
// for more info. Note that as of May 2006, the page says the symbols should
// be called "SIDEBYSIDE_COMMONCONTROLS" but the headers in my SDKs in VC 6 & 7
// don't reference that symbol. If ISOLATION_AWARE_ENABLED doesn't work for you,
// try changing it to SIDEBYSIDE_COMMONCONTROLS
#define ISOLATION_AWARE_ENABLED 1

// Disable functionality that we aren't using.
#define WIN32_LEAN_AND_MEAN 1
#define NOGDICAPMASKS 1
//#define NOVIRTUALKEYCODES 1
//#define NOWINMESSAGES 1
//#define NOWINSTYLES 1
//#define NOSYSMETRICS 1
//#define NOMENUS 1
#define NOICONS 1
//#define NOKEYSTATES 1
#define NOSYSCOMMANDS 1
//#define NORASTEROPS 1
//#define NOSHOWWINDOW 1
#define NOATOM 1
//#define NOCLIPBOARD 1
//#define NOCOLOR 1
//#define NOCTLMGR 1
//#define NODRAWTEXT 1
//#define NOGDI 1
//#define NOKERNEL 1
//#define NONLS 1
//#define NOMB 1
#define NOMEMMGR 1
//#define NOMETAFILE 1
#define NOMINMAX 1
//#define NOMSG 1
#define NOOPENFILE 1
//#define NOSCROLL 1
#define NOSERVICE 1
//#define NOSOUND 1
//#define NOTEXTMETRIC 1
//#define NOWH 1
//#define NOWINOFFSETS 1
#define NOCOMM 1
#define NOKANJI 1
#define NOHELP 1
#define NOPROFILER 1
#define NODEFERWINDOWPOS 1
#define NOMCX 1

#include <windows.h>
#include "tcharx.h"

#ifndef WM_DPICHANGED
#  define WM_DPICHANGED 0x2E0
#endif
#ifndef WM_DPICHANGED_BEFOREPARENT
#  define WM_DPICHANGED_BEFOREPARENT 0x2E2
#endif
#ifndef WM_DPICHANGED_AFTERPARENT
#  define WM_DPICHANGED_AFTERPARENT 0x2E3
#endif

#if defined(__GNUC__) && defined(__MINGW32__) && _WIN32_WINNT < 0x0502 && defined(__cplusplus)
/**
 * MinGW-w64 only defines ULONG overloads for the various atomic functions
 * if _WIN32_WINNT >= 0x0502.
 */
static inline ULONG InterlockedIncrement(ULONG volatile *Addend)
{
	return (ULONG)(InterlockedIncrement((LONG volatile*)Addend));
}
static inline ULONG InterlockedDecrement(ULONG volatile *Addend)
{
	return (ULONG)(InterlockedDecrement((LONG volatile*)Addend));
}
#endif /* __GNUC__ && __MINGW32__ && _WIN32_WINNT < 0x0502 && __cplusplus */

// UUID attribute.
#ifdef _MSC_VER
#  define UUID_ATTR(str) __declspec(uuid(str))
#else /* !_MSC_VER */
// UUID attribute is not supported by gcc-5.2.0.
#  define UUID_ATTR(str)
#endif /* _MSC_VER */

// SAL 1.0 annotations not supported by MinGW-w64 5.0.1.
#ifndef __out_opt
#  define __out_opt
#endif

// SAL 2.0 annotations not supported by MinGW-w64 3.1.0. (Ubuntu 14.04)
#ifndef _Inout_
#  define _Inout_
#endif

// SAL 2.0 annotations not supported by Windows SDK 7.1A. (MSVC 2010)
#ifndef _COM_Outptr_
#  define _COM_Outptr_
#endif
#ifndef _Outptr_
#  define _Outptr_
#endif
#ifndef _Acquires_lock_
#  define _Acquires_lock_(lock)
#endif
#ifndef _Releases_lock_
#  define _Releases_lock_(lock)
#endif
#ifndef _Out_writes_
#  define _Out_writes_(var)
#endif
#ifndef _Outptr_result_nullonfailure_
#  define _Outptr_result_nullonfailure_
#endif
#ifndef _Outptr_opt_
#  define _Outptr_opt_
#endif

// FIXME: _Check_return_ on MSYS2/MinGW-w64 (gcc-9.2.0-2, MinGW-w64 7.0.0.5524.2346384e-1) fails:
// "error: expected unqualified-id before string constant"
#if defined(__GNUC__)
#  ifdef _Check_return_
#    undef _Check_return_
#  endif
#  define _Check_return_
#endif

// Current image instance.
// This is filled in by the linker.
// Reference: https://devblogs.microsoft.com/oldnewthing/20041025-00/?p=37483
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

// LoadLibraryEx() flags
#ifndef LOAD_WITH_ALTERED_SEARCH_PATH
#  define LOAD_WITH_ALTERED_SEARCH_PATH 0x00000008
#endif
#ifndef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
#  define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR 0x00000100
#endif
#ifndef LOAD_LIBRARY_SEARCH_APPLICATION_DIR
#  define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#endif
#ifndef LOAD_LIBRARY_SEARCH_USER_DIRS
#  define LOAD_LIBRARY_SEARCH_USER_DIRS 0x00000400
#endif
#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#  define LOAD_LIBRARY_SEARCH_SYSTEM32 0x00000800
#endif
#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
#  define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS 0x00001000
#endif
#ifndef LOAD_LIBRARY_SAFE_CURRENT_DIRS
#  define LOAD_LIBRARY_SAFE_CURRENT_DIRS 0x00002000
#endif

// If building for "old" Windows (pre-XP), disable LoadLibraryEx() usage.
#ifdef ENABLE_OLDWINCOMPAT
#  ifdef LoadLibraryEx
#    undef LoadLibraryEx
#  endif
#  define LoadLibraryEx(lpLibFileName, hFile, dwFlags)	LoadLibrary(lpLibFileName)
#  define LoadLibraryExA(lpLibFileName, hFile, dwFlags)	LoadLibraryA(lpLibFileName)
#  define LoadLibraryExW(lpLibFileName, hFile, dwFlags)	LoadLibraryW(lpLibFileName)
#endif /* ENABLE_OLDWINCOMPAT */
