/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * IDownloader.cpp: Downloader interface.                                  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.version.h"
#include "IDownloader.hpp"

// C includes (C++ namespace)
#include <cstdio>

// C++ includes
#include <string>
using std::tstring;

#ifdef __linux__
#  include "ini.h"
#endif /* __linux__ */

#ifdef __APPLE__
#  include <CoreServices/CoreServices.h>
#endif /* __APPLE__ */

namespace RpDownload {

IDownloader::IDownloader()
	: m_mtime(-1)
	, m_if_modified_since(-1)
	, m_maxSize(0)
	, m_inProgress(false)
{
	createUserAgent();
}

IDownloader::IDownloader(const TCHAR *url)
	: m_url(url)
	, m_mtime(-1)
	, m_if_modified_since(-1)
	, m_maxSize(0)
	, m_inProgress(false)
{
	createUserAgent();
}

IDownloader::IDownloader(const tstring &url)
	: m_url(url)
	, m_mtime(-1)
	, m_if_modified_since(-1)
	, m_maxSize(0)
	, m_inProgress(false)
{
	createUserAgent();
}

/** Properties **/

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

/**
 * Get the If-Modified-Since request timestamp.
 * @return If-Modified-Since timestamp (-1 for none)
 */
time_t IDownloader::ifModifiedSince(void) const
{
	return m_if_modified_since;
}

/**
 * Set the If-Modified-Since request timestamp.
 * @param timestamp If-Modified-Since timestamp (-1 for none)
 */
void IDownloader::setIfModifiedSince(time_t timestamp)
{
	assert(!m_inProgress);
	// TODO: Don't set if m_inProgress?
	m_if_modified_since = timestamp;
}

/** Data accessors **/

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

#ifdef __linux__
struct inih_ctx {
	const char *field_name;
	char ret_value[64];

