/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * gtest_init.c: Google Test initialization.                               *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// C++ includes.
#include <locale>
using std::locale;

#include "dll-macros.h"
#include "tcharx.h"

// librpsecure
#include "librpsecure/os-secure.h"

#ifdef _WIN32
// GDI+ initialization.
// NOTE: Not linking to librptexture (libromdata).
#  include "libwin32common/RpWin32_sdk.h"
// NOTE: Gdiplus requires min/max.
#  include <algorithm>
namespace Gdiplus {
        using std::min;
        using std::max;
}
#  include <olectl.h>
#  include <gdiplus.h>
#endif /* _WIN32 */

extern "C" int gtest_main(int argc, TCHAR *argv[]);

int RP_C_API _tmain(int argc, TCHAR *argv[])
{
	// Set OS-specific security options.
	rp_secure_param_t param;
#if defined(_WIN32)
	param.bHighSec = FALSE;
#elif defined(HAVE_SECCOMP)
	static const int syscall_wl[] = {
		// Syscalls used by rom-properties unit tests.
		// TODO: Add more syscalls.
		// FIXME: glibc-2.31 uses 64-bit time syscalls that may not be
		// defined in earlier versions, including Ubuntu 14.04.
		SCMP_SYS(clock_gettime),
#if defined(__SNR_clock_gettime64) || defined(__NR_clock_gettime64)
		SCMP_SYS(clock_gettime64),
#endif /* __SNR_clock_gettime64 || __NR_clock_gettime64 */
		SCMP_SYS(fcntl),     SCMP_SYS(fcntl64),		// gcc profiling
		SCMP_SYS(fstat),     SCMP_SYS(fstat64),		// __GI___fxstat() [printf()]
		SCMP_SYS(fstatat64), SCMP_SYS(newfstatat),	// Ubuntu 19.10 (32-bit)
		SCMP_SYS(futex),	// iconv_open()
		SCMP_SYS(gettimeofday),	// 32-bit only? [testing::internal::GetTimeInMillis()]
		SCMP_SYS(mmap),		// iconv_open()
		SCMP_SYS(mmap2),	// iconv_open() [might only be needed on i386...]
		SCMP_SYS(mprotect),	// iconv_open()
		SCMP_SYS(munmap),	// free() [in some cases]
		SCMP_SYS(lseek), SCMP_SYS(_llseek),
		SCMP_SYS(lstat), SCMP_SYS(lstat64),		// LibRpBase::FileSystem::is_symlink(), resolve_symlink()
		SCMP_SYS(open),		// Ubuntu 16.04
		SCMP_SYS(openat),	// glibc-2.31
#if defined(__SNR_openat2)
		SCMP_SYS(openat2),	// Linux 5.6
#elif defined(__NR_openat2)
		__NR_openat2,		// Linux 5.6
#endif /* __SNR_openat2 || __NR_openat2 */

		// Google Test
		SCMP_SYS(getcwd),	// testing::internal::FilePath::GetCurrentDir()
					// - testing::internal::UnitTestImpl::AddTestInfo()
		SCMP_SYS(ioctl),	// testing::internal::posix::IsATTY()

		// MiniZip
		SCMP_SYS(close),	// mktime() [mz_zip_dosdate_to_time_t()]
		SCMP_SYS(stat), SCMP_SYS(stat64),	// mktime() [mz_zip_dosdate_to_time_t()]

		// glibc ncsd
		// TODO: Restrict connect() to AF_UNIX.
		SCMP_SYS(connect), SCMP_SYS(recvmsg), SCMP_SYS(sendto),

#if defined(__SNR_statx) || defined(__NR_statx)
		//SCMP_SYS(getcwd),	// called by glibc's statx() [referenced above]
		SCMP_SYS(statx),
#endif /* __SNR_statx || __NR_statx */

		-1	// End of whitelist
	};
	param.syscall_wl = syscall_wl;
	param.threading = true;		// FIXME: Only if OpenMP is enabled?
#elif defined(HAVE_PLEDGE)
	// Promises:
	// - stdio: General stdio functionality.
	// - rpath: Read test cases.
	param.promises = "stdio rpath";
#elif defined(HAVE_TAME)
	param.tame_flags = TAME_STDIO | TAME_RPATH;
#else
	param.dummy = 0;
#endif
	rp_secure_enable(param);

#ifdef _WIN32
	// Initialize GDI+.
	// NOTE: Not linking to librptexture (libromdata),
	// so we have to do the initialization here.
	Gdiplus::GdiplusStartupInput gdipSI;
	gdipSI.GdiplusVersion = 1;
	gdipSI.DebugEventCallback = nullptr;
	gdipSI.SuppressBackgroundThread = FALSE;
	gdipSI.SuppressExternalCodecs = FALSE;
	ULONG_PTR gdipToken;
	Gdiplus::Status status = GdiplusStartup(&gdipToken, &gdipSI, nullptr);
	if (status != Gdiplus::Status::Ok) {
		fputs("*** ERROR: GDI+ initialization failed.\n", stderr);
		return EXIT_FAILURE;
	}

	// RpGdiplusBackend will be set up by tests that use it.
#endif /* _WIN32 */

	// Set the C and C++ locales.
	// NOTE: The variable needs to be static char[] because
	// POSIX putenv() takes `char*` and the buffer becomes
	// part of the environment.
	static TCHAR lc_all_env[] = _T("LC_ALL=C");
	_tputenv(lc_all_env);
	locale::global(locale("C"));

	// Call the actual main function.
	int ret = gtest_main(argc, argv);
#ifdef _WIN32
	// Shut down GDI+.
	Gdiplus::GdiplusShutdown(gdipToken);
#endif /* _WIN32 */
	return ret;
}
