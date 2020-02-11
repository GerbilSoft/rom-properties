/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * os-secure_posix.c: OS security functions. (Linux)                       *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "config.rp-download.h"
#include "os-secure.h"

// C includes.
#include <errno.h>
#include <unistd.h>
#include <seccomp.h>
#include <sys/prctl.h>

// Uncomment this to enable seccomp debugging.
//#define SECCOMP_DEBUG 1

#ifdef SECCOMP_DEBUG
# include <signal.h>
# include <stdio.h>
# define SCMP_ACTION SCMP_ACT_TRAP
// NOTE: SYS_SECCOMP is defined in <asm/siginfo.h>, but we can't include it
// because it has all sorts of conflicts with <signal.h>.
# ifndef SYS_SECCOMP
#  define SYS_SECCOMP 1
# endif /* !SYS_SECCOMP */
#else /* !SECCOMP_DEBUG */
# define SCMP_ACTION SCMP_ACT_KILL
#endif /* SECCOMP_DEBUG */

#ifdef SECCOMP_DEBUG
// Syscalls we've already warned about.
// Note that we do a linear O(n) search, which shouldn't be a
// problem because this is only used for debugging.
#define SYSCALL_ARRAY_SIZE 1024
typedef struct _syscall_warn_t {
	int num_syscall;
	unsigned int num_arch;
} syscall_warn_t;
static syscall_warn_t syscalls_warned[SYSCALL_ARRAY_SIZE];
static int syscalls_warned_idx = 0;

static const char *get_arch_name(unsigned int arch)
{
	switch (arch) {
		case SCMP_ARCH_X86:		return "i386";
		case SCMP_ARCH_X86_64:		return "amd64";
		case SCMP_ARCH_X32:		return "x32";
		case SCMP_ARCH_ARM:		return "arm";
#ifdef SCMP_ARCH_AARCH64
		case SCMP_ARCH_AARCH64:		return "arm64";
#endif
#ifdef SCMP_ARCH_MIPS
		case SCMP_ARCH_MIPS:		return "mips";
#endif
#ifdef SCMP_ARCH_MIPS64
		case SCMP_ARCH_MIPS64:		return "mips64";
#endif
#ifdef SCMP_ARCH_MIPS64N32
		case SCMP_ARCH_MIPS64N32:	return "mips64n32";
#endif
#ifdef SCMP_ARCH_MIPSEL
		case SCMP_ARCH_MIPSEL:		return "mipsel";
#endif
#ifdef SCMP_ARCH_MIPSEL64
		case SCMP_ARCH_MIPSEL64:	return "mipsel64";
#endif
#ifdef SCMP_ARCH_MIPSEL64N32
		case SCMP_ARCH_MIPSEL64N32:	return "mipsel64n32";
#endif
#ifdef SCMP_ARCH_PPC
		case SCMP_ARCH_PPC:		return "powerpc";
#endif
#ifdef SCMP_ARCH_PPC64
		case SCMP_ARCH_PPC64:		return "powerpc64";
#endif
#ifdef SCMP_ARCH_PPC64LE
		case SCMP_ARCH_PPC64LE:		return "powerpc64le";
#endif
#ifdef SCMP_ARCH_S390
		case SCMP_ARCH_S390:		return "s390";
#endif
#ifdef SCMP_ARCH_S390X
		case SCMP_ARCH_S390X:		return "s390x";
#endif
#ifdef SCMP_ARCH_PARISC
		case SCMP_ARCH_PARISC:		return "parisc";
#endif
#ifdef SCMP_ARCH_PARISC64
		case SCMP_ARCH_PARISC64:	return "parisc64";
#endif
		default:			return "unknown";
	}
}

/**
 * Signal handler for seccomp in SCMP_ACT_TRAP mode.
 * @param sig		[in] Signal number.
 * @param info		[in] siginfo_t*
 * @param ucontext	[in] ucontext_t*
 */
void seccomp_sigsys_handler(int sig, siginfo_t *info, void *ucontext)
{
	((void)ucontext);	// not used

	if (sig != SIGSYS || info->si_signo != SIGSYS || info->si_code != SYS_SECCOMP) {
		// Incorrect signal.
		return;
	}

	// Check if we've warned about this syscall already.
	for (int i = 0; i < syscalls_warned_idx; i++) {
		if (syscalls_warned[i].num_syscall == info->si_syscall &&
		    syscalls_warned[i].num_arch == info->si_arch)
		{
			// We already warned about this syscall.
			return;
		}
	}

	// Add the new syscall to the list if we have space.
	if (syscalls_warned_idx < SYSCALL_ARRAY_SIZE) {
		syscalls_warned[syscalls_warned_idx].num_syscall = info->si_syscall;
		syscalls_warned[syscalls_warned_idx].num_arch = info->si_arch;
		syscalls_warned_idx++;
	}

	// Print a warning.
	fprintf(stderr, "SYSCALL TRAP: [%s] %s()\n",
		get_arch_name(info->si_arch),
		seccomp_syscall_resolve_num_arch(info->si_arch, info->si_syscall));
}
#endif /* SECCOMP_DEBUG */

/**
 * Enable OS-specific security functionality.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_download_os_secure(void)
{
	// Ensure child processes will never be granted more
	// privileges via setuid, capabilities, etc.
	prctl(PR_SET_NO_NEW_PRIVS, 1);
	// Ensure ptrace() can't be used to escape the seccomp restrictions.
	prctl(PR_SET_DUMPABLE, 0);

#ifdef SECCOMP_DEBUG
	// Install the signal handler for SIGSYS.
	struct sigaction act;
	act.sa_sigaction = seccomp_sigsys_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	sigaction(SIGSYS, &act, NULL);
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

	// Syscalls used by rp-download.
	// TODO: Add more syscalls.
	// FIXME: glibc-2.31 uses 64-bit time syscalls that may not be
	// defined in earlier versions, including Ubuntu 14.04.
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(access), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clock_gettime), 0);
#ifdef __NR_clock_gettime64
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clock_gettime64), 0);
#endif /* __NR_clock_gettime64 */
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fcntl), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fsetxattr), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fstat), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(futex), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getdents), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getrusage), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getuid), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(lseek), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mkdir), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mmap), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(open), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(openat), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(poll), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(select), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(stat), 0);
	//seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(uname), 0);	// ???
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(utimensat), 0);

	// Syscalls used by cURL.
	// NOTE: cURL uses a threaded DNS resolver by default.
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(bind), 0);	// Needed for amiibo.life
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(connect), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clone), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getpeername), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getpid), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getrandom), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getsockname), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getsockopt), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(ioctl), 0);	// ???
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(madvise), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mprotect), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(recvfrom), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(recvmsg), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(sendmmsg), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(sendto), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(set_robust_list), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(setsockopt), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(socket), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(socketpair), 0);
	seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(sysinfo), 0);

#ifndef NDEBUG
	// abort() [called by assert()]
	//seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getpid), 0);	// referenced above
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
