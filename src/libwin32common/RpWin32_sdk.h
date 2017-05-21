/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * RpWin32_sdk.h: Windows SDK defines and includes.                        *
 *                                                                         *
 * Copyright (c) 2009-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_RPWIN32_SDK_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_RPWIN32_SDK_H__

// Show a warning if one of the macros isn't defined in CMake.
#ifndef WINVER
#pragma message("WINVER not defined; defaulting to 0x0500.")
#define WINVER		0x0500
#endif
#ifndef _WIN32_WINNT
#pragma message("_WIN32_WINNT not defined; defaulting to 0x0500.")
#define _WIN32_WINNT	0x0500
#endif
#ifndef _WIN32_IE
#pragma message("_WIN32_IE not defined; defaulting to 0x0500.")
#define _WIN32_IE	0x0500
#endif

// Define this symbol to get XP themes. See:
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
#define NOMENUS 1
#define NOICONS 1
#define NOKEYSTATES 1
#define NOSYSCOMMANDS 1
//#define NORASTEROPS 1
//#define NOSHOWWINDOW 1
#define NOATOM 1
//#define NOCLIPBOARD 1
//#define NOCOLOR 1
//#define NOCTLMGR 1
#define NODRAWTEXT 1
//#define NOGDI 1
//#define NOKERNEL 1
//#define NONLS 1
//#define NOMB 1
#define NOMEMMGR 1
//#define NOMETAFILE 1
#define NOMINMAX 1
//#define NOMSG 1
#define NOOPENFILE 1
#define NOSCROLL 1
#define NOSERVICE 1
//#define NOSOUND 1
//#define NOTEXTMETRIC 1
#define NOWH 1
//#define NOWINOFFSETS 1
#define NOCOMM 1
#define NOKANJI 1
#define NOHELP 1
#define NOPROFILER 1
#define NODEFERWINDOWPOS 1
#define NOMCX 1

#include <windows.h>

#if defined(__GNUC__) && defined(__MINGW32__) && _WIN32_WINNT < 0x0502
/**
 * MinGW-w64 only defines ULONG overloads for the various atomic functions
 * if _WIN32_WINNT > 0x0502.
 */
static inline ULONG InterlockedIncrement(ULONG volatile *Addend)
{
	return (ULONG)(InterlockedIncrement(reinterpret_cast<LONG volatile*>(Addend)));
}
static inline ULONG InterlockedDecrement(ULONG volatile *Addend)
{
	return (ULONG)(InterlockedDecrement(reinterpret_cast<LONG volatile*>(Addend)));
}
#endif /* __GNUC__ && __MINGW32__ && _WIN32_WINNT < 0x0502 */

// UUID attribute.
#ifdef _MSC_VER
#define UUID_ATTR(str) __declspec(uuid(str))
#else /* !_MSC_VER */
// UUID attribute is not supported by gcc-5.2.0.
#define UUID_ATTR(str)
#endif /* _MSC_VER */

// SAL 1.0 annotations not supported by MinGW-w64 5.0.1.
#ifndef __out_opt
#define __out_opt
#endif

// SAL 2.0 annotations not supported by MSVC 2010.
#ifndef _COM_Outptr_
#define _COM_Outptr_
#endif
#ifndef _Outptr_
#define _Outptr_
#endif

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_RPWIN32_SDK_H__ */
