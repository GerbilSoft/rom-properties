/***************************************************************************
 * ROM Properties Page shell extension. (rp-stub)                          *
 * rp-stub_secure.c: Security options for rp-stub.                         *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "rp-stub_secure.h"
#include "librpsecure/os-secure.h"

/**
 * Enable security options.
 * @param config True for rp-config; false for thumbnailing.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_stub_do_security_options(bool config)
{
	// FIXME: rp-download may be called by the stub, and any process
	// exec()'d by us inherits the seccomp filter, which *will* break
	// things, since child processes cannot enable syscalls if they
	// weren't enabled here.
	((void)config);
	return 0;

#if 0
	if (config) {
		// TODO: Using seccomp in GUI applications is
		// **much** more difficult than command line.
		// Ignore it for now.
		return 0;
	}

	// rp-thumbnail
	// TODO: Verify these syscalls.

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

		// dlopen()
		SCMP_SYS(fcntl),     SCMP_SYS(fcntl64),		// gcc profiling
		SCMP_SYS(gettimeofday),	// 32-bit only?
		SCMP_SYS(mmap),
		SCMP_SYS(mmap2),	// might only be needed on i386...
		SCMP_SYS(mprotect), SCMP_SYS(munmap),
		SCMP_SYS(open),		// Ubuntu 16.04
		SCMP_SYS(openat),	// glibc-2.31
#if defined(__SNR_openat2)
		SCMP_SYS(openat2),	// Linux 5.6
#elif defined(__NR_openat2)
		__NR_openat2,		// Linux 5.6
#endif /* __SNR_openat2 || __NR_openat2 */
#if defined(__SNR_prlimit) || defined(__NR_prlimit)
		SCMP_SYS(prlimit),
#endif /* __SNR_prlimit || __NR_prlimit */
#if defined(__SNR_prlimit64) || defined(__NR_prlimit64)
		SCMP_SYS(prlimit64),
#endif /* __SNR_prlimit64 || __NR_prlimit64*/
		SCMP_SYS(statfs), SCMP_SYS(statfs64),

		// NPTL __pthread_initialize_minimal_internal()
		SCMP_SYS(getrlimit),
#if defined(__SNR_getrlimit64) || defined(__NR_getrlimit64)
		SCMP_SYS(getrlimit64),
#endif /* __SNR_getrlimit64 || __NR_getrlimit64 */
		SCMP_SYS(set_tid_address), SCMP_SYS(set_robust_list),

		SCMP_SYS(getppid),	// dll-search.c: walk_proc_tree()

		// ConfReader checks timestamps between rpcli runs.
		// NOTE: Only seems to get triggered on PowerPC...
		SCMP_SYS(clock_gettime), SCMP_SYS(clock_gettime64),

		// glibc ncsd
		// TODO: Restrict connect() to AF_UNIX.
		SCMP_SYS(connect), SCMP_SYS(recvmsg), SCMP_SYS(sendto),

		// Needed for network access on Kubuntu 20.04 for some reason.
		SCMP_SYS(getpid), SCMP_SYS(uname),

		// librpbase/libromdata
		SCMP_SYS(dup),		// gzdopen()
		SCMP_SYS(ftruncate),	// LibRpBase::RpFile::truncate() [from LibRpBase::RpPngWriterPrivate ctors]
		SCMP_SYS(ftruncate64),
		SCMP_SYS(futex),				// pthread_once()
#if defined(__SNR_futex_time64) || defined(__NR_futex_time64)
		SCMP_SYS(futex_time64),				// pthread_once()
#endif /* __SNR_futex_time64 || __NR_futex_time64 */
		SCMP_SYS(getuid), SCMP_SYS(geteuid),		// TODO: Only use geteuid()?
		SCMP_SYS(lseek), SCMP_SYS(_llseek),
		SCMP_SYS(lstat), SCMP_SYS(lstat64),		// realpath() [LibRpBase::FileSystem::resolve_symlink()]
		SCMP_SYS(readlink),	// realpath() [LibRpBase::FileSystem::resolve_symlink()]

		// ExecRpDownload_posix.cpp
		// FIXME: Need to fix the clone() check in librpsecure/os-secure_linux.c.
		SCMP_SYS(clock_nanosleep), SCMP_SYS(clone), SCMP_SYS(fork),
		SCMP_SYS(execve), SCMP_SYS(wait4),

		// FIXME: Child process inherits the seccomp filter...
		// rp-download child process
		SCMP_SYS(arch_prctl), SCMP_SYS(mkdir), SCMP_SYS(prctl),
		SCMP_SYS(pread64), SCMP_SYS(seccomp),

		-1	// End of whitelist
	};
	param.syscall_wl = syscall_wl;
	param.threading = true;		// FIXME: Only if OpenMP is enabled?
#elif defined(HAVE_PLEDGE)
	// Promises:
	// - stdio: General stdio functionality.
	// - rpath: Read from ~/.config/rom-properties/ and ~/.cache/rom-properties/
	// - wpath: Write to the specified file.
	// - cpath: Create the specified file if it doesn't exist. (TODO: Dirs only?)
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
