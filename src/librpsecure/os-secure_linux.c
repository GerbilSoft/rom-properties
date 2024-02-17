/***************************************************************************
 * ROM Properties Page shell extension. (librpsecure)                      *
 * os-secure_linux.c: OS security functions. (Linux)                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "os-secure.h"

// C includes.
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// libseccomp
#include <seccomp.h>
#include <sys/prctl.h>
#include <linux/sched.h>	// CLONE_THREAD

#include "seccomp-debug.h"
#ifdef ENABLE_SECCOMP_DEBUG
#  define SCMP_ACTION SCMP_ACT_TRAP
#else /* !ENABLE_SECCOMP_DEBUG */
#  define SCMP_ACTION SCMP_ACT_KILL
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
		fputs("*** ERROR: rp_secure_enable() called with NULL syscall whitelist.\n", stderr);
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
#ifdef ENABLE_SECCOMP_DEBUG
		fputs("*** ERROR: seccomp_init() failed.\n", stderr);
#endif /* ENABLE_SECCOMP_DEBUG */
		return -ENOSYS;
	}

	static const int syscall_wl_std[] = {
		// Allow basic syscalls.
		SCMP_SYS(brk),
		SCMP_SYS(exit),
		SCMP_SYS(exit_group),
		SCMP_SYS(read),
		SCMP_SYS(rt_sigreturn),
		SCMP_SYS(write),

		// restart_syscall() is called by glibc to restart
		// certain syscalls if they're interrupted.
		SCMP_SYS(restart_syscall),

		// OpenMP [and also abort()]
		// NOTE: Also used by Ubuntu 20.04's DNS resolver.
		SCMP_SYS(rt_sigaction),
		SCMP_SYS(rt_sigprocmask),

#ifndef NDEBUG
		// abort() [called by assert()]
		SCMP_SYS(getpid),
		SCMP_SYS(gettid),
		SCMP_SYS(tgkill),
		SCMP_SYS(uname),	// needed on some systems
#endif /* !NDEBUG */

#ifdef GCOV
		SCMP_SYS(getpid),	// gcov uses getpid() in gcov_open() if GCOV_LOCKED
					// is defined when compiling gcc.
#endif /* GCOV */

		-1	// End of whitelist
	};

	// Whitelist the standard syscalls.
	for (const int *p = syscall_wl_std; *p != -1; p++) {
		seccomp_rule_add_array(ctx, SCMP_ACT_ALLOW, *p, 0, NULL);
	}

	// Multi-threading syscalls.
	if (param.threading) {
		// clone() syscall. Only allow threads.
		const struct scmp_arg_cmp clone_params[] = {
			SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_THREAD, CLONE_THREAD),
		};
		seccomp_rule_add_array(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clone),
			(unsigned int)(sizeof(clone_params)/sizeof(clone_params[0])), clone_params);

		// Other syscalls for multi-threading.
		static const int syscall_wl_threading[] = {
			SCMP_SYS(clone),
			SCMP_SYS(set_robust_list),
#if defined(__SNR_clone3)
			SCMP_SYS(clone3),	// pthread_create() with glibc-2.34
#elif defined(__NR_clone3)
			__NR_clone3,		// pthread_create() with glibc-2.34
#endif /* __SNR_clone3 || __NR_clone3 */
#if defined(__SNR_rseq)
			SCMP_SYS(rseq),		// restartable sequences, glibc-2.35
#elif defined(__NR_rseq)
			__NR_rseq,		// restartable sequences, glibc-2.35
#endif /* __SNR_rseq || __NR_rseq */

#ifdef __clang__
			// LLVM/clang's OpenMP implementation (libomp) calls
			// the following functions:
			// - getuid() [__kmp_reg_status_name()]
			// - ftruncate64() [__kmp_register_library_startup(); used on an SHM FD]
			// - getdents64() [sysconf(), __kmp_get_xproc()]
			// - getrlimit64() [prlimit64()] [__kmp_runtime_initialize()]
			// - sysinfo() [get_phys_pages() -> qsort_r() -> __kmp_stg_init()]
			// - sched_getaffinity() [__kmp_affinity_determine_capable()]
			// - sched_setaffinity() [KMPNativeAffinity::Mask::set_system_affinity()]
			// - sched_yield() [__kmp_wait_template<>]
			// - unlink() [shm_unlink() -> __kmp_unregister_library()] [!!]
			// - madvise() [called on shutdown for some reason if OpenMP is initialized]
			// TODO: Only add these if compiling with OpenMP.
			// TODO: Maybe allow the sched_() functions regardless of compiler?
			// FIXME: For ftruncate() and unlink(), only allow use of the SHM FD.
			SCMP_SYS(getuid),
			SCMP_SYS(ftruncate), SCMP_SYS(ftruncate64),
			SCMP_SYS(getdents), SCMP_SYS(getdents64),
			SCMP_SYS(getrlimit), SCMP_SYS(prlimit64),
			SCMP_SYS(sysinfo),
			SCMP_SYS(sched_getaffinity),
			SCMP_SYS(sched_setaffinity),
			SCMP_SYS(sched_yield),
			SCMP_SYS(unlink),
			SCMP_SYS(madvise),
#endif /* __clang__ */

			-1	// End of whitelist
		};

		for (const int *p = syscall_wl_threading; *p != -1; p++) {
			seccomp_rule_add_array(ctx, SCMP_ACT_ALLOW, *p, 0, NULL);
		}
	}

	// Add syscalls from the caller's whitelist.
	// TODO: More extensive syscall parameters?
	for (const int *p = param.syscall_wl; *p != -1; p++) {
		seccomp_rule_add_array(ctx, SCMP_ACT_ALLOW, *p, 0, NULL);
	}

	// Load the filter.
	int ret = seccomp_load(ctx);
	seccomp_release(ctx);
#ifdef ENABLE_SECCOMP_DEBUG
	if (ret != 0) {
		fprintf(stderr, "*** ERROR: seccomp_load() failed: %s\n", strerror(-ret));
	}
#endif /* ENABLE_SECCOMP_DEBUG */
	return ret;
}
