/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * InitOnceExecuteOnceXP.c: InitOnceExecuteOnce() implementation for       *
 * Windows XP. (also works on later systems)                               *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

// Based on the implementation from Chromium:
// - https://chromium.googlesource.com/chromium/src.git/+/18ad5f3a40ceab583961ca5dc064e01900514c57%5E%21/#F0

#ifndef _WIN32
#error InitOnceExecuteOnceXP.c should only be compiled in Windows builds.
#endif /* _WIN32 */

#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0600

#include "InitOnceExecuteOnceXP.h"
#include <wincred.h>

/**
 * InitOnceExecuteOnce()
 * Based on the implementation from Chromium.
 * @param once
 * @param func
 * @param param
 * @param context
 * @return TRUE if initialization succeeded; FALSE if not.
 */
BOOL WINAPI InitOnceExecuteOnceXP(_Inout_ PINIT_ONCE_XP once, _In_ PINIT_ONCE_XP_FN func,
	_Inout_opt_ PVOID param, _Out_opt_ LPVOID *context)
{
	// Copied from "perftools_pthread_once" in tcmalloc.

	// Try for a fast path first. Note: this should be an acquire semantics read
	// It is on x86 and x64, where Windows runs.
	if (*once != 1) {
		while (1) {
			switch (InterlockedCompareExchange(once, 2, 0)) {
				case 0: {
					BOOL bRet = func(once, param, context);
					if (bRet) {
						// Initialization succeeded.
						InterlockedExchange(once, 1);
						return TRUE;
					} else {
						// Initialization failed.
						InterlockedExchange(once, 0);
						return FALSE;
					}
					// Should not get here...
					return FALSE;
				}
				case 1:
					// The initializer has already been executed.
					return TRUE;
				default:
					// The initializer is being processed by another thread.
					SwitchToThread();
					break;
			}
		}
	}

	// Already initialized.
	return TRUE;
}

#endif /* !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0600 */
