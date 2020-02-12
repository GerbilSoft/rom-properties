/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * os-secure_posix.c: OS security functions. (Linux)                       *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "os-secure.h"

// C includes.
#include <errno.h>
#include <unistd.h>
#include <seccomp.h>
#include <sys/prctl.h>

// Uncomment this to enable seccomp debugging.
//#define SECCOMP_DEBUG 1

#ifdef SECCOMP_DEBUG
# include "seccomp-debug.h"
# define SCMP_ACTION SCMP_ACT_TRAP
#else /* !SECCOMP_DEBUG */
# define SCMP_ACTION SCMP_ACT_KILL
#endif /* SECCOMP_DEBUG */

/**
 * Enable OS-specific security functionality.
 * @return 0 on success; negative POSIX error code on error.
 */
int rpcli_os_secure(void)
{
	// Ensure child processes will never be granted more
	// privileges via setuid, capabilities, etc.
	prctl(PR_SET_NO_NEW_PRIVS, 1);
	// Ensure ptrace() can't be used to escape the seccomp restrictions.
	prctl(PR_SET_DUMPABLE, 0);

#ifdef SECCOMP_DEBUG
	// Install the SIGSYS handler for libseccomp.
	seccomp_debug_install_sigsys();
#endif /* SECCOMP_DEBUG */

	// Initialize the filter.
	scmp_filter_ctx ctx = seccomp_init(SCMP_ACTION);
	if (!ctx) {
		// Cannot initialize seccomp.
		return -ENOSYS;
	}

	// Allow basic syscalls.
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigreturn), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);

	// Syscalls used by rpcli.
	// TODO: Add more syscalls.
	// FIXME: glibc-2.31 uses 64-bit time syscalls that may not be
	// defined in earlier versions, including Ubuntu 14.04.
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(dup), 0);	// gzdopen()
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fstat), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(futex), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(ioctl), 0);	// for devices; also afl-fuzz
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(lseek), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(lstat), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mmap), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mprotect), 0);	// dlopen()
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(open), 0);	// Ubuntu 16.04
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(openat), 0);	// glibc-2.31

	// KeyManager (keys.conf)
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(access), 0);	// LibUnixCommon::isWritableDirectory()
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(stat), 0);	// LibUnixCommon::isWritableDirectory()
	// NOTE: The following syscalls are only made if either access() or stat() can't be run.
	// TODO: Can this happen in other situations?
	//seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(connect), 0);	// ???
	//seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getuid), 0);
	//seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(sendto), 0);	// ???
	//seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(socket), 0);	// ???

#ifndef NDEBUG
	// abort() [called by assert()]
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getpid), 0);	// referenced above
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(gettid), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigaction), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigprocmask), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(tgkill), 0);
#endif /* NDEBUG */

	// Load the filter.
	int ret = seccomp_load(ctx);
	seccomp_release(ctx);
	return ret;
}
