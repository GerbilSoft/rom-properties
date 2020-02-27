/***************************************************************************
 * ROM Properties Page shell extension. (librpsecure)                      *
 * os-secure_linux.c: OS security functions. (Linux)                       *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpsecure.h"
#include "os-secure.h"

// C includes.
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// libseccomp
#include <seccomp.h>
#include <sys/prctl.h>

#ifdef ENABLE_SECCOMP_DEBUG
# include "seccomp-debug.h"
# define SCMP_ACTION SCMP_ACT_TRAP
#else /* !ENABLE_SECCOMP_DEBUG */
# define SCMP_ACTION SCMP_ACT_KILL
#endif /* ENABLE_SECCOMP_DEBUG */

/**
 * Enable OS-specific security functionality.
 * @param param OS-specific parameter.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_secure_enable(rp_secure_param_t param)
{
	assert(param.syscall_wl != NULL);
	if (!param.syscall_wl) {
		printf("*** ERROR: rp_secure_enable() called with NULL syscall whitelist.\n");
		abort();
	}

	// Ensure child processes will never be granted more
	// privileges via setuid, capabilities, etc.
	prctl(PR_SET_NO_NEW_PRIVS, 1);
#ifndef ENABLE_SECCOMP_DEBUG
	// Ensure ptrace() can't be used to escape the seccomp restrictions.
	prctl(PR_SET_DUMPABLE, 0);
#endif /* !ENABLE_SECCOMP_DEBUG */

#ifdef ENABLE_SECCOMP_DEBUG
	// Install the SIGSYS handler for libseccomp.
	seccomp_debug_install_sigsys();
#endif /* ENABLE_SECCOMP_DEBUG */

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

#ifndef NDEBUG
	// abort() [called by assert()]
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getpid), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(gettid), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigaction), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigprocmask), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(tgkill), 0);
#endif /* NDEBUG */

	// Add syscalls from the whitelist.
	// TODO: Syscall parameters?
	for (const int *p = param.syscall_wl; *p != -1; p++) {
		seccomp_rule_add(ctx, SCMP_ACT_ALLOW, *p, 0);
	}

	// Load the filter.
	int ret = seccomp_load(ctx);
	seccomp_release(ctx);
	return ret;
}
