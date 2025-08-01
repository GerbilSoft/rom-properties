/***************************************************************************
 * ROM Properties Page shell extension. (librpbase/tests)                  *
 * gtest_init.cpp: Google Test initialization.                             *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "gtest_init.hpp"

#include "common.h"
#include "dll-macros.h"

// librpsecure
#include "librpsecure/os-secure.h"

#ifdef _WIN32
// GDI+ initialization.
// NOTE: Not linking to librptexture (libromdata).
#  include "libwin32common/RpWin32_sdk.h"
#  include "libwin32common/rp_versionhelpers.h"
// NOTE: Gdiplus requires min/max.
#  include <algorithm>
namespace Gdiplus {
        using std::min;
        using std::max;
}
#  include <olectl.h>
#  include <gdiplus.h>
#endif /* _WIN32 */

// libfmt
#include "rp-libfmt.h"

// C++ includes.
#include <locale>
#include <vector>
using std::locale;
using std::vector;

#ifdef _WIN32
static UINT old_console_output_cp = 0;
static void RestoreConsoleOutputCP(void)
{
	if (old_console_output_cp != 0) {
		SetConsoleOutputCP(old_console_output_cp);
	}
}
#endif /* _WIN32 */

#ifdef HAVE_SECCOMP
// Each unit test has to specify which set of syscalls are needed.
// The base set is always included.
// NOTE: This is a bitfield.

// Base syscall list for seccomp.
// These syscalls are needed for all tests.
static constexpr int16_t syscall_wl_base[] = {
	// Syscalls used by rom-properties unit tests.
	// TODO: Add more syscalls.
	// FIXME: glibc-2.31 uses 64-bit time syscalls that may not be
	// defined in earlier versions, including Ubuntu 14.04.
	SCMP_SYS(clock_gettime),
#if defined(__SNR_clock_gettime64) || defined(__NR_clock_gettime64)
	SCMP_SYS(clock_gettime64),
#endif /* __SNR_clock_gettime64 || __NR_clock_gettime64 */
	SCMP_SYS(fcntl), SCMP_SYS(fcntl64),		// gcc profiling
	SCMP_SYS(futex),				// iconv_open()
#if defined(__SNR_futex_time64) || defined(__NR_futex_time64)
	SCMP_SYS(futex_time64),				// iconv_open()
#endif /* __SNR_futex_time64 || __NR_futex_time64 */
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

	// RomDataFormat needs this, at least on 32-bit (i386) KF5 builds.
	SCMP_SYS(clock_getres),

	// for ImageDecoderTest so we don't have to copy the test files to the binary directory
	SCMP_SYS(chdir),

	// Google Test
	SCMP_SYS(getcwd),	// testing::internal::FilePath::GetCurrentDir()
				// - testing::internal::UnitTestImpl::AddTestInfo()
	SCMP_SYS(ioctl),	// testing::internal::posix::IsATTY()

	// Needed for some Google Test assertion failures
	SCMP_SYS(getpid),	// also used by gcov if GCOV_LOCKED is defined when compiling gcc
	SCMP_SYS(gettid),
	SCMP_SYS(sched_getaffinity),
	SCMP_SYS(tgkill),	// ???

	// MiniZip
	SCMP_SYS(close),			// mktime() [mz_zip_dosdate_to_time_t()]

	// glibc ncsd
	// TODO: Restrict connect() to AF_UNIX.
	SCMP_SYS(connect), SCMP_SYS(recvmsg), SCMP_SYS(sendto),

	// for posix_fadvise()
	SCMP_SYS(fadvise64), SCMP_SYS(fadvise64_64),
	SCMP_SYS(arm_fadvise64_64),	// CPU-specific syscall for Linux on 32-bit ARM
};

// for Google Test Death Tests when spawning a new process
static constexpr int16_t syscall_wl_gtest_death_test[] = {
	SCMP_SYS(pipe), SCMP_SYS(pipe2),
	SCMP_SYS(dup), SCMP_SYS(dup2),
	SCMP_SYS(getrandom),
	SCMP_SYS(wait4),
	SCMP_SYS(unlink),	// to remove temporary files: /tmp/gtest_captured_stream.XXXXXX
	//SCMP_SYS(execve),	// only used if the above syscalls fail?
};

