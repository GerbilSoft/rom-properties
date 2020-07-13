/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * IDownloader.cpp: Downloader interface.                                  *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "IDownloader.hpp"
#include "config.version.h"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
using std::tstring;

#ifdef __APPLE__
# include <CoreServices/CoreServices.h>
#endif /* __APPLE__ */

namespace RpDownload {

IDownloader::IDownloader()
	: m_mtime(-1)
	, m_inProgress(false)
	, m_maxSize(0)
{
	createUserAgent();
}

IDownloader::IDownloader(const TCHAR *url)
	: m_url(url)
	, m_mtime(-1)
	, m_inProgress(false)
	, m_maxSize(0)
{
	createUserAgent();
}

IDownloader::IDownloader(const tstring &url)
	: m_url(url)
	, m_mtime(-1)
	, m_inProgress(false)
	, m_maxSize(0)
{
	createUserAgent();
}

IDownloader::~IDownloader()
{ }

/** Properties. **/

/**
 * Is a download in progress?
 * @return True if a download is in progress.
 */
bool IDownloader::isInProgress(void) const
{
	return m_inProgress;
}

/**
 * Get the current URL.
 * @return URL.
 */
tstring IDownloader::url(void) const
{
	return m_url;
}

/**
 * Set the URL.
 * @param url New URL.
 */
void IDownloader::setUrl(const TCHAR *url)
{
	assert(!m_inProgress);
	// TODO: Don't set if m_inProgress?
	m_url = url;
}

/**
 * Set the URL.
 * @param url New URL.
 */
void IDownloader::setUrl(const tstring &url)
{
	assert(!m_inProgress);
	// TODO: Don't set if m_inProgress?
	m_url = url;
}

/**
 * Get the maximum buffer size. (0 == unlimited)
 * @return Maximum buffer size.
 */
size_t IDownloader::maxSize(void) const
{
	return m_maxSize;
}

/**
 * Set the maximum buffer size. (0 == unlimited)
 * @param maxSize Maximum buffer size.
 */
void IDownloader::setMaxSize(size_t maxSize)
{
	assert(!m_inProgress);
	// TODO: Don't set if m_inProgress?
	m_maxSize = maxSize;
}

/** Data accessors. **/

/**
 * Get the size of the data.
 * @return Size of the data.
 */
size_t IDownloader::dataSize(void) const
{
	return m_data.size();
}

/**
* Get a pointer to the start of the data.
* @return Pointer to the start of the data.
*/
const uint8_t *IDownloader::data(void) const
{
	return m_data.data();
}

/**
 * Get the Last-Modified time.
 * @return Last-Modified time, or -1 if none was set by the server.
 */
time_t IDownloader::mtime(void) const
{
	return m_mtime;
}

/**
 * Clear the data.
 */
void IDownloader::clear(void)
{
	assert(!m_inProgress);
	// TODO: Don't clear if m_inProgress?
	m_data.clear();
}

#if defined(_WIN32)
/**
 * Get the OS release information.
 * @return OS release information, or empty string if not available.
 */
static tstring getOSRelease(void)
{
	static bool bTriedWow64 = false;
	static bool bIsWow64 = false;

	// Get the OS version number.
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionEx(&osvi);

	tstring s_os_version;
	switch (osvi.dwPlatformId) {
		case VER_PLATFORM_WIN32s:
			// Good luck with that.
			s_os_version = _T("Win32s");
			break;
		case VER_PLATFORM_WIN32_WINDOWS:
		default:
			s_os_version = _T("Windows");
			break;
		case VER_PLATFORM_WIN32_NT:
			s_os_version = _T("Windows NT");
			break;
	}
	s_os_version += _T(' ');

	// Version number.
	TCHAR buf[32];
	_sntprintf(buf, _countof(buf), _T("%u.%u"), osvi.dwMajorVersion, osvi.dwMinorVersion);
	s_os_version += buf;

#  ifdef _WIN64
	s_os_version += _T("; Win64");
#  else /* !_WIN64 */
	// Check for WOW64.
	if (!bTriedWow64) {
		HMODULE hKernel32 = GetModuleHandle(_T("kernel32"));
		assert(hKernel32 != nullptr);
		if (hKernel32) {
			typedef BOOL (WINAPI *PFNISWOW64PROCESS)(HANDLE hProcess, PBOOL Wow64Process);
			PFNISWOW64PROCESS pfnIsWow64Process = (PFNISWOW64PROCESS)GetProcAddress(hKernel32, "IsWow64Process");

			if (pfnIsWow64Process) {
				BOOL bIsWow64Process = FALSE;
				BOOL bRet = IsWow64Process(GetCurrentProcess(), &bIsWow64Process);
				if (bRet && bIsWow64Process) {
					bIsWow64 = true;
				}
			}
		}
		bTriedWow64 = true;
	}

	if (bIsWow64) {
		s_os_version += _T("; WOW64");
	}
#  endif /* _WIN64 */

	return s_os_version;
}
#elif defined(__linux__)
/**
 * Get the OS release information.
 * @return OS release information, or empty string if not available.
 */
static tstring getOSRelease(void)
{
	// Reference: https://www.freedesktop.org/software/systemd/man/os-release.html

	// TODO: Distro and/or kernel version?
	FILE *f_in = fopen("/etc/os-release", "r");
	if (!f_in) {
		f_in = fopen("/usr/lib/os-release", "r");
		if (!f_in) {
			// os-release file not found.
			return tstring();
		}
	}

	// Find the "NAME=" line.
	tstring distro_name;
	char buf[256];
	for (unsigned int line_count = 0; !feof(f_in) && line_count < 32; line_count++) {
		char *line = fgets(buf, sizeof(buf), f_in);
		if (!line)
			break;

		// Remove leading spaces.
		while (*line != '\0' && ISSPACE(*line)) {
			line++;
		}
		if (*line == '\0')
			continue;

		if (strncmp(line, "NAME=", 5) != 0) {
			// Not "NAME=".
			continue;
		}

		line += 5;
		if (*line == '\0')
			continue;

		// Remove spaces at the end.
		size_t len = strlen(line);
		while (len > 0 && ISSPACE(line[len-1])) {
			len--;
		}
		if (len == 0)
			continue;

		// Check if we need to remove double-quotes.
		if (*line == '"') {
			// Check if the last character is double-quotes.
			if (len >= 2 && line[len-1] == '"') {
				// It's double-quotes.
				line++;
				len -= 2;
			}
		}

		// Line found.
		distro_name.assign(line, len);
		break;
	}

	return distro_name;
}
#endif

/**
 * Create the User-Agent value.
 */
void IDownloader::createUserAgent(void)
{
	m_userAgent.reserve(256);
	m_userAgent = _T("rom-properties/") _T(RP_VERSION_STRING);

	// CPU
#if defined(_M_ARM64) || defined(__aarch64__)
# define CPU "ARM64"
# define MAC_CPU "ARM64"
#elif defined(_M_ARM) || defined(__arm__)
# define CPU "ARM"
# define MAC_CPU "ARM"
#elif defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__x86_64__)
# ifdef _WIN32
#  define CPU "x64"
# else /* !_WIN32 */
#  define CPU "x86_64"
# endif
# define MAC_CPU "Intel"
#elif defined(_M_IX86) || defined(__i386__)
# ifdef _WIN32
#  define CPU ""
#  define NO_CPU 1
# else /* !_WIN32 */
#  define CPU "i386"
# endif /* _WIN32 */
# define MAC_CPU "Intel"
#elif defined(__powerpc64__) || defined(__ppc64__)
# define CPU "PPC64"
# define MAC_CPU "PPC"
#elif defined(__powerpc__) || defined(__ppc__)
# define CPU "PPC"
# define MAC_CPU "PPC"
#endif

#ifdef _WIN32
	m_userAgent += _T(" (");
	const tstring s_os_version = getOSRelease();
	m_userAgent += s_os_version;
#  ifndef NO_CPU
	if (!s_os_version.empty()) {
		m_userAgent += _T("; ") _T(CPU);
	}
#  endif /* NO_CPU */
	m_userAgent += _T(')');
#elif defined(__linux__)
	const tstring os_release = getOSRelease();
	m_userAgent += _T(" (");
	if (!os_release.empty()) {
		m_userAgent += os_release;
		m_userAgent += _T("; ");
	}
	m_userAgent += _T("Linux ") _T(CPU) _T(")");
#elif defined(__FreeBSD__)
	// TODO: Distribution version?
	m_userAgent += _T(" (FreeBSD; ") _T(CPU) _T(")");
#elif defined(__NetBSD__)
	// TODO: Distribution version?
	m_userAgent += _T(" (NetBSD; ") _T(CPU) _T(")");
#elif defined(__OpenBSD__)
	// TODO: Distribution version?
	m_userAgent += _T(" (OpenBSD; ") _T(CPU) _T(")");
#elif defined(__bsdi__)
	// TODO: Distribution version?
	m_userAgent += _T(" (BSDi; ") _T(CPU) _T(")");
#elif defined(__DragonFly__)
	// TODO: Distribution version?
	m_userAgent += _T(" (DragonFlyBSD; ") _T(CPU) _T(")");
#elif defined(__APPLE__)
	// Get the OS version.
	// TODO: Include the bugfix version?
	SInt32 major, minor;
	//SInt32 bugfix;

	Gestalt(gestaltSystemVersionMajor, &major);
	Gestalt(gestaltSystemVersionMinor, &minor);
	//Gestalt(gestaltSystemVersionBugFix, &bugfix);

	char buf[32];
	snprintf(buf, sizeof(buf), "%d.%d", major, minor);
	m_userAgent += _T(" (Macintosh; ") _T(MAC_CPU) _T(" Mac OS X ");
	m_userAgent += buf;
	m_userAgent += _T(')');
#elif defined(__unix__)
	// Generic UNIX fallback.
	m_userAgent += _T(" (Unix; ") _T(CPU) _T(")");
#else
	// Unknown OS...
	m_userAgent += _T(" (Unknown; ") _T(CPU) _T(")");
#endif
}

}
