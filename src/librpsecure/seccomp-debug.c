/***************************************************************************
 * ROM Properties Page shell extension. (librpsecure)                      *
 * seccomp-debug.c: Linux seccomp debug functionality.                     *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "seccomp-debug.h"

#ifdef ENABLE_SECCOMP_DEBUG

#include <seccomp.h>

#include <signal.h>
#include <stdio.h>

// NOTE: SYS_SECCOMP is defined in <asm/siginfo.h>, but we can't include it
// because it has all sorts of conflicts with <signal.h>.
#ifndef SYS_SECCOMP
# define SYS_SECCOMP 1
#endif /* !SYS_SECCOMP */

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

static const char *seccomp_debug_get_arch_name(unsigned int arch)
{
	switch (arch) {
		case SCMP_ARCH_X86:		return "i386";
		case SCMP_ARCH_X86_64:		return "amd64";
		case SCMP_ARCH_X32:		return "x32";
		case SCMP_ARCH_ARM:		return "arm";
#ifdef SCMP_ARCH_AARCH64
		case SCMP_ARCH_AARCH64:		return "arm64";
#endif
#ifdef SCMP_ARCH_LOONGARCH32
		case SCMP_ARCH_LOONGARCH32:	return "loongarch32";
#endif
#ifdef SCMP_ARCH_LOONGARCH64
		case SCMP_ARCH_LOONGARCH64:	return "loongarch64";
#endif
#ifdef SCMP_ARCH_M68K
		case SCMP_ARCH_M68K:		return "m68k";
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
#ifdef SCMP_ARCH_RISCV32
		case SCMP_ARCH_RISCV32:		return "riscv32";
#endif
#ifdef SCMP_ARCH_RISCV64
		case SCMP_ARCH_RISCV64:		return "riscv64";
#endif
#ifdef SCMP_ARCH_SHEB
		case SCMP_ARCH_SHEB:		return "sheb";
#endif
#ifdef SCMP_ARCH_SH
		case SCMP_ARCH_SH:		return "sh";
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
static void seccomp_debug_sigsys_handler(int sig, siginfo_t *info, void *ucontext)
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
		seccomp_debug_get_arch_name(info->si_arch),
		seccomp_syscall_resolve_num_arch(info->si_arch, info->si_syscall));
}

/**
 * Install the signal handler for SIGSYS.
 * This will print debugging information for trapped system calls.
 */
void seccomp_debug_install_sigsys(void)
{
	struct sigaction act;
	act.sa_sigaction = seccomp_debug_sigsys_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	sigaction(SIGSYS, &act, NULL);
}

#endif /* !ENABLE_SECCOMP_DEBUG */