// for Qt tests
static constexpr int16_t syscall_wl_qt[] = {
	// Qt checks for setuid root and fails if detected:
	// "FATAL: The application binary appears to be running setuid, this is a security hole."
	SCMP_SYS(geteuid), SCMP_SYS(getuid),
	// Other uid/gid functions that aren't directly related to the above error,
	// but are still needed by Qt5/Qt6.
	SCMP_SYS(getegid), SCMP_SYS(getgid),
	SCMP_SYS(getresuid), SCMP_SYS(getresgid),

	// Other syscalls required by Qt
	// TODO: Only enable these syscalls for Qt tests?
	SCMP_SYS(readlink),
	SCMP_SYS(getdents), SCMP_SYS(getdents64),
	// Qt5's QStandardPaths needs mkdir(), but we probably shouldn't allow it...
	// TODO: Stubs that simply return success or failure?
	SCMP_SYS(mkdir),
	SCMP_SYS(socket),
	SCMP_SYS(eventfd2),
	SCMP_SYS(prctl),
	SCMP_SYS(poll), SCMP_SYS(ppoll),
	SCMP_SYS(getsockname),
	SCMP_SYS(sendmsg),
	// Needed on Xubuntu 16.04 by both Qt4 and Qt5 for some reason
	// (Possibly the "minimal" QPA plugin wasn't that minimal?)
	SCMP_SYS(fstatfs),	// Qt4 only
	SCMP_SYS(sysinfo),	// Qt4 only
	SCMP_SYS(statfs),
	SCMP_SYS(getpeername),
	SCMP_SYS(writev),
	SCMP_SYS(recvfrom),
	SCMP_SYS(shutdown),
	SCMP_SYS(shmget), SCMP_SYS(shmat),
	SCMP_SYS(shmctl), SCMP_SYS(shmdt),
	SCMP_SYS(getsockopt),	// Qt4 only
	SCMP_SYS(pipe2),	// Qt4 only
};

// for GTK tests
static constexpr int16_t syscall_wl_gtk[] = {
	/**
	 * GTK3 (probably GTK4 too, not sure about GTK2) checks for setuid root and fails if detected:
	 *
	 * This process is currently running setuid or setgid.
	 * This is not a supported use of GTK+. You must create a helper
	 * program instea.d For further details, see:
	 *
	 *   http://www.gtk.org/setuid.html
	 *
	 * Refusing to initialize GTK+.
	 */

	// NOTE: Ordering here is based mostly on GTK3.
	// GTK2 and GTK4 are similar, but in a different order.
	// Not that it matters at all...
	SCMP_SYS(getresuid),
	SCMP_SYS(geteuid), SCMP_SYS(getuid),
	SCMP_SYS(getegid), SCMP_SYS(getgid),
	SCMP_SYS(getpeername),

	// Additional syscalls needed for initialization.
	SCMP_SYS(getresgid),
	SCMP_SYS(socket),
	SCMP_SYS(prctl),	// for __pthread_setname_np()
	SCMP_SYS(readlink),	// GTK4 needs this
	SCMP_SYS(shutdown),	// GTK3 on Xubuntu 16.04 needs this

	SCMP_SYS(eventfd2),
	SCMP_SYS(sched_getattr),
	SCMP_SYS(sched_setattr),
	SCMP_SYS(getdents), SCMP_SYS(getdents64),

	// D-Bus initialization
	SCMP_SYS(pwrite64),
	SCMP_SYS(sendmsg), SCMP_SYS(recvfrom),
	SCMP_SYS(poll), SCMP_SYS(ppoll),

	// dconf initialization
	SCMP_SYS(mkdir),	// FIXME: Is this actually needed? Stub it?
	SCMP_SYS(memfd_create),
	SCMP_SYS(fallocate),
	SCMP_SYS(getsockname),
	SCMP_SYS(mremap),

	// GTK4
	SCMP_SYS(getrandom),
	SCMP_SYS(setsockopt),
	SCMP_SYS(fstatfs),

	// GTK2
	SCMP_SYS(writev),
	SCMP_SYS(uname),	// NOTE: Only in code coverage builds?
};

#endif /* HAVE_SECCOMP */

extern "C" int gtest_main(int argc, TCHAR *argv[]);