	explicit inih_ctx(const char *field_name = nullptr)
		: field_name(field_name)
	{
		ret_value[0] = '\0';
	}
};

/**
 * inih parsing function for /etc/os-release and /etc/lsb-release.
 * @param user Pointer to inih_ctx.
 * @param section Section
 * @param name Key
 * @param value Value
 * @return 1 to continue; 0 to stop processing.
 */
static int parse_os_release(void *user, const char *section, const char *name, const char *value)
{
	// We want to find the specified field name,
	// which is not contained within a section.
	if (section[0] != '\0') {
		// We're in a section.
		return 1;
	}

	inih_ctx *const ctx = static_cast<inih_ctx*>(user);
	if (!strcmp(ctx->field_name, name)) {
		// Found the field.
		// TODO: strlcpy() or snprintf() or similar?
		strncpy(ctx->ret_value, value, sizeof(ctx->ret_value));
		ctx->ret_value[sizeof(ctx->ret_value)-1] = '\0';
		return 0;
	}

	// Continue processing.
	return 0;
}
#endif /* __linux__ */

/**
 * Get the OS release information.
 * @return OS release information, or empty string if not available.
 */
tstring IDownloader::getOSRelease(void)
{
	tstring s_os_release;

#if defined(_WIN32)
	// Get the OS version number.
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionEx(&osvi);

	switch (osvi.dwPlatformId) {
		case VER_PLATFORM_WIN32s:
			// Good luck with that.
			s_os_release = _T("Win32s");
			break;
		case VER_PLATFORM_WIN32_WINDOWS:
		default:
			s_os_release = _T("Windows");
			break;
		case VER_PLATFORM_WIN32_NT:
			s_os_release = _T("Windows NT");
			break;
	}
	s_os_release += _T(' ');

	// Version number
	TCHAR buf[32];
	if (osvi.dwMajorVersion == 10 && osvi.dwBuildNumber >= 20000) {
		// Windows 11
		osvi.dwMajorVersion = 11;
	}
	_sntprintf(buf, _countof(buf), _T("%lu.%lu"), osvi.dwMajorVersion, osvi.dwMinorVersion);
	s_os_release += buf;

#  ifdef _WIN64
	s_os_release += _T("; Win64");
#  else /* !_WIN64 */
	// Check for WOW64.
	static bool bTriedWow64 = false;
	static bool bIsWow64 = false;
	if (!bTriedWow64) {
		HMODULE hKernel32 = GetModuleHandle(_T("kernel32.dll"));
		assert(hKernel32 != nullptr);
		if (hKernel32) {
			typedef BOOL (WINAPI *pfnIsWow64Process_t)(HANDLE hProcess, PBOOL Wow64Process);
			pfnIsWow64Process_t pfnIsWow64Process = (pfnIsWow64Process_t)GetProcAddress(hKernel32, "IsWow64Process");

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
		s_os_release += _T("; WOW64");
	}
#  endif /* _WIN64 */

#elif defined(__linux__)

	// Reference: https://www.freedesktop.org/software/systemd/man/os-release.html

	// TODO: Distro and/or kernel version?
	inih_ctx ctx;
	FILE *f_in = fopen("/etc/os-release", "r");
	if (f_in) {
		ctx.field_name = "NAME";
	} else {
		f_in = fopen("/usr/lib/os-release", "r");
		if (f_in) {
			ctx.field_name = "NAME";
		} else {
			// os-release file not found.
			// Try the older lsb-release file.
			f_in = fopen("/etc/lsb-release", "r");
			if (f_in) {
				ctx.field_name = "DISTRIB_ID";
			}
		}
	}
	if (!ctx.field_name) {
		// No field name...
		if (f_in) {
			fclose(f_in);
		}
		return {};
	}

	// Find the requested field.
	ini_parse_file(f_in, parse_os_release, &ctx);
	fclose(f_in);
	if (ctx.ret_value[0] == '\0') {
		// Field not found.
		return {};
	}

	// Remove leading and trailing double-quotes, if present.
	const char *os_release = ctx.ret_value;
	if (os_release[0] == '"') {
		os_release++;
	}
	s_os_release.assign(os_release);
	if (!s_os_release.empty() && s_os_release[s_os_release.size()-1] == '"') {
		s_os_release.resize(s_os_release.size()-1);
	}
#endif

	return s_os_release;
}

/**
 * Create the User-Agent value.
 */
void IDownloader::createUserAgent(void)
{
	m_userAgent.reserve(256);
	m_userAgent = _T("rom-properties/") _T(RP_VERSION_STRING);

	// CPU
#if defined(_M_ARM64EC)
#  define CPU "ARM64EC"
#  define MAC_CPU "ARM64EC"
#elif defined(_M_ARM64) || defined(__aarch64__)
#  define CPU "ARM64"
#  define MAC_CPU "ARM64"
#elif defined(_M_ARM) || defined(__arm__)
#  define CPU "ARM"
#  define MAC_CPU "ARM"
#elif defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__x86_64__)
#  ifdef _WIN32
#    define CPU "x64"
#  else /* !_WIN32 */
#    define CPU "x86_64"
#  endif
#  define MAC_CPU "Intel"
#elif defined(_M_IX86) || defined(__i386__)
#  ifdef _WIN32
#    define CPU ""
#    define NO_CPU 1
#  else /* !_WIN32 */
#    define CPU "i386"
#  endif /* _WIN32 */
#  define MAC_CPU "Intel"
#elif defined(__powerpc64__) || defined(__ppc64__)
#  define CPU "PPC64"
#  define MAC_CPU "PPC"
#elif defined(__powerpc__) || defined(__ppc__)
#  define CPU "PPC"
#  define MAC_CPU "PPC"
#elif defined(__riscv) && (__riscv_xlen == 32)
#  define CPU "riscv32"
#  define MAC_CPU "riscv32"
#elif defined(__riscv) && (__riscv_xlen == 64)
#  define CPU "riscv64"
#  define MAC_CPU "riscv64"
#else
#  error Unknown CPU, please update this
#endif

#ifdef _WIN32
	m_userAgent += _T(" (");
	const tstring s_os_release = getOSRelease();
	m_userAgent += s_os_release;
#  ifndef NO_CPU
	if (!s_os_release.empty()) {
		m_userAgent += _T("; ") _T(CPU);
	}
#  endif /* NO_CPU */
	m_userAgent += _T(')');
#elif defined(__linux__)
	const tstring s_os_release = getOSRelease();
	m_userAgent += _T(" (");
	if (!s_os_release.empty()) {
		m_userAgent += s_os_release;
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

} //namespace RpDownload
