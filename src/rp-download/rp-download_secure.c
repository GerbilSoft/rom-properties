/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * rp-download_secure.h: Security options for rp-download.                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "rp-download_secure.h"

// librpsecure
#include "librpsecure/os-secure.h"
#include "librpsecure/restrict-dll.h"

/**
 * Enable security options.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_download_do_security_options(void)
{
#ifdef _WIN32
	// Suppress Windows "critical" error dialogs.
	// This is a legacy MS-DOS holdover, e.g. the "Abort, Retry, Fail" prompt.
	SetErrorMode(SEM_FAILCRITICALERRORS);
#endif /* !_WIN32 */

	// Restrict DLL lookups.
	rp_secure_restrict_dll_lookups();
	// Reduce process integrity, if available.
	rp_secure_reduce_integrity();

	// Set OS-specific security options.
	rp_secure_param_t param;
#if defined(_WIN32)
	param.bHighSec = FALSE;
#elif defined(HAVE_SECCOMP)
	static const int16_t syscall_wl[] = {
		// Syscalls used by rp-download.
		// TODO: Add more syscalls.
		// FIXME: glibc-2.31 uses 64-bit time syscalls that may not be
		// defined in earlier versions, including Ubuntu 14.04.
		SCMP_SYS(clock_gettime),
#if defined(__SNR_clock_gettime64) || defined(__NR_clock_gettime64)
		SCMP_SYS(clock_gettime64),
#endif /* __SNR_clock_gettime64 || __NR_clock_gettime64 */
		SCMP_SYS(close),
		SCMP_SYS(fcntl),     SCMP_SYS(fcntl64),		// gcc profiling
		SCMP_SYS(fsetxattr),
		SCMP_SYS(futex),
#if defined(__SNR_futex_time64) || defined(__NR_futex_time64)
		SCMP_SYS(futex_time64),
#endif /* __SNR_futex_time64 || __NR_futex_time64 */
		SCMP_SYS(getdents), SCMP_SYS(getdents64),
		SCMP_SYS(getppid),	// for bubblewrap verification
		SCMP_SYS(getrusage),
		SCMP_SYS(gettimeofday),	// 32-bit only?
		SCMP_SYS(getuid),	// TODO: Only use geteuid()?
		SCMP_SYS(lseek), SCMP_SYS(_llseek),
		//SCMP_SYS(lstat), SCMP_SYS(lstat64),	// Not sure if used?
		SCMP_SYS(mkdir), SCMP_SYS(mmap), SCMP_SYS(mmap2),
		SCMP_SYS(munmap),
		SCMP_SYS(open),		// Ubuntu 16.04
		SCMP_SYS(openat),	// glibc-2.31
#if defined(__SNR_openat2)
		SCMP_SYS(openat2),	// Linux 5.6
#elif defined(__NR_openat2)
		__NR_openat2,		// Linux 5.6
#endif /* __SNR_openat2 || __NR_openat2 */
		SCMP_SYS(poll), SCMP_SYS(select),
		SCMP_SYS(unlink),	// to delete expired cache files
		SCMP_SYS(utimensat),

		// glibc ncsd
		// TODO: Restrict connect() to AF_UNIX.
		SCMP_SYS(connect), SCMP_SYS(recvmsg), SCMP_SYS(sendto),
		SCMP_SYS(sendmmsg),	// getaddrinfo() (32-bit only?)
		SCMP_SYS(ioctl),	// getaddrinfo() (32-bit only?) [FIXME: Filter for FIONREAD]
		SCMP_SYS(recvfrom),	// getaddrinfo() (32-bit only?)

		// Needed for network access on Kubuntu 20.04 for some reason.
		SCMP_SYS(getpid), SCMP_SYS(uname),

		// cURL and OpenSSL
		SCMP_SYS(bind),		// getaddrinfo() [curl_thread_create_thunk(), curl-7.68.0]
		SCMP_SYS(eventfd2),	// curl-8.11.1 (actually added in 8.9.0, but didn't work until 8.11.1)
#ifdef __SNR_getrandom
		SCMP_SYS(getrandom),
#endif /* __SNR_getrandom */
		SCMP_SYS(getpeername), SCMP_SYS(getsockname),
		SCMP_SYS(getsockopt), SCMP_SYS(madvise), SCMP_SYS(mprotect),
		SCMP_SYS(setsockopt), SCMP_SYS(socket),
		SCMP_SYS(socketcall),	// FIXME: Enhanced filtering? [cURL+GnuTLS only?]
		SCMP_SYS(socketpair), SCMP_SYS(sysinfo),
		SCMP_SYS(rt_sigprocmask),	// Ubuntu 20.04: __GI_getaddrinfo() ->
						// gaih_inet() ->
						// _nss_myhostname_gethostbyname4_r()
		SCMP_SYS(getuid32),	// Ubuntu 16.04: RAND_status() -> RAND_poll() [i386 only]

		// libnss_resolve.so (systemd-resolved)
		SCMP_SYS(geteuid),
		SCMP_SYS(sendmsg),	// libpthread.so [_nss_resolve_gethostbyname4_r() from libnss_resolve.so]

		// FIXME: Manjaro is using these syscalls for some reason...
		SCMP_SYS(prctl), SCMP_SYS(mremap), SCMP_SYS(ppoll),

		// cURL's "easy" functions use multi internally, which uses pipe().
		// Some update, either cURL 8.4.0 -> 8.5.0 or glibc 2.38 -> 2.39,
		// is now using the pipe2() syscall.
		SCMP_SYS(pipe2),

		// Needed on 32-bit Ubuntu 16.04 (glibc-2.23, cURL 7.47.0) for some reason...
		// (called from getaddrinfo())
		SCMP_SYS(time),

		// Needed by cURL 8.13 for QUIC (HTTP/3).
		SCMP_SYS(recvmmsg),

		-1	// End of whitelist
	};
	param.syscall_wl = syscall_wl;
	param.threading = true;		// libcurl uses multi-threading.
#elif defined(HAVE_PLEDGE)
	// Promises:
	// - stdio: General stdio functionality.
	// - rpath: Read from ~/.config/rom-properties/ and ~/.cache/rom-properties/
	// - wpath: Write to ~/.cache/rom-properties/
	// - cpath: Create ~/.cache/rom-properties/ if it doesn't exist.
	// - inet: Internet access.
	// - fattr: Modify file attributes, e.g. mtime.
	// - dns: Resolve hostnames.
	// - getpw: Get user's home directory if HOME is empty.
	param.promises = "stdio rpath wpath cpath inet fattr dns getpw";
#elif defined(HAVE_TAME)
	// NOTE: stdio includes fattr, e.g. utimes().
	param.tame_flags = TAME_STDIO | TAME_RPATH | TAME_WPATH | TAME_CPATH |
	                   TAME_INET | TAME_DNS | TAME_GETPW;
#else
	param.dummy = 0;
#endif

	return rp_secure_enable(param);
}