int RP_C_API _tmain(int argc, TCHAR *argv[])
{
	// Set OS-specific security options.
	rp_secure_param_t param;
#if defined(_WIN32)
	param.bHighSec = FALSE;
#elif defined(HAVE_SECCOMP)
	vector<int16_t> syscall_wl;
	syscall_wl.reserve(ARRAY_SIZE(syscall_wl_base) + 64);

	// Add base syscalls.
	syscall_wl.insert(syscall_wl.end(), syscall_wl_base, &syscall_wl_base[ARRAY_SIZE(syscall_wl_base)]);

	if (rp_gtest_syscall_set & RP_GTEST_SYSCALL_SET_GTEST_DEATH_TEST) {
		// Add Google Test death test syscalls.
		syscall_wl.insert(syscall_wl.end(), syscall_wl_gtest_death_test, &syscall_wl_gtest_death_test[ARRAY_SIZE(syscall_wl_gtest_death_test)]);
	}

	if (rp_gtest_syscall_set & RP_GTEST_SYSCALL_SET_QT) {
		// Add Qt syscalls.
		syscall_wl.insert(syscall_wl.end(), syscall_wl_qt, &syscall_wl_qt[ARRAY_SIZE(syscall_wl_qt)]);
	}

	if (rp_gtest_syscall_set & RP_GTEST_SYSCALL_SET_GTK) {
		// Add GTK syscalls.
		syscall_wl.insert(syscall_wl.end(), syscall_wl_gtk, &syscall_wl_gtk[ARRAY_SIZE(syscall_wl_gtk)]);
	}

	// End of syscalls.
	syscall_wl.push_back(-1);
	param.syscall_wl = syscall_wl.data();
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

#ifdef HAVE_SECCOMP
	// TODO: Check for std::vector::shrink_to_fit().
	syscall_wl.clear();
	//syscall_wl.shrink_to_fit();
#endif /* HAVE_SECCOMP */

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
		fmt::print(stderr, FSTR("*** ERROR: GDI+ initialization failed.\n"));
		return EXIT_FAILURE;
	}

	// RpGdiplusBackend will be set up by tests that use it.
#endif /* _WIN32 */

#ifdef __GLIBC__
	// Reduce /etc/localtime stat() calls.
	// References:
	// - https://lwn.net/Articles/944499/
	// - https://gitlab.com/procps-ng/procps/-/merge_requests/119
	setenv("TZ", ":/etc/localtime", 0);
#endif /* __GLIBC__ */

	// Set the C and C++ locales.
	// NOTE: The variable needs to be static char[] because
	// POSIX putenv() takes `char*` and the buffer becomes
	// part of the environment.
#if defined(_WIN32) || defined(__APPLE__)
#  define T_C_LOCALE _T("C")
#  define C_LOCALE "C"
#else /* !_WIN32 && !__APPLE__ */
#  define T_C_LOCALE _T("C.UTF-8")
#  define C_LOCALE "C.UTF-8"
#endif /* _WIN32 || __APPLE__ */
	static TCHAR lc_all_env[] = _T("LC_ALL=") T_C_LOCALE;
	static TCHAR lc_messages_env[] = _T("LC_MESSAGES=") T_C_LOCALE;
	_tputenv(lc_all_env);
	_tputenv(lc_messages_env);

#ifdef __APPLE__
	// Mac OS X also requires setting LC_CTYPE="UTF-8".
	putenv("LC_CTYPE=UTF-8");
#endif /* __APPLE__ */

	// NOTE: MinGW-w64 12.0.0 doesn't like setting the C++ locale to "".
	// Setting it to "C" works fine, though.
	locale::global(locale(C_LOCALE));

#ifdef _WIN32
	// Enable UTF-8 console output.
	// Tested on Windows XP (fails) and Windows 7 (works).
	// TODO: Does it work on Windows Vista?
	// FIXME: On Windows 7, if locale is set to Spanish (es_ES), running
	// `rpcli rpcli.exe` causes a random crash halfway through printing,
	// if we set the console output CP to UTF-8.
	// TODO: Verify if that happens on Windows 8 or 8.1.
	if (IsWindows10OrGreater()) {
		old_console_output_cp = GetConsoleOutputCP();
		atexit(RestoreConsoleOutputCP);
		SetConsoleOutputCP(CP_UTF8);
	}
#endif /* _WIN32 */

	// Call the actual main function.
	int ret = gtest_main(argc, argv);

#ifdef _WIN32
	// Shut down GDI+.
	Gdiplus::GdiplusShutdown(gdipToken);
#endif /* _WIN32 */

	return ret;
}
