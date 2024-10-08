/***************************************************************************
 * ROM Properties Page shell extension. (D-Bus Thumbnailer)                *
 * rptsecure.c: Security options for rp-thumbnailer-dbus.                  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "rptsecure.h"
#include "librpsecure/os-secure.h"

/**
 * Enable security options.
 * @return 0 on success; negative POSIX error code on error.
 */
int rpt_do_security_options(void)
{
	// FIXME: rp-download may be called by the stub, and any process
	// exec()'d by us inherits the seccomp filter, which *will* break
	// things, since child processes cannot enable syscalls if they
	// weren't enabled here.
	return 0;

#if 0
	// Set OS-specific security options.
	rp_secure_param_t param;
#if defined(_WIN32)
	param.bHighSec = FALSE;
#elif defined(HAVE_SECCOMP)
	static constexpr int syscall_wl[] = {
		// Syscalls used by rp-download.
		// TODO: Add more syscalls.
		// FIXME: glibc-2.31 uses 64-bit time syscalls that may not be
		// defined in earlier versions, including Ubuntu 14.04.

		SCMP_SYS(close),
		SCMP_SYS(dup),		// gzdopen()
		SCMP_SYS(fcntl),     SCMP_SYS(fcntl64),		// gcc profiling
		SCMP_SYS(fstat),     SCMP_SYS(fstat64),		// __GI___fxstat() [printf()]
		SCMP_SYS(fstatat64), SCMP_SYS(newfstatat),	// Ubuntu 19.10 (32-bit)
		SCMP_SYS(ftruncate),	// LibRpBase::RpFile::truncate() [from LibRpBase::RpPngWriterPrivate ctors]
		SCMP_SYS(ftruncate64),
		SCMP_SYS(futex),	// iconv_open(), dlopen()
		SCMP_SYS(gettimeofday),	// 32-bit only?
		SCMP_SYS(getppid),	// dll-search.c: walk_proc_tree()
		SCMP_SYS(getuid),	// TODO: Only use geteuid()?
		SCMP_SYS(lseek), SCMP_SYS(_llseek),
		SCMP_SYS(lstat), SCMP_SYS(lstat64),	// LibRpBase::FileSystem::is_symlink(), resolve_symlink()
		SCMP_SYS(mkdir),	// g_mkdir_with_parents() [rp_thumbnailer_process()]
		SCMP_SYS(mmap),		// iconv_open(), dlopen()
		SCMP_SYS(mmap2),	// iconv_open(), dlopen() [might only be needed on i386...]
		SCMP_SYS(munmap),	// dlopen(), free() [in some cases]
		SCMP_SYS(mprotect),	// iconv_open()
		SCMP_SYS(open),		// Ubuntu 16.04
		SCMP_SYS(openat),	// glibc-2.31
#if defined(__SNR_openat2)
		SCMP_SYS(openat2),	// Linux 5.6
#elif defined(__NR_openat2)
		__NR_openat2,		// Linux 5.6
#endif /* __SNR_openat2 || __NR_openat2 */
		SCMP_SYS(readlink),	// realpath() [LibRpBase::FileSystem::resolve_symlink()]
		SCMP_SYS(stat), SCMP_SYS(stat64),	// LibUnixCommon::isWritableDirectory()
		SCMP_SYS(statfs), SCMP_SYS(statfs64),	// LibRpBase::FileSystem::isOnBadFS()

		// ConfReader checks timestamps between rpcli runs.
		// NOTE: Only seems to get triggered on PowerPC...
		SCMP_SYS(clock_gettime), SCMP_SYS(clock_gettime64),

#if defined(__SNR_statx) || defined(__NR_statx)
		SCMP_SYS(getcwd),	// called by glibc's statx()
		SCMP_SYS(statx),
#endif /* __SNR_statx || __NR_statx */

		// glibc ncsd
		// TODO: Restrict connect() to AF_UNIX.
		SCMP_SYS(connect), SCMP_SYS(recvmsg), SCMP_SYS(sendto),

		// Needed for network access on Kubuntu 20.04 for some reason.
		SCMP_SYS(getpid), SCMP_SYS(uname),

		// glib / D-Bus
		SCMP_SYS(eventfd2),
		SCMP_SYS(fcntl), SCMP_SYS(fcntl64),
		SCMP_SYS(getdents), SCMP_SYS(getdents64)	// g_file_new_for_uri() [rp_create_thumbnail()]
		SCMP_SYS(getegid), SCMP_SYS(geteuid), SCMP_SYS(poll),
		SCMP_SYS(recvfrom), SCMP_SYS(sendmsg), SCMP_SYS(socket),
		SCMP_SYS(socketcall),	// FIXME: Enhanced filtering? [cURL+GnuTLS only?]
		SCMP_SYS(socketpair), SCMP_SYS(sysinfo),
		SCMP_SYS(rt_sigprocmask),	// Ubuntu 20.04: __GI_getaddrinfo() ->
						// gaih_inet() ->
						// _nss_myhostname_gethostbyname4_r()

		// only if G_MESSAGES_DEBUG=all [on Gentoo, but not Ubuntu 14.04]
		SCMP_SYS(getpeername),	// g_log_writer_is_journald() [g_log()]
		SCMP_SYS(ioctl),	// isatty() [g_log()]

		// TODO: Parameter filtering for prctl().
		SCMP_SYS(prctl),	// pthread_setname_np() [g_thread_proxy(), start_thread()]

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
	// - getpw: Get user's home directory if HOME is empty.
	param.promises = "stdio rpath wpath cpath getpw";
#elif defined(HAVE_TAME)
	// NOTE: stdio includes fattr, e.g. utimes().
	param.tame_flags = TAME_STDIO | TAME_RPATH | TAME_WPATH | TAME_CPATH | TAME_GETPW;
#else
	param.dummy = 0;
#endif
	return rp_secure_enable(param);
#endif
}
