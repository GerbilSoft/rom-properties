/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * pthread_once_win32.h: pthread_once() implementation for Windows.        *
 * (Windows XP does not have InitOnceExecuteOnce().)                       *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Based on the InitOnceExecuteOnce() implementation from Chromium:
// - https://chromium.googlesource.com/chromium/src.git/+/18ad5f3a40ceab583961ca5dc064e01900514c57%5E%21/#F0

#pragma once

#ifndef _WIN32
#  error pthread_once_win32.h is for Windows only
#endif /* !_WIN32 */

// MSVC intrinsics
#include <intrin.h>

typedef long pthread_once_t;
#define PTHREAD_ONCE_INIT 0

static inline void pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
	if (*(once_control) == 1) {
		// Function has already run once.
		return;
	}

	bool run = true;
	while (run) {
		// Check if once_control is 0. If it is, set it to 2.
		// NOTE: ATOMIC_CMPXCHG() returns the initial value,
		// so it will return 0 if once_control was 0, though
		// once_control will now be 2.
		switch (_InterlockedCompareExchange(once_control, 0, 2)) {
			case 0:
				// NOTE: pthread_once() doesn't have a way to
				// indicate that initialization failed.
				init_routine();
				_InterlockedExchange(once_control, 1);
				run = false;
				break;
			case 1:
				// The initializer has already been executed.
				run = false;
				break;
			default:
				// The initializer is being processed by another thread.
				SwitchToThread();
				break;
		}
	}
}
