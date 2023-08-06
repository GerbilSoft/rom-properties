/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * tcharx.h: TCHAR support for Windows and Linux.                          *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifdef _WIN32

// Windows: Use the SDK tchar.h.
#include <tchar.h>

// std::tstring, std::tstringstream
#if defined(__cplusplus) && !defined(tstring)
#  ifdef _UNICODE
#    define tstring		wstring
#    define tstringstream	wstringstream
#    define tistringstream	wistringstream
#    define tostringstream	wostringstream
#  else /* !_UNICODE */
#    define tstring		string
#    define tstringstream	stringstream
#    define tistringstream	istringstream
#    define tostringstream	ostringstream
#  endif /* _UNICODE */
#endif /* __cplusplus && !tstring */

#else /* !_WIN32 */

// Other systems: Define TCHAR and related macros.
typedef char TCHAR;
#define _T(x) x
#define _tmain main
#define tstring string

// ctype.h
#define _istalpha(c) isalpha(c)

// stdio.h
#define _fputts(s, stream) fputs((s), (stream))
#define _fputtc(c, stream) fputc((c), (stream))

#define _taccess(pathname, mode)	access((pathname), (mode))
#define _tchdir(path)			chdir(path)
#define _tfopen(pathname, mode)		fopen((pathname), (mode))
#define _tmkdir(path, mode)		mkdir((path), (mode))
#define _tremove(pathname)		remove(pathname)

#define _tprintf printf
#define _ftprintf fprintf
#define _sntprintf snprintf
#define _vtprintf vprintf
#define _vftprintf vfprintf
#define _vsprintf vsprintf

// stdlib.h
#define _tcscmp(s1, s2)			strcmp((s1), (s2))
#define _tcsicmp(s1, s2)		strcasecmp((s1), (s2))
#define _tcsnicmp(s1, s2)		strncasecmp((s1), (s2), (n))
#define _tcstol(nptr, endptr, base)	strtol((nptr), (endptr), (base))
#define _tcstoul(nptr, endptr, base)	strtoul((nptr), (endptr), (base))
#define _tputenv(envstring)		putenv(envstring)
#define _ttol(nptr)			atol(nptr)

// string.h
#define _tcschr(s, c)			strchr((s), (c))
#define _tcscmp(s1, s2)			strcmp((s1), (s2))
#define _tcsdup(s)			strdup(s)
#define _tcserror(errnum)		strerror(errnum)
#define _tcslen(s)			strlen(s)
#define _tcsncmp(s1, s2, n)		strncmp((s1), (s2), (n))
#define _tcsrchr(s, c)			strrchr((s), (c))

#endif /* _WIN32 */

// Directory separator character.
#ifdef _WIN32
#  define DIR_SEP_CHR _T('\\')
#  define DIR_SEP_STR _T("\\")
#else /* !_WIN32 */
#  define DIR_SEP_CHR _T('/')
#  define DIR_SEP_STR _T("/")
#endif /* _WIN32 */
