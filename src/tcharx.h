/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * tcharx.h: TCHAR support for Windows and Linux.                          *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// NOTE: tcharx.h has _sntprintf(), but not sntprintf().
// As of MSVC 2015, snprintf() always NULL-terminates the buffer,
// but _snprintf(), and thus _sntprintf(), does *not* in some cases.
// We're switching to our own sntprintf() macro to handle this.
// Reference: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/snprintf-snprintf-snprintf-l-snwprintf-snwprintf-l?view=msvc-170
#ifdef _sntprintf
#  undef _sntprintf
#endif
#define _snprintf __DO_NOT_USE_OLD_MSVC__SNPRINTF
#define _sntprintf __DO_NOT_USE_OLD_MSVC__SNTPRINTF
#define _snwprintf __DO_NOT_USE_OLD_MSVC__SNWPRINTF
#define _swprintf __DO_NOT_USE_OLD_MSVC__SWPRINTF

#ifdef _WIN32

// Windows: Use the SDK tchar.h.
#include <tchar.h>

#define sntprintf swprintf

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

#ifdef _UNICODE
#  define _tmemcmp(s1, s2, n)	wmemcmp((s1), (s2), (n))

/**
 * _tmemcmp_inline() for Windows unicode strings.
 * Neither gcc nor MSVC will inline wmemcmp(), so
 * convert it to memcmp().
 * @param s1
 * @param s2
 * @param n
 */
static inline int _tmemcmp_inline(const TCHAR *s1, const TCHAR *s2, size_t n)
{
	return memcmp(s1, s2, n * sizeof(TCHAR));
}

#else /* !_UNICODE */
#  define sntprintf snprintf
#  define _tmemcmp(s1, s2, n)		memcmp((s1), (s2), (n))
#  define _tmemcmp_inline(s1, s2, n)	memcmp((s1), (s2), (n))
#endif /* _UNICODE */

#else /* !_WIN32 */

// Other systems: Define TCHAR and related macros.
typedef char TCHAR;
#define _T(x) x
#define _tmain main
#define tstring string

// ctype.h
#define _istalpha(c) isalpha(c)

// stdio.h
#define sntprintf snprintf

#define _fputts(s, stream) fputs((s), (stream))
#define _fputtc(c, stream) fputc((c), (stream))

#define _taccess(pathname, mode)	access((pathname), (mode))
#define _tchdir(path)			chdir(path)
#define _tfopen(pathname, mode)		fopen((pathname), (mode))
#define _tmkdir(path, mode)		mkdir((path), (mode))
#define _tremove(pathname)		remove(pathname)

#define _tprintf printf
#define _ftprintf fprintf
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
#define _tcscat(dst, src)		strcat((dst), (src))
#define _tcschr(s, c)			strchr((s), (c))
#define _tcscmp(s1, s2)			strcmp((s1), (s2))
#define _tcsdup(s)			strdup(s)
#define _tcserror(errnum)		strerror(errnum)
#define _tcslen(s)			strlen(s)
#define _tcsncmp(s1, s2, n)		strncmp((s1), (s2), (n))
#define _tcsrchr(s, c)			strrchr((s), (c))
#define _tmemcmp(s1, s2, n)		memcmp((s1), (s2), (n))
#define _tmemcmp_inline(s1, s2, n)	memcmp((s1), (s2), (n))

// direct.h (unistd.h)
#define _tgetcwd(buf, size)		getcwd((buf), (size))

#endif /* _WIN32 */

// Directory separator character.
#ifdef _WIN32
#  define DIR_SEP_CHR _T('\\')
#  define DIR_SEP_STR _T("\\")
#else /* !_WIN32 */
#  define DIR_SEP_CHR _T('/')
#  define DIR_SEP_STR _T("/")
#endif /* _WIN32 */
