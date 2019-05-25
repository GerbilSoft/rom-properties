/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * pthread_once.h: pthread_once() implementation for systems that don't    *
 * support pthreads natively.                                              *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Based on the InitOnceExecuteOnce() implementation from Chromium:
// - https://chromium.googlesource.com/chromium/src.git/+/18ad5f3a40ceab583961ca5dc064e01900514c57%5E%21/#F0
#include "config.librpbase.h"
#ifdef HAVE_PTHREADS
#error pthread_once.c should not be compiled if pthreads is available.
#endif /* HAVE_PTHREADS */

#include "common.h"
#include "pthread_once.h"
#include "Atomics.h"

#ifdef HAVE_WIN32_THREADS
# include "libwin32common/RpWin32_sdk.h"
# define pthread_yield() SwitchToThread()
#else
# error Unsupported threading model.
#endif

/**
 * pthread_once() implementation.
 * Based on the InitOnceExecuteOnce() implementation from Chromium.
 * @param once_control
 * @param init_routine
 * @return
 */
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
	// Copied from "perftools_pthread_once" in tcmalloc.

	// Try for a fast path first. Note: this should be an acquire semantics read
	// It is on x86 and x64, where Windows runs.
	if (*once_control != 1) {
		while (1) {
			// Check if once_control is 0. If it is, set it to 2.
			// NOTE: ATOMIC_CMPXCHG() returns the initial value,
			// so it will return 0 if once_control was 0, though
			// once_control will now be 2.
			switch (ATOMIC_CMPXCHG(once_control, 0, 2)) {
				case 0:
					// NOTE: pthread_once() doesn't have a way to
					// indicate that initialization failed.
					init_routine();
					ATOMIC_EXCHANGE(once_control, 1);
					return 0;
				case 1:
					// The initializer has already been executed.
					return 0;
				default:
					// The initializer is being processed by another thread.
					pthread_yield();
					break;
			}
		}
	}

	// Already initialized.
	return 0;
}
